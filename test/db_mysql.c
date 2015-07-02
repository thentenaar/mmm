/**
 * Minimal Migration Manager - MySQL Database Driver Tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

#include "mysql_stubs.h"
#include "../src/db/mysql.c"

static int row_cb_returns = 1;
static int row_cb_called = 0;

static int row_cb(void *userdata, int n_cols, char **fields,
                  char **column_names)
{
	++row_cb_called;

	if (userdata == (void *)1 && n_cols == 1) {
		CU_ASSERT_STRING_EQUAL(column_names[0], "col");
		CU_ASSERT_PTR_NULL(fields[0]);
	}

	if (userdata == (void *)2 && n_cols == 1) {
		CU_ASSERT_STRING_EQUAL(column_names[0], "col");
		CU_ASSERT_STRING_EQUAL(fields[0], "value");
	}

	return row_cb_returns;
}

/**
 * Test that db_mysql_init() and db_mysql_uninit()
 * work.
 */
static void test_mysql_init_uninit(void)
{
	reset_mysql_stubs();
	CU_ASSERT_EQUAL(0, db_mysql_init());
	CU_ASSERT_EQUAL(0, db_mysql_uninit());
}

/**
 * Test that db_mysql_connect() returns NULL if given invalid
 * arguments.
 */
static void mysql_connect_invalid_args(void)
{
	reset_mysql_stubs();
	CU_ASSERT_PTR_NULL(db_mysql_connect("test", 0, NULL, "p", "db"));
	CU_ASSERT_PTR_NULL(db_mysql_connect("test", 0, "u", NULL, "db"));
	CU_ASSERT_PTR_NULL(db_mysql_connect("test", 0, "u", "p", NULL));
	CU_ASSERT_FALSE(mysql_init_called);
}

/**
 * Test that db_mysql_connect() returns NULL if mysql_init()
 * fails.
 */
static void mysql_connect_mysql_init_fails(void)
{
	reset_mysql_stubs();
	mysql_init_returns = NULL;
	CU_ASSERT_PTR_NULL(db_mysql_connect("test", 0, "u", "p", "db"));
	CU_ASSERT_TRUE(mysql_init_called);
	CU_ASSERT_FALSE(mysql_options_called);
}

/**
 * Test that db_mysql_connect() returns NULL and handles
 * errors appropriately when mysql_real_connect() fails.
 */
static void mysql_connect_fails(void)
{
	/* Connecting to a normal host */
	reset_mysql_stubs();
	mysql_init_returns = (MYSQL *)1234;
	mysql_real_connect_returns = NULL;
	mysql_error_returns = "xxx";
	errbuf[0] = '\0';

	CU_ASSERT_PTR_NULL(db_mysql_connect("test", 0, "u", "p", "db"));
	CU_ASSERT_TRUE(mysql_init_called && mysql_options_called);
	CU_ASSERT_TRUE(mysql_real_connect_called);
	CU_ASSERT_TRUE(mysql_error_called && mysql_close_called);
	CU_ASSERT_STRING_EQUAL(errbuf, "db_mysql_connect: xxx\n");

	/* Connecting to a socket */
	reset_mysql_stubs();
	mysql_init_returns = (MYSQL *)1234;
	mysql_real_connect_returns = NULL;
	mysql_error_returns = "xxx";
	errbuf[0] = '\0';

	CU_ASSERT_PTR_NULL(db_mysql_connect("/tst", 0, "u", "p", "db"));
	CU_ASSERT_TRUE(mysql_init_called && mysql_options_called);
	CU_ASSERT_TRUE(mysql_real_connect_called);
	CU_ASSERT_TRUE(mysql_error_called && mysql_close_called);
	CU_ASSERT_STRING_EQUAL(errbuf, "db_mysql_connect: xxx\n");

	/* Connecting to the default host */
	reset_mysql_stubs();
	mysql_init_returns = (MYSQL *)1234;
	mysql_real_connect_returns = NULL;
	mysql_error_returns = "xxx";
	errbuf[0] = '\0';

	CU_ASSERT_PTR_NULL(db_mysql_connect(NULL, 0, "u", "p", "db"));
	CU_ASSERT_TRUE(mysql_init_called && mysql_options_called);
	CU_ASSERT_TRUE(mysql_real_connect_called);
	CU_ASSERT_TRUE(mysql_error_called && mysql_close_called);
	CU_ASSERT_STRING_EQUAL(errbuf, "db_mysql_connect: xxx\n");
}

/**
 * Test that db_mysql_connect() works.
 */
static void test_mysql_connect(void)
{
	reset_mysql_stubs();
	mysql_init_returns = (MYSQL *)1234;
	mysql_real_connect_returns = mysql_init_returns;

	CU_ASSERT_PTR_NOT_NULL(db_mysql_connect(NULL, 0, "u", "p",
	                                        "db"));
	CU_ASSERT_TRUE(mysql_init_called && mysql_options_called);
	CU_ASSERT_TRUE(mysql_real_connect_called);
	CU_ASSERT_FALSE(mysql_error_called || mysql_close_called);
}

/**
 * Test that db_mysql_query() returns 1 if given
 * invalid args
 */
static void mysql_query_invalid_args(void)
{
	MYSQL *dbh = (MYSQL *)1234;

	reset_mysql_stubs();
	row_cb_called = 0;
	CU_ASSERT_EQUAL(1, db_mysql_query(NULL, "test", row_cb, NULL));
	CU_ASSERT_EQUAL(1, db_mysql_query(dbh, NULL, row_cb, NULL));
	CU_ASSERT_FALSE(row_cb_called);
	CU_ASSERT_FALSE(mysql_real_query_called);
}

/**
 * Test that db_mysql_query() returns 1 and handles error messages
 * correctly when mysql_real_query() fails.
 */
static void mysql_query_fails(void)
{
	MYSQL *dbh = (MYSQL *)1234;

	/* Without an error message */
	reset_mysql_stubs();
	mysql_real_query_returns = 1;
	mysql_error_returns = NULL;
	CU_ASSERT_EQUAL(1, db_mysql_query(dbh, "test", row_cb, NULL));
	CU_ASSERT_TRUE(mysql_real_query_called && mysql_error_called);

	/* With an error message */
	reset_mysql_stubs();
	mysql_real_query_returns = 1;
	mysql_error_returns = "xxx";
	errbuf[0] = '\0';

	CU_ASSERT_EQUAL(1, db_mysql_query(dbh, "test", row_cb, NULL));
	CU_ASSERT_TRUE(mysql_real_query_called && mysql_error_called);
	CU_ASSERT_STRING_EQUAL(errbuf, "db_mysql_query: query failed: "
	                               "xxx\n");
}

/**
 * Test that db_mysql_query() returns 0 if no results were
 * returned.
 */
static void mysql_query_no_results(void)
{
	MYSQL *dbh = (MYSQL *)1234;

	reset_mysql_stubs();
	mysql_real_query_returns = 0;
	mysql_store_result_returns = NULL;
	mysql_next_result_returns = -1;
	CU_ASSERT_EQUAL(0, db_mysql_query(dbh, "test", row_cb, NULL));
	CU_ASSERT_TRUE(mysql_real_query_called);
	CU_ASSERT_TRUE(mysql_store_result_called);
	CU_ASSERT_TRUE(mysql_next_result_called);
	CU_ASSERT_FALSE(mysql_num_rows_called);
}

/**
 * Test that db_mysql_query() correctly handles the case when
 * no rows are indicated.
 */
static void mysql_query_no_rows(void)
{
	MYSQL *dbh = (MYSQL *)1234;

	reset_mysql_stubs();
	mysql_real_query_returns = 0;
	mysql_store_result_returns = (MYSQL_RES *)1234;
	mysql_num_rows_returns = 0;
	mysql_num_fields_returns = 1;
	mysql_next_result_returns = -1;

	CU_ASSERT_EQUAL(0, db_mysql_query(dbh, "test", row_cb, NULL));
	CU_ASSERT_TRUE(mysql_real_query_called);
	CU_ASSERT_TRUE(mysql_store_result_called);
	CU_ASSERT_TRUE(mysql_num_rows_called && mysql_num_fields_called);
	CU_ASSERT_TRUE(mysql_next_result_called);
	CU_ASSERT_FALSE(mysql_fetch_fields_called);
}

/**
 * Test that db_mysql_query() correctly handles the case when
 * no cols are indicated.
 */
static void mysql_query_no_cols(void)
{
	MYSQL *dbh = (MYSQL *)1234;

	reset_mysql_stubs();
	mysql_real_query_returns = 0;
	mysql_store_result_returns = (MYSQL_RES *)1234;
	mysql_num_rows_returns = 1;
	mysql_num_fields_returns = 0;
	mysql_next_result_returns = -1;

	CU_ASSERT_EQUAL(0, db_mysql_query(dbh, "test", row_cb, NULL));
	CU_ASSERT_TRUE(mysql_real_query_called);
	CU_ASSERT_TRUE(mysql_store_result_called);
	CU_ASSERT_TRUE(mysql_num_rows_called && mysql_num_fields_called);
	CU_ASSERT_TRUE(mysql_next_result_called);
	CU_ASSERT_TRUE(mysql_free_result_called);
	CU_ASSERT_FALSE(mysql_fetch_fields_called);
}

/**
 * Test that db_mysql_query() correctly handles the case when
 * no callback is specified.
 */
static void mysql_query_no_callback(void)
{
	MYSQL *dbh = (MYSQL *)1234;

	reset_mysql_stubs();
	mysql_real_query_returns = 0;
	mysql_store_result_returns = (MYSQL_RES *)1234;
	mysql_num_rows_returns = 1;
	mysql_num_fields_returns = 1;
	mysql_next_result_returns = -1;

	CU_ASSERT_EQUAL(0, db_mysql_query(dbh, "test", NULL, NULL));
	CU_ASSERT_TRUE(mysql_real_query_called);
	CU_ASSERT_TRUE(mysql_store_result_called);
	CU_ASSERT_TRUE(mysql_num_rows_called && mysql_num_fields_called);
	CU_ASSERT_TRUE(mysql_next_result_called);
	CU_ASSERT_TRUE(mysql_free_result_called);
	CU_ASSERT_FALSE(mysql_fetch_fields_called);
}

/**
 * Test that db_mysql_query() correctly handles the case when
 * no fields are returned.
 */
static void mysql_query_no_fields(void)
{
	MYSQL *dbh = (MYSQL *)1234;

	reset_mysql_stubs();
	mysql_real_query_returns = 0;
	mysql_store_result_returns = (MYSQL_RES *)1234;
	mysql_num_rows_returns = 1;
	mysql_num_fields_returns = 1;
	mysql_fetch_fields_returns = NULL;
	mysql_next_result_returns = -1;

	CU_ASSERT_EQUAL(0, db_mysql_query(dbh, "test", row_cb, NULL));
	CU_ASSERT_TRUE(mysql_real_query_called);
	CU_ASSERT_TRUE(mysql_store_result_called);
	CU_ASSERT_TRUE(mysql_num_rows_called && mysql_num_fields_called);
	CU_ASSERT_TRUE(mysql_fetch_fields_called);
	CU_ASSERT_TRUE(mysql_next_result_called);
	CU_ASSERT_TRUE(mysql_free_result_called);
	CU_ASSERT_FALSE(mysql_fetch_row_called);
}

/**
 * Test that db_mysql_query() works.
 */
static void test_mysql_query(void)
{
	MYSQL *dbh = (MYSQL *)1234;
	MYSQL_FIELD fields[1] = { {"col"} };
	char *row[1] = { "val" };

	reset_mysql_stubs();
	mysql_real_query_returns = 0;
	mysql_store_result_returns = (MYSQL_RES *)1234;
	mysql_num_rows_returns = 1;
	mysql_num_fields_returns = 1;
	mysql_fetch_fields_returns = fields;
	mysql_fetch_row_returns = (MYSQL_ROW)&row;
	mysql_next_result_returns = -1;

	CU_ASSERT_EQUAL(0, db_mysql_query(dbh, "test", row_cb, NULL));
	CU_ASSERT_TRUE(mysql_real_query_called);
	CU_ASSERT_TRUE(mysql_store_result_called);
	CU_ASSERT_TRUE(mysql_num_rows_called && mysql_num_fields_called);
	CU_ASSERT_TRUE(mysql_fetch_fields_called);
	CU_ASSERT_TRUE(mysql_next_result_called);
	CU_ASSERT_TRUE(mysql_free_result_called);
	CU_ASSERT_TRUE(mysql_fetch_row_called);
}

/**
 * Test that db_mysql_disconnect() works.
 */
static void test_mysql_disconnect(void)
{
	reset_mysql_stubs();
	db_mysql_disconnect(NULL);
	CU_ASSERT_FALSE(mysql_close_called);
	db_mysql_disconnect((void *)1234);
	CU_ASSERT_TRUE(mysql_close_called);
}

static CU_TestInfo db_mysql_tests[] = {
	{
		"db_mysql_init() / uninit() - works",
		test_mysql_init_uninit
	},
	{
		"db_mysql_connect() - invalid args",
		mysql_connect_invalid_args
	},
	{
		"db_mysql_connect() - mysql_init() fails",
		mysql_connect_mysql_init_fails
	},
	{
		"db_mysql_connect() - fails",
		mysql_connect_fails
	},
	{
		"db_mysql_connect() - works",
		test_mysql_connect
	},
	{
		"db_mysql_query() - invalid args",
		mysql_query_invalid_args
	},
	{
		"db_mysql_query() - fails",
		mysql_query_fails
	},
	{
		"db_mysql_query() - no results",
		mysql_query_no_results
	},
	{
		"db_mysql_query() - no rows",
		mysql_query_no_rows
	},
	{
		"db_mysql_query() - no columns",
		mysql_query_no_cols
	},
	{
		"db_mysql_query() - no callback",
		mysql_query_no_callback
	},
	{
		"db_mysql_query() - no fields",
		mysql_query_no_fields
	},
	{
		"db_mysql_query() - works",
		test_mysql_query
	},
	{
		"db_mysql_disconnect() - works",
		test_mysql_disconnect
	},

	CU_TEST_INFO_NULL
};

void db_mysql_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Database Driver: mysql", NULL, NULL);
	while (db_mysql_tests[i].pName) {
		CU_add_test(suite, db_mysql_tests[i].pName,
		            db_mysql_tests[i].pTestFunc);
		i++;
	}
}

