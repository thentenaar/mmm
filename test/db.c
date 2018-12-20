/**
 * Minimal Migration Manager - Database Layer Tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <CUnit/CUnit.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

#include "../src/db.h"
#include "../src/db.c"

/* {{{ DB driver stub */
/**
 * This is a test-controllable instance of a db
 * driver, which allows us to track whether certain
 * driver functions were called, that they were passed
 * the proper params, etc.
 */
static int driver_init_called       = 0;
static int driver_uninit_called     = 0;
static int driver_connect_called    = 0;
static int driver_query_called      = 0;
static int driver_disconnect_called = 0;

static int driver_init(void)
{
	driver_init_called++;

	/* Fail on the 2nd call */
	if (driver_init_called == 2)
		return 1;
	return 0;
}

static int driver_uninit(void)
{
	driver_uninit_called++;

	/* Fail on the 2nd call */
	if (driver_uninit_called == 2)
		return 1;
	return 0;
}

static void driver_config(void)
{
	return;
}

static void *driver_connect(const char *host, const unsigned short port,
                            const char *username, const char *password,
                            const char *db)
{
	driver_connect_called++;

	CU_ASSERT_EQUAL(port, 0);
	CU_ASSERT_PTR_NULL(username);
	CU_ASSERT_PTR_NULL(password);
	CU_ASSERT_PTR_NOT_NULL(db);
	CU_ASSERT_STRING_EQUAL(db, "test");

	if (host && strlen(host) == 4 && !memcmp(host, "fail", 4))
		return NULL;
	return (void *)1234;
}

static int driver_query(void *dbh, const char *query,
                        db_row_callback_t callback,
                        void *userdata)
{
	driver_query_called++;

	CU_ASSERT_PTR_NOT_NULL_FATAL(dbh);
	CU_ASSERT_EQUAL_FATAL(dbh, (void *)1234);
	CU_ASSERT_PTR_NOT_NULL_FATAL(query);
	CU_ASSERT_TRUE(!callback);
	CU_ASSERT_PTR_NULL(userdata);

	if (strlen(query) == 4 && !memcmp(query, "fail", 4))
		return 1;
	return 0;
}

static void driver_disconnect(void *dbh)
{
	driver_disconnect_called++;
	CU_ASSERT_EQUAL_FATAL(dbh, (void *)1234);
}

const struct db_driver_vtable driver_without_init = {
	"no-init",
	0,
	NULL, /* config */
	NULL, /* init */
	NULL, /* uninit */
	NULL, /* driver_connect, */
	NULL, /* driver_query, */
	NULL  /* driver_disconnect */
};

const struct db_driver_vtable driver_with_init = {
	"init",
	1,
	driver_config,
	driver_init,
	driver_uninit,
	driver_connect,
	driver_query,
	driver_disconnect
};
/* }}} */

/**
 * Test that db_init() skips a driver without an
 * init callback.
 */
static void db_init_skips_drivers_without_init(void)
{
	errbuf[0] = '\0';
	driver_init_called = 0;
	drivers[0] = &driver_without_init;
	db_init();
	CU_ASSERT_PTR_NOT_NULL(drivers[0]);
	CU_ASSERT_EQUAL(errbuf[0], '\0');
	CU_ASSERT_FALSE(driver_init_called);
}

/**
 * That that we get the correct error message if db_init()
 * fails to initialize a driver.
 */
static void db_init_fails_to_init_driver(void)
{
	const char *err = "failed to initialize 'init'\n";
	errbuf[0] = '\0';
	drivers[0] = &driver_with_init;
	driver_init_called = 1;
	db_init();
	CU_ASSERT_EQUAL(driver_init_called, 2);
	CU_ASSERT_PTR_NULL(drivers[0]);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that db_get_config_cb() returns NULL if
 * passed invalid parameters.
 */
static void db_get_config_cb_invalid_params(void)
{
	drivers[0] = &driver_with_init;
	CU_ASSERT_TRUE(!db_get_config_cb(NULL, 1));
	CU_ASSERT_TRUE(!db_get_config_cb("init", 0));
}

/**
 * Test that db_get_config_cb() fails if no drivers
 * are usable.
 */
static void db_get_config_cb_no_usable_drivers(void)
{
	memset(drivers, 0, sizeof(drivers));
	CU_ASSERT_TRUE(!db_get_config_cb("init", 4));
}

/**
 * Test that db_get_config_cb() returns the specified
 * driver's config callback.
 */
static void test_db_get_config_cb(void)
{
	memset(drivers, 0, sizeof(drivers));
	drivers[1] = &driver_with_init;
	drivers[2] = &driver_without_init;
	CU_ASSERT_EQUAL(db_get_config_cb("init", 4), driver_config);
	CU_ASSERT_EQUAL(db_get_config_cb("no-init", 7), NULL);
}

/**
 * Test that db_connect() returns 1 if passed
 * invalid parameters.
 */
static void db_connect_invalid_params(void)
{
	CU_ASSERT_EQUAL(1, db_connect(NULL, NULL, 0, NULL, NULL, "test"));
	CU_ASSERT_EQUAL(1, db_connect("test", NULL, 0, NULL, NULL, NULL));
}

/**
 * Test that db_connect() return 1 if no drivers
 * are usable.
 */
static void db_connect_no_usable_drivers(void)
{
	memset(drivers, 0, sizeof(drivers));
	CU_ASSERT_EQUAL(1, db_connect("test", NULL, 0, NULL, NULL,
	                              "test"));
}

/**
 * Test that db_connect() return 1 if the underlying
 * callback returns NULL (i.e. connection failed.)
 */
static void db_connect_connection_fails(void)
{
	memset(drivers, 0, sizeof(drivers));
	drivers[1] = &driver_with_init;
	driver_connect_called = 0;
	CU_ASSERT_EQUAL(1, db_connect("init", "fail", 0, NULL, NULL,
	                              "test"));
	CU_ASSERT_TRUE(driver_connect_called);
}

/**
 * Test that db_connect() return 1 if a database
 * connection has already been established.
 */
static void db_connect_session_exists(void)
{
	memset(drivers, 0, sizeof(drivers));
	drivers[2] = &driver_with_init;
	driver_connect_called = 0;
	session.dbh = (void *)1234;
	session.type = 2;

	CU_ASSERT_EQUAL(1, db_connect("init", NULL, 0, NULL, NULL,
	                              "test"));

	CU_ASSERT_FALSE(driver_connect_called);
	CU_ASSERT_EQUAL(session.dbh, (void *)1234);
	CU_ASSERT_EQUAL(session.type, 2);
}

/**
 * Test that db_connect() return 0 if the underlying
 * callback returns non-NULL (i.e. connection successful.)
 */
static void test_db_connect(void)
{
	memset(drivers, 0, sizeof(drivers));
	drivers[2] = &driver_with_init;
	driver_connect_called = 0;
	session.dbh = NULL;
	session.type = 0;
	CU_ASSERT_EQUAL(0, db_connect("init", NULL, 0, NULL, NULL,
	                              "test"));

	CU_ASSERT_TRUE(driver_connect_called);
	CU_ASSERT_EQUAL(session.dbh, (void *)1234);
	CU_ASSERT_EQUAL(session.type, 2);
}

/**
 * Test that db_query() returns the proper result when
 * passed invalid parameters.
 */
static void db_query_invalid_params(void)
{
	memset(drivers, 0, sizeof(drivers));
	drivers[0] = &driver_with_init;
	driver_query_called = 0;
	session.dbh = (void *)1234;
	session.type = 0;

	/* !query */
	CU_ASSERT_EQUAL(-1, db_query(NULL, NULL, NULL));
}

/**
 * Test that db_query() returns -1 if the driver indicated
 * by session->type isn't usable.
 */
static void db_query_no_usable_drivers(void)
{
	memset(drivers, 0, sizeof(drivers));

	session.dbh = NULL;
	session.type = 0;

	/* !session.dbh */
	CU_ASSERT_EQUAL(-1, db_query("test", NULL, NULL));

	/* session.type == N_DB_DRIVERS */
	session.type = N_DB_DRIVERS;
	session.dbh = (void *)1234;
	CU_ASSERT_EQUAL(-1, db_query("test", NULL, NULL));

	/* session.type > N_DB_DRIVERS */
	session.type = N_DB_DRIVERS + 1;
	session.dbh = (void *)1234;
	CU_ASSERT_EQUAL(-1, db_query("test", NULL, NULL));

	/* !drivers[session->type] */
	session.type = 1;
	session.dbh = (void *)1234;
	CU_ASSERT_EQUAL(-1, db_query("test", NULL, NULL));

	/* !drivers[session->type]->query */
	drivers[1] = &driver_without_init;
	CU_ASSERT_EQUAL(-1, db_query("test", NULL, NULL));
}

/**
 * Test that db_query() calls the driver callback, and returns
 * the proper result.
 */
static void test_db_query(void)
{
	memset(drivers, 0, sizeof(drivers));
	drivers[1] = &driver_with_init;
	session.type = 1;
	session.dbh = (void *)1234;


	CU_ASSERT_EQUAL(0, db_query("test", NULL, NULL));
	CU_ASSERT_EQUAL(driver_query_called, 1);
	CU_ASSERT_EQUAL(1, db_query("fail", NULL, NULL));
	CU_ASSERT_EQUAL(driver_query_called, 2);
}

/**
 * Test that db_has_transactional_ddl() works.
 */
static void test_db_has_transactional_ddl(void)
{
	memset(drivers, 0, sizeof(drivers));
	drivers[1] = &driver_with_init;
	session.type = 1;
	session.dbh = (void *)1234;
	CU_ASSERT_TRUE(db_has_transactional_ddl());

	drivers[1] = &driver_without_init;
	CU_ASSERT_FALSE(db_has_transactional_ddl());
}

/**
 * Test that the driver disconnect callback doesn't get called
 * by db_disconnect() when it's given invalid parameters.
 */
static void db_disconnect_invalid_params(void)
{
	memset(drivers, 0, sizeof(drivers));
	drivers[0] = &driver_with_init;
	driver_disconnect_called = 0;

	/* !session.dbh */
	session.dbh = NULL;
	session.type = 0;
	db_disconnect();
	CU_ASSERT_FALSE(driver_disconnect_called);

	/* session.type == N_DB_DRIVERS */
	session.type = N_DB_DRIVERS;
	db_disconnect();
	CU_ASSERT_FALSE_FATAL(driver_disconnect_called);

	/* session.type > N_DB_DRIVERS */
	session.type = N_DB_DRIVERS + 1;
	db_disconnect();
	CU_ASSERT_FALSE_FATAL(driver_disconnect_called);

	/* !drivers[session.type] */
	session.type = 1;
	db_disconnect();
	CU_ASSERT_FALSE_FATAL(driver_disconnect_called);
}

/**
 * Test that db_disconnect() doesn't attempt to call
 * the disconnect callback if the driver specified by
 * session->type isn't usable.
 */
static void db_disconnect_no_usable_drivers(void)
{
	memset(drivers, 0, sizeof(drivers));
	drivers[0] = &driver_without_init;
	driver_disconnect_called = 0;

	session.type = 0;
	session.dbh = (void *)1234;

	/* !drivers[session.type]->disconnect */
	db_disconnect();
	CU_ASSERT_FALSE(driver_disconnect_called);
}

/**
 * Test that db_disconnect() calls the driver callback.
 */
static void test_db_disconnect(void)
{
	memset(drivers, 0, sizeof(drivers));
	drivers[0] = &driver_with_init;
	driver_disconnect_called = 0;

	session.type = 0;
	session.dbh = (void *)1234;

	db_disconnect();
	CU_ASSERT_TRUE(driver_disconnect_called);
}

/**
 * Test that db_uninit() skips a driver without an
 * uninit callback.
 */
static void db_uninit_skips_drivers_without_uninit(void)
{
	errbuf[0] = '\0';
	driver_uninit_called = 0;
	memset(drivers, 0, sizeof(drivers));
	drivers[0] = &driver_without_init;

	db_uninit();
	CU_ASSERT_PTR_NOT_NULL(drivers[0]);
	CU_ASSERT_EQUAL(errbuf[0], '\0');
	CU_ASSERT_FALSE(driver_uninit_called);
}

/**
 * That that we get the correct error message if db_uninit()
 * fails to uninitialize a driver.
 */
static void db_uninit_fails_to_uninit_driver(void)
{
	const char *err = "failed to uninitialize 'init'\n";

	errbuf[0] = '\0';
	memset(drivers, 0, sizeof(drivers));
	drivers[1] = &driver_with_init;
	driver_uninit_called = 1;

	db_uninit();
	CU_ASSERT_EQUAL(driver_uninit_called, 2);
	CU_ASSERT_PTR_NULL(drivers[0]);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

static CU_TestInfo db_tests[] = {
	{
		"db_init - skips drivers without init()",
		db_init_skips_drivers_without_init
	},
	{
		"db_init - fails to init driver",
		db_init_fails_to_init_driver
	},
	{
		"db_get_config_cb - invalid params",
		db_get_config_cb_invalid_params
	},
	{
		"db_get_config_cb - no usable drivers",
		db_get_config_cb_no_usable_drivers
	},
	{
		"db_get_config_cb works",
		test_db_get_config_cb
	},
	{
		"db_connect - invalid params",
		db_connect_invalid_params
	},
	{
		"db_connect - no usable drivers",
		db_connect_no_usable_drivers
	},
	{
		"db_connect - connection failed",
		db_connect_connection_fails
	},
	{
		"db_connect - session exists",
		db_connect_session_exists
	},
	{
		"db_connect works",
		test_db_connect
	},
	{
		"db_query - invalid params",
		db_query_invalid_params
	},
	{
		"db_query - no usable drivers",
		db_query_no_usable_drivers
	},
	{
		"db_query works",
		test_db_query
	},
	{
		"db_has_transactional_ddl works",
		test_db_has_transactional_ddl
	},
	{
		"db_disconnect - invalid params",
		db_disconnect_invalid_params
	},
	{
		"db_disconnect - no usable drivers",
		db_disconnect_no_usable_drivers
	},
	{
		"db_disconnect works",
		test_db_disconnect
	},
	{
		"db_uninit - skips drivers without uninit()",
		db_uninit_skips_drivers_without_uninit
	},
	{
		"db_uninit - fails to uninit driver",
		db_uninit_fails_to_uninit_driver
	},

	CU_TEST_INFO_NULL
};

void db_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Database Layer", NULL, NULL);
	while (db_tests[i].pName) {
		CU_add_test(suite, db_tests[i].pName,
		            db_tests[i].pTestFunc);
		i++;
	}
}

