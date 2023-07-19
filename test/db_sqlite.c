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

#include <check.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

#include "sqlite3_stubs.h"
#include "../src/db/sqlite3.c"

/**
 * Test that db_sqlite3_init() and db_sqlite3_uninit() work.
 */
START_TEST(test_sqlite3_init_uninit)
{
	ck_assert_int_eq(db_sqlite3_init(), 0);
	ck_assert_int_eq(db_sqlite3_uninit(), 0);

	sqlite3_initialize_returns = ~SQLITE_OK;
	sqlite3_shutdown_returns   = ~SQLITE_OK;

	ck_assert_int_eq(db_sqlite3_init(), 1);
	ck_assert_int_eq(db_sqlite3_uninit(), 1);
}
END_TEST

/**
 * Test that db_sqlite3_connect() returns NULL if db
 * is NULL.
 */
START_TEST(sqlite3_connect_null_db)
{
	sqlite3_open_dbh = (void *)1234;
	ck_assert_ptr_null(db_sqlite3_connect(NULL, 0, NULL, NULL, NULL));
}
END_TEST

/**
 * Test that db_sqlite3_connect() returns NULL if db
 * is an empty string.
 */
START_TEST(sqlite3_connect_empty_db)
{
	ck_assert_ptr_null(db_sqlite3_connect(NULL, 0, NULL, NULL, ""));
}
END_TEST

/**
 * Test that db_sqlite3_connect() handles error messages if
 * sqlite3_open() fails.
 */
START_TEST(sqlite3_connect_handles_error_messages)
{
	sqlite3_errmsg_returns = "test";
	sqlite3_open_returns   = ~SQLITE_OK;
	*errbuf = '\0';

	ck_assert_ptr_null(db_sqlite3_connect(NULL, 0, NULL, NULL, "test.db"));
	ck_assert_str_eq(errbuf, "[sqlite3_connect] test\n");
}
END_TEST

/**
 * Test that db_sqlite3_connect() works.
 */
START_TEST(test_sqlite3_connect)
{
	sqlite3_open_returns   = SQLITE_OK;
	sqlite3_open_dbh       = (void *)1234;
	sqlite3_errmsg_returns = NULL;

	ck_assert_ptr_eq(db_sqlite3_connect(NULL, 0, NULL, NULL, "test.db"),
	                 sqlite3_open_dbh);
}
END_TEST

/**
 * Test that db_sqlite3_query() returns 1 if the query fails.
 */
START_TEST(sqlite3_query_fails)
{
	sqlite3_exec_errmsg  = NULL;
	sqlite3_exec_returns = ~SQLITE_OK;
	ck_assert_int_eq(db_sqlite3_query(NULL, "test", NULL, NULL), 1);
}
END_TEST

/**
 * Test that db_sqlite3_query() correctly reports any
 * error message from sqlite.
 */
START_TEST(sqlite3_query_fails_with_error_message)
{
	char errmsg[] = "xxx";
	*errbuf = '\0';
	sqlite3_exec_errmsg  = errmsg;
	sqlite3_exec_returns = ~SQLITE_OK;
	ck_assert_int_eq(db_sqlite3_query(NULL, "test", NULL, NULL), 1);
	ck_assert_str_eq(errbuf, "query failed: xxx\n");
}
END_TEST

/**
 * Test that db_sqlite3_query() return 0 if the
 * callback causes SQLITE_ABORT to be returned.
 */
START_TEST(sqlite3_query_callback_abort)
{
	sqlite3_exec_errmsg  = NULL;
	sqlite3_exec_returns = SQLITE_ABORT;
	ck_assert_int_eq(db_sqlite3_query(NULL, "test", NULL, NULL), 0);
}
END_TEST

/**
 * Test that db_sqlite3_query() works.
 */
START_TEST(test_sqlite3_query)
{
	sqlite3_exec_errmsg  = NULL;
	sqlite3_exec_returns = SQLITE_OK;
	ck_assert_int_eq(db_sqlite3_query(NULL, "test", NULL, NULL), 0);
}
END_TEST

Suite *db_sqlite3_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("Database Driver: sqlite3");
	t = tcase_create("db_sqlite3_init");
	tcase_add_test(t, test_sqlite3_init_uninit);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_sqlite3_connect");
	tcase_add_test(t, sqlite3_connect_null_db);
	tcase_add_test(t, sqlite3_connect_empty_db);
	tcase_add_test(t, sqlite3_connect_handles_error_messages);
	tcase_add_test(t, test_sqlite3_connect);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_sqlite3_query");
	tcase_add_test(t, sqlite3_query_fails);
	tcase_add_test(t, sqlite3_query_fails_with_error_message);
	tcase_add_test(t, sqlite3_query_callback_abort);
	tcase_add_test(t, test_sqlite3_query);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

