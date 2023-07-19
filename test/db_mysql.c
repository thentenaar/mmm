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

#include <check.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

#include "mysql_stubs.h"
#include "../src/db/mysql.c"

static int row_cb_returns = 1;
static int row_cb_called  = 0;

static int row_cb(void *userdata, int n_cols, char **fields,
                  char **column_names)
{
	++row_cb_called;

	if (userdata == (void *)1 && n_cols == 1) {
		ck_assert_str_eq(column_names[0], "col");
		ck_assert_ptr_null(fields[0]);
	}

	if (userdata == (void *)2 && n_cols == 1) {
		ck_assert_str_eq(column_names[0], "col");
		ck_assert_str_eq(fields[0], "value");
	}

	return row_cb_returns;
}

/**
 * Test that db_mysql_init() and db_mysql_uninit()
 * work.
 */
START_TEST(test_mysql_init_uninit)
{
	ck_assert_int_eq(db_mysql_init(), 0);
	ck_assert_int_eq(db_mysql_uninit(), 0);
}
END_TEST

/**
 * Test that db_mysql_connect() returns NULL if given invalid
 * arguments.
 */
START_TEST(mysql_connect_invalid_args)
{
	ck_assert_ptr_null(db_mysql_connect("test", 0, NULL, "p", "db"));
	ck_assert_ptr_null(db_mysql_connect("test", 0, "u", NULL, "db"));
	ck_assert_ptr_null(db_mysql_connect("test", 0, "u", "p", NULL));
	ck_assert(!mysql_init_called);
}
END_TEST

/**
 * Test that db_mysql_connect() returns NULL if mysql_init()
 * fails.
 */
START_TEST(mysql_connect_init_fails)
{
	mysql_init_returns = NULL;
	ck_assert_ptr_null(db_mysql_connect("test", 0, "u", "p", "db"));
	ck_assert(mysql_init_called && !mysql_options_called);
}
END_TEST

/**
 * Test that db_mysql_connect() returns NULL and handles
 * errors appropriately when connecting to a host fails.
 */
START_TEST(mysql_connect_fails_host)
{
	char errmsg[]              = "xxx";
	mysql_init_returns         = (MYSQL *)1234;
	mysql_real_connect_returns = NULL;
	mysql_error_returns        = errmsg;
	*errbuf = '\0';

	ck_assert_ptr_null(db_mysql_connect("test", 0, "u", "p", "db"));
	ck_assert_str_eq(errbuf, "[mysql_connect] xxx\n");
	ck_assert(mysql_init_called && mysql_options_called);
	ck_assert(mysql_real_connect_called);
	ck_assert(mysql_error_called && mysql_close_called);
}
END_TEST

/**
 * Test that db_mysql_connect() returns NULL and handles
 * errors appropriately when connecting to a unix socket fails.
 */
START_TEST(mysql_connect_fails_unix_socket)
{
	char errmsg[]              = "xxx";
	mysql_init_returns         = (MYSQL *)1234;
	mysql_real_connect_returns = NULL;
	mysql_error_returns        = errmsg;
	*errbuf = '\0';

	ck_assert_ptr_null(db_mysql_connect("/tst", 0, "u", "p", "db"));
	ck_assert_str_eq(errbuf, "[mysql_connect] xxx\n");
	ck_assert(mysql_init_called && mysql_options_called);
	ck_assert(mysql_real_connect_called);
	ck_assert(mysql_error_called && mysql_close_called);
}
END_TEST

/**
 * Test that db_mysql_connect() returns NULL and handles
 * errors appropriately when connecting to the default host fails.
 */
START_TEST(mysql_connect_fails_default_host)
{
	char errmsg[]              = "xxx";
	mysql_init_returns         = (MYSQL *)1234;
	mysql_real_connect_returns = NULL;
	mysql_error_returns        = errmsg;
	*errbuf = '\0';

	ck_assert_ptr_null(db_mysql_connect(NULL, 0, "u", "p", "db"));
	ck_assert_str_eq(errbuf, "[mysql_connect] xxx\n");
	ck_assert(mysql_init_called && mysql_options_called);
	ck_assert(mysql_real_connect_called);
	ck_assert(mysql_error_called && mysql_close_called);
}
END_TEST

/**
 * Test that db_mysql_connect() works.
 */
START_TEST(test_mysql_connect)
{
	mysql_init_returns         = (MYSQL *)1234;
	mysql_real_connect_returns = mysql_init_returns;

	ck_assert_ptr_nonnull(db_mysql_connect(NULL, 0, "u", "p", "db"));
	ck_assert(mysql_init_called && mysql_options_called);
	ck_assert(mysql_real_connect_called);
	ck_assert(!mysql_error_called && !mysql_close_called);
}
END_TEST

/**
 * Test that db_mysql_query() returns 1 if given
 * invalid args.
 */
START_TEST(mysql_query_invalid_args)
{
	MYSQL *dbh = (MYSQL *)1234;

	row_cb_called = 0;
	ck_assert_int_eq(db_mysql_query(NULL, "test", row_cb, NULL), 1);
	ck_assert_int_eq(db_mysql_query(dbh, NULL, row_cb, NULL), 1);
	ck_assert(!row_cb_called && !mysql_real_query_called);
}
END_TEST

/**
 * Test that db_mysql_query() returns 1 when mysql_real_query() fails.
 */
START_TEST(mysql_query_fails)
{
	MYSQL *dbh = (MYSQL *)1234;

	mysql_real_query_returns = 1;
	mysql_error_returns      = NULL;
	ck_assert_int_eq(db_mysql_query(dbh, "test", row_cb, NULL), 1);
	ck_assert(mysql_real_query_called && mysql_error_called);
}
END_TEST

/**
 * Test that db_mysql_query() handles error messages when
 * mysql_real_query() fails.
 */
START_TEST(mysql_query_fails_errmsg)
{
	char errmsg[] = "xxx";
	MYSQL *dbh = (MYSQL *)1234;

	*errbuf = '\0';
	mysql_real_query_returns = 1;
	mysql_error_returns      = errmsg;
	ck_assert_int_eq(db_mysql_query(dbh, "test", row_cb, NULL), 1);
	ck_assert_str_eq(errbuf, "query failed: xxx\n");
	ck_assert(mysql_real_query_called && mysql_error_called);
}
END_TEST

/**
 * Test that db_mysql_query() returns 0 if no results were
 * returned.
 */
START_TEST(mysql_query_no_results)
{
	MYSQL *dbh = (MYSQL *)1234;

	mysql_real_query_returns   = 0;
	mysql_store_result_returns = NULL;
	mysql_next_result_returns  = -1;
	ck_assert_int_eq(db_mysql_query(dbh, "test", row_cb, NULL), 0);
	ck_assert(mysql_real_query_called  && mysql_store_result_called);
	ck_assert(mysql_next_result_called && !mysql_num_rows_called);
}
END_TEST

/**
 * Test that db_mysql_query() correctly handles the case when
 * no rows are indicated.
 */
START_TEST(mysql_query_no_rows)
{
	MYSQL *dbh = (MYSQL *)1234;

	mysql_real_query_returns   = 0;
	mysql_store_result_returns = (MYSQL_RES *)1234;
	mysql_num_rows_returns     = 0;
	mysql_num_fields_returns   = 1;
	mysql_next_result_returns  = -1;

	ck_assert_int_eq(db_mysql_query(dbh, "test", row_cb, NULL), 0);
	ck_assert(mysql_real_query_called  && mysql_store_result_called);
	ck_assert(mysql_num_rows_called    && mysql_num_fields_called);
	ck_assert(mysql_next_result_called && !mysql_fetch_fields_called);
}
END_TEST

/**
 * Test that db_mysql_query() correctly handles the case when
 * no cols are indicated.
 */
START_TEST(mysql_query_no_cols)
{
	MYSQL *dbh = (MYSQL *)1234;

	mysql_real_query_returns   = 0;
	mysql_store_result_returns = (MYSQL_RES *)1234;
	mysql_num_rows_returns     = 1;
	mysql_num_fields_returns   = 0;
	mysql_next_result_returns  = -1;

	ck_assert_int_eq(db_mysql_query(dbh, "test", row_cb, NULL), 0);
	ck_assert(mysql_real_query_called  && mysql_store_result_called);
	ck_assert(mysql_num_rows_called    && mysql_num_fields_called);
	ck_assert(mysql_next_result_called && mysql_free_result_called);
	ck_assert(!mysql_fetch_fields_called);
}
END_TEST

/**
 * Test that db_mysql_query() correctly handles the case when
 * no callback is specified.
 */
START_TEST(mysql_query_no_cb)
{
	MYSQL *dbh = (MYSQL *)1234;

	mysql_real_query_returns   = 0;
	mysql_store_result_returns = (MYSQL_RES *)1234;
	mysql_num_rows_returns     = 1;
	mysql_num_fields_returns   = 1;
	mysql_next_result_returns  = -1;

	ck_assert_int_eq(db_mysql_query(dbh, "test", NULL, NULL), 0);
	ck_assert(mysql_real_query_called  && mysql_store_result_called);
	ck_assert(mysql_num_rows_called    && mysql_num_fields_called);
	ck_assert(mysql_next_result_called && mysql_free_result_called);
	ck_assert(!mysql_fetch_fields_called);
}
END_TEST

/**
 * Test that db_mysql_query() correctly handles the case when
 * no fields are returned.
 */
START_TEST(mysql_query_no_fields)
{
	MYSQL *dbh = (MYSQL *)1234;

	mysql_real_query_returns   = 0;
	mysql_store_result_returns = (MYSQL_RES *)1234;
	mysql_num_rows_returns     = 1;
	mysql_num_fields_returns   = 1;
	mysql_fetch_fields_returns = NULL;
	mysql_next_result_returns  = -1;

	ck_assert_int_eq(db_mysql_query(dbh, "test", row_cb, NULL), 0);
	ck_assert(mysql_real_query_called   && mysql_store_result_called);
	ck_assert(mysql_num_rows_called     && mysql_num_fields_called);
	ck_assert(mysql_fetch_fields_called && !mysql_fetch_row_called);
	ck_assert(mysql_next_result_called  && mysql_free_result_called);
}
END_TEST

/**
 * Test that db_mysql_query() works.
 */
START_TEST(test_mysql_query)
{
	char *row[1];
	char col[] = "col";
	char val[] = "val";
	MYSQL *dbh = (MYSQL *)1234;
	MYSQL_FIELD fields[1];

	fields[0].name = col;
	row[0]         = val;
	mysql_real_query_returns   = 0;
	mysql_store_result_returns = (MYSQL_RES *)1234;
	mysql_num_rows_returns     = 1;
	mysql_num_fields_returns   = 1;
	mysql_fetch_fields_returns = fields;
	mysql_fetch_row_returns    = (MYSQL_ROW)&row;
	mysql_next_result_returns  = -1;

	ck_assert_int_eq(db_mysql_query(dbh, "test", row_cb, NULL), 0);
	ck_assert(mysql_real_query_called   && mysql_store_result_called);
	ck_assert(mysql_num_rows_called     && mysql_num_fields_called);
	ck_assert(mysql_fetch_fields_called && mysql_fetch_row_called);
	ck_assert(mysql_next_result_called  && mysql_free_result_called);
}
END_TEST

/**
 * Test that db_mysql_disconnect() works.
 */
START_TEST(test_mysql_disconnect)
{
	db_mysql_disconnect(NULL);
	ck_assert(!mysql_close_called);
	db_mysql_disconnect((void *)1234);
	ck_assert(mysql_close_called);
}
END_TEST

Suite *db_mysql_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("Database Driver: mysql");
	t = tcase_create("db_mysql_init");
	tcase_add_checked_fixture(t, reset_mysql_stubs, NULL);
	tcase_add_test(t, test_mysql_init_uninit);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_mysql_connect");
	tcase_add_checked_fixture(t, reset_mysql_stubs, NULL);
	tcase_add_test(t, mysql_connect_invalid_args);
	tcase_add_test(t, mysql_connect_init_fails);
	tcase_add_test(t, mysql_connect_fails_host);
	tcase_add_test(t, mysql_connect_fails_unix_socket);
	tcase_add_test(t, mysql_connect_fails_default_host);
	tcase_add_test(t, test_mysql_connect);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_mysql_query");
	tcase_add_checked_fixture(t, reset_mysql_stubs, NULL);
	tcase_add_test(t, mysql_query_invalid_args);
	tcase_add_test(t, mysql_query_fails);
	tcase_add_test(t, mysql_query_fails_errmsg);
	tcase_add_test(t, mysql_query_no_results);
	tcase_add_test(t, mysql_query_no_rows);
	tcase_add_test(t, mysql_query_no_cols);
	tcase_add_test(t, mysql_query_no_cb);
	tcase_add_test(t, mysql_query_no_fields);
	tcase_add_test(t, test_mysql_query);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_mysql_disconnect");
	tcase_add_checked_fixture(t, reset_mysql_stubs, NULL);
	tcase_add_test(t, test_mysql_disconnect);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

