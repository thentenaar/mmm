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
#include <check.h>

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

	ck_assert_int_eq(port, 0);
	ck_assert_ptr_null(username);
	ck_assert_ptr_null(password);
	ck_assert_str_eq(db, "test");

	if (host && strlen(host) == 4 && !memcmp(host, "fail", 4))
		return NULL;
	return (void *)1234;
}

static int driver_query(void *dbh, const char *query,
                        db_row_callback_t callback,
                        void *userdata)
{
	driver_query_called++;

	ck_assert_ptr_eq(dbh, (void *)1234);
	ck_assert_ptr_nonnull(query);
	ck_assert(!callback);
	ck_assert_ptr_null(userdata);

	if (strlen(query) == 4 && !memcmp(query, "fail", 4))
		return 1;
	return 0;
}

static void driver_disconnect(void *dbh)
{
	driver_disconnect_called++;
	ck_assert_ptr_eq(dbh, (void *)1234);
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
START_TEST(db_init_skips_drivers_without_init)
{
	*errbuf = '\0';
	driver_init_called = 0;
	drivers[0] = &driver_without_init;
	db_init();

	ck_assert(!*errbuf && !driver_init_called);
}
END_TEST

/**
 * That that we get the correct error message if db_init()
 * fails to initialize a driver.
 */
START_TEST(db_init_fails_to_init_driver)
{
	const char *err = "failed to initialize 'init'\n";

	*errbuf = '\0';
	drivers[0] = &driver_with_init;
	driver_init_called = 1;

	db_init();
	ck_assert_int_eq(driver_init_called, 2);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that db_get_config_cb() returns NULL if
 * passed invalid parameters.
 */
START_TEST(db_get_config_cb_invalid_params)
{
	drivers[0] = &driver_with_init;
	ck_assert(!db_get_config_cb(NULL, 1));
	ck_assert(!db_get_config_cb("init", 0));
}
END_TEST

/**
 * Test that db_get_config_cb() fails if no drivers
 * are usable.
 */
START_TEST(db_get_config_cb_no_usable_drivers)
{
	memset(drivers, 0, sizeof drivers);
	ck_assert(!db_get_config_cb("init", 4));
}
END_TEST

/**
 * Test that db_get_config_cb() returns the specified
 * driver's config callback.
 */
START_TEST(test_db_get_config_cb)
{
	memset(drivers, 0, sizeof drivers);
	drivers[1] = &driver_with_init;
	drivers[2] = &driver_without_init;

	ck_assert(db_get_config_cb("init", 4) == driver_config);
	ck_assert(!db_get_config_cb("no-init", 7));
}
END_TEST

/**
 * Test that db_connect() returns 1 if passed
 * invalid parameters.
 */
START_TEST(db_connect_invalid_params)
{
	ck_assert_int_ne(db_connect(NULL, NULL, 0, NULL, NULL, "test"), 0);
	ck_assert_int_ne(db_connect("test", NULL, 0, NULL, NULL, NULL), 0);
}
END_TEST

/**
 * Test that db_connect() returns 1 if no drivers
 * are usable.
 */
START_TEST(db_connect_no_usable_drivers)
{
	memset(drivers, 0, sizeof drivers);
	ck_assert_int_ne(db_connect("test", NULL, 0, NULL, NULL, "test"), 0);
}
END_TEST

/**
 * Test that db_connect() returns 1 if the underlying
 * callback returns NULL (i.e. connection failed.)
 */
START_TEST(db_connect_connection_fails)
{
	memset(drivers, 0, sizeof drivers);
	drivers[1] = &driver_with_init;
	driver_connect_called = 0;

	ck_assert_int_ne(db_connect("init", "fail", 0, NULL, NULL, "test"), 0);
	ck_assert(driver_connect_called);
}
END_TEST

/**
 * Test that db_connect() return 1 if a database
 * connection has already been established.
 */
START_TEST(db_connect_session_exists)
{
	memset(drivers, 0, sizeof drivers);
	drivers[2] = &driver_with_init;
	driver_connect_called = 0;
	session.dbh  = (void *)1234;
	session.type = 2;

	ck_assert_int_ne(db_connect("init", NULL, 0, NULL, NULL, "test"), 0);
	ck_assert(!driver_connect_called);
	ck_assert_ptr_eq(session.dbh, (void *)1234);
	ck_assert_uint_eq(session.type, 2);
}
END_TEST

/**
 * Test that db_connect() return 0 if the underlying
 * callback returns non-NULL (i.e. connection successful.)
 */
START_TEST(test_db_connect)
{
	memset(drivers, 0, sizeof drivers);
	drivers[2] = &driver_with_init;
	driver_connect_called = 0;
	session.dbh  = NULL;
	session.type = 0;

	ck_assert_int_eq(db_connect("init", NULL, 0, NULL, NULL, "test"), 0);
	ck_assert(driver_connect_called);
	ck_assert_ptr_eq(session.dbh, (void *)1234);
	ck_assert_uint_eq(session.type, 2);
}
END_TEST

/**
 * Test that db_query() returns the proper result when
 * passed invalid parameters.
 */
START_TEST(db_query_invalid_params)
{
	memset(drivers, 0, sizeof drivers);
	drivers[0] = &driver_with_init;
	driver_query_called = 0;
	session.dbh  = (void *)1234;
	session.type = 0;
	ck_assert_int_ne(db_query(NULL, NULL, NULL), 0);
}
END_TEST

/**
 * Test that db_query() returns -1 if the driver indicated
 * by session->type isn't usable.
 */
START_TEST(db_query_no_usable_drivers)
{
	memset(drivers, 0, sizeof drivers);
	session.dbh  = NULL;
	session.type = 0;

	/* !session.dbh */
	ck_assert_int_ne(db_query("test", NULL, NULL), 0);

	/* session.type == N_DB_DRIVERS */
	session.type = N_DB_DRIVERS;
	session.dbh  = (void *)1234;
	ck_assert_int_ne(db_query("test", NULL, NULL), 0);

	/* session.type > N_DB_DRIVERS */
	session.type = N_DB_DRIVERS + 1;
	session.dbh  = (void *)1234;
	ck_assert_int_ne(db_query("test", NULL, NULL), 0);

	/* !drivers[session->type] */
	session.type = 1;
	session.dbh  = (void *)1234;
	ck_assert_int_ne(db_query("test", NULL, NULL), 0);

	/* !drivers[session->type]->query */
	drivers[1] = &driver_without_init;
	ck_assert_int_ne(db_query("test", NULL, NULL), 0);
}
END_TEST

/**
 * Test that db_query() calls the driver callback, and returns
 * the proper result.
 */
START_TEST(test_db_query)
{
	memset(drivers, 0, sizeof drivers);
	drivers[1]   = &driver_with_init;
	session.type = 1;
	session.dbh  = (void *)1234;

	ck_assert_int_eq(db_query("test", NULL, NULL), 0);
	ck_assert_int_eq(driver_query_called, 1);
	ck_assert_int_eq(db_query("fail", NULL, NULL), 1);
	ck_assert_int_eq(driver_query_called, 2);
}
END_TEST

/**
 * Test that db_has_transactional_ddl() works.
 */
START_TEST(test_db_has_transactional_ddl)
{
	memset(drivers, 0, sizeof drivers);
	drivers[1]   = &driver_with_init;
	session.type = 1;
	session.dbh  = (void *)1234;
	ck_assert(db_has_transactional_ddl());

	drivers[1] = &driver_without_init;
	ck_assert(!db_has_transactional_ddl());
}
END_TEST

/**
 * Test that the driver disconnect callback doesn't get called
 * by db_disconnect() when it's given invalid parameters.
 */
START_TEST(db_disconnect_invalid_params)
{
	memset(drivers, 0, sizeof drivers);
	drivers[0] = &driver_with_init;
	driver_disconnect_called = 0;

	/* !session.dbh */
	session.dbh  = NULL;
	session.type = 0;
	db_disconnect();
	ck_assert(!driver_disconnect_called);

	/* session.type == N_DB_DRIVERS */
	session.type = N_DB_DRIVERS;
	db_disconnect();
	ck_assert(!driver_disconnect_called);

	/* session.type > N_DB_DRIVERS */
	session.type = N_DB_DRIVERS + 1;
	db_disconnect();
	ck_assert(!driver_disconnect_called);

	/* !drivers[session.type] */
	session.type = 1;
	db_disconnect();
	ck_assert(!driver_disconnect_called);
}
END_TEST

/**
 * Test that db_disconnect() doesn't attempt to call
 * the disconnect callback if the driver specified by
 * session->type isn't usable.
 */
START_TEST(db_disconnect_no_usable_drivers)
{
	memset(drivers, 0, sizeof drivers);
	drivers[0] = &driver_without_init;
	driver_disconnect_called = 0;

	session.type = 0;
	session.dbh  = (void *)1234;

	/* !drivers[session.type]->disconnect */
	db_disconnect();
	ck_assert(!driver_disconnect_called);
}
END_TEST

/**
 * Test that db_disconnect() calls the driver callback.
 */
START_TEST(test_db_disconnect)
{
	memset(drivers, 0, sizeof drivers);
	drivers[0] = &driver_with_init;
	driver_disconnect_called = 0;

	session.type = 0;
	session.dbh  = (void *)1234;
	db_disconnect();
	ck_assert(driver_disconnect_called);
}
END_TEST

/**
 * Test that db_uninit() skips a driver without an
 * uninit callback.
 */
START_TEST(db_uninit_skips_drivers_without_uninit)
{
	*errbuf = '\0';
	driver_uninit_called = 0;
	memset(drivers, 0, sizeof drivers);
	drivers[0] = &driver_without_init;

	db_uninit();
	ck_assert_ptr_nonnull(drivers[0]);
	ck_assert(!*errbuf);
	ck_assert(!driver_uninit_called);
}
END_TEST

/**
 * That that we get the correct error message if db_uninit()
 * fails to uninitialize a driver.
 */
START_TEST(db_uninit_fails_to_uninit_driver)
{
	const char *err = "failed to uninitialize 'init'\n";

	*errbuf = '\0';
	memset(drivers, 0, sizeof drivers);
	drivers[1] = &driver_with_init;
	driver_uninit_called = 1;

	db_uninit();
	ck_assert_int_eq(driver_uninit_called, 2);
	ck_assert_ptr_null(drivers[0]);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

Suite *db_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("Database");
	t = tcase_create("db_init");
	tcase_add_test(t, db_init_skips_drivers_without_init);
	tcase_add_test(t, db_init_fails_to_init_driver);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_get_config_cb");
	tcase_add_test(t, db_get_config_cb_invalid_params);
	tcase_add_test(t, db_get_config_cb_no_usable_drivers);
	tcase_add_test(t, test_db_get_config_cb);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_connect");
	tcase_add_test(t, db_connect_invalid_params);
	tcase_add_test(t, db_connect_no_usable_drivers);
	tcase_add_test(t, db_connect_connection_fails);
	tcase_add_test(t, db_connect_session_exists);
	tcase_add_test(t, test_db_connect);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_query");
	tcase_add_test(t, db_query_invalid_params);
	tcase_add_test(t, db_query_no_usable_drivers);
	tcase_add_test(t, test_db_query);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_has_transactional_ddl");
	tcase_add_test(t, test_db_has_transactional_ddl);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_disconnect");
	tcase_add_test(t, db_disconnect_invalid_params);
	tcase_add_test(t, db_disconnect_no_usable_drivers);
	tcase_add_test(t, test_db_disconnect);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_uninit");
	tcase_add_test(t, db_uninit_skips_drivers_without_uninit);
	tcase_add_test(t, db_uninit_fails_to_uninit_driver);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

