/**
 * Minimal Migration Manager - SQLite3 Database Driver Tests
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

#include "sqlite3_stubs.h"
#include "../src/db/sqlite3.c"

/**
 * Test that db_sqlite3_init() and db_sqlite3_uninit() work.
 */
static void test_sqlite3_init_uninit(void)
{
	CU_ASSERT_EQUAL(0, db_sqlite3_init());
	CU_ASSERT_EQUAL(0, db_sqlite3_uninit());

	sqlite3_initialize_returns = ~SQLITE_OK;
	sqlite3_shutdown_returns = ~SQLITE_OK;

	CU_ASSERT_EQUAL(1, db_sqlite3_init());
	CU_ASSERT_EQUAL(1, db_sqlite3_uninit());

	db_sqlite3_disconnect(NULL);
	db_sqlite3_disconnect((void *)1234);
}

/**
 * Test that db_sqlite3_connect() returns NULL if db
 * is NULL.
 */
static void sqlite3_connect_returns_null_if_null_db(void)
{
	sqlite3_open_dbh = (void *)1234;
	CU_ASSERT_PTR_NULL(db_sqlite3_connect(NULL, 0, NULL, NULL, NULL));
}

/**
 * Test that db_sqlite3_connect() returns NULL if db
 * is an empty string.
 */
static void sqlite3_connect_returns_null_if_empty_db(void)
{
	CU_ASSERT_PTR_NULL(db_sqlite3_connect(NULL, 0, NULL, NULL, ""));
}

/**
 * Test that db_sqlite3_connect() handles error messages if
 * sqlite3_open() fails.
 */
static void sqlite3_connect_handles_error_messages(void)
{
	sqlite3_errmsg_returns = "test";
	sqlite3_open_returns = ~SQLITE_OK;
	errbuf[0] = '\0';

	CU_ASSERT_PTR_NULL(db_sqlite3_connect(NULL, 0, NULL, NULL,
	                                      "test.db"));
	CU_ASSERT_STRING_EQUAL(errbuf, "[sqlite3_connect] test\n");
}

/**
 * Test that db_sqlite3_connect() works.
 */
static void test_sqlite3_connect(void)
{
	void *x;

	sqlite3_open_returns = SQLITE_OK;
	sqlite3_open_dbh = (void *)1234;
	sqlite3_errmsg_returns = NULL;

	x = db_sqlite3_connect(NULL, 0, NULL, NULL, "test.db");
	CU_ASSERT_EQUAL(x, sqlite3_open_dbh);
}

/**
 * Test that db_sqlite3_query() returns 1 if the query
 * fails.
 */
static void sqlite3_query_fails(void)
{
	sqlite3_exec_errmsg = NULL;
	sqlite3_exec_returns = ~SQLITE_OK;
	CU_ASSERT_EQUAL(1, db_sqlite3_query(NULL, "test", NULL, NULL));
}

/**
 * Test that db_sqlite3_query() correctly reports any
 * error message from sqlite.
 */
static void sqlite3_query_fails_with_error_message(void)
{
	const char *err = "query failed: xxx\n";

	*errbuf = '\0';
	sqlite3_exec_errmsg = "xxx";
	sqlite3_exec_returns = ~SQLITE_OK;
	CU_ASSERT_EQUAL(1, db_sqlite3_query(NULL, "test", NULL, NULL));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that db_sqlite3_query() return 0 if the
 * callback causes SQLITE_ABORT to be returned.
 */
static void sqlite3_query_callback_abort(void)
{
	sqlite3_exec_errmsg = NULL;
	sqlite3_exec_returns = SQLITE_ABORT;
	CU_ASSERT_EQUAL(0, db_sqlite3_query(NULL, "test", NULL, NULL));
}

/**
 * Test that db_sqlite3_query() works.
 */
static void test_sqlite3_query(void)
{
	sqlite3_exec_errmsg = NULL;
	sqlite3_exec_returns = SQLITE_OK;
	CU_ASSERT_EQUAL(0, db_sqlite3_query(NULL, "test", NULL, NULL));
}

static CU_TestInfo db_sqlite3_tests[] = {
	{
		"db_sqlite3_init() / db_sqlite3_uninit() work",
		test_sqlite3_init_uninit
	},
	{
		"db_sqlite3_connect() - NULL db",
		sqlite3_connect_returns_null_if_null_db
	},
	{
		"db_sqlite3_connect() - empty db",
		sqlite3_connect_returns_null_if_empty_db
	},
	{
		"db_sqlite3_connect() - handles error messages",
		sqlite3_connect_handles_error_messages
	},
	{
		"db_sqlite3_connect() - works",
		test_sqlite3_connect
	},
	{
		"db_sqlite3_query() - fails",
		sqlite3_query_fails
	},
	{
		"db_sqlite3_query() - fails w/error message",
		sqlite3_query_fails_with_error_message
	},
	{
		"db_sqlite3_query() - callback returns error",
		sqlite3_query_callback_abort
	},
	{
		"db_sqlite3_query() - works",
		test_sqlite3_query
	},

	CU_TEST_INFO_NULL
};

void db_sqlite3_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Database Driver: sqlite3", NULL, NULL);
	while (db_sqlite3_tests[i].pName) {
		CU_add_test(suite, db_sqlite3_tests[i].pName,
		            db_sqlite3_tests[i].pTestFunc);
		i++;
	}
}

