/**
 * Minimal Migration Manager - PostgreSQL Database Driver Tests
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

#include "libpq_stubs.h"
#include "../src/db/pgsql.c"

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
 * Test that db_pgsql_connect() returns NULL on invalid arguments.
 */
START_TEST(pgsql_connect_invalid_args)
{
	PQconnectdb_returns = (PGconn *)1234;
	PQstatus_returns    = CONNECTION_OK;

	ck_assert_ptr_null(db_pgsql_connect("test", 1, "u", "p", NULL));
	ck_assert_ptr_null(db_pgsql_connect(NULL, 1, "u", "p", "db"));
	ck_assert_ptr_null(db_pgsql_connect("test", 1, NULL, "p", "db"));
	ck_assert_ptr_null(db_pgsql_connect("test", 1, "u", NULL, "db"));
	ck_assert_ptr_null(db_pgsql_connect("test", 1, "u", "p", ""));
	ck_assert_ptr_null(db_pgsql_connect("", 1, "u", "p", "db"));
	ck_assert_ptr_null(db_pgsql_connect("test", 1, "", "p", "db"));
	ck_assert_ptr_null(db_pgsql_connect("test", 1, "u", "", "db"));
}
END_TEST

/**
 * Test that db_pgsql_connect() returns NULL if PQconnectdb()
 * returns NULL.
 */
START_TEST(pgsql_connect_null_dbh)
{
	PQconnectdb_returns = NULL;
	PQstatus_returns    = CONNECTION_OK;

	ck_assert_ptr_null(db_pgsql_connect("test", 0, "u", "p", "db"));
	ck_assert(!PQfinish_called);
}
END_TEST

/**
 * Test that db_pgsql_connect() handles error messages if
 * the database handle has a status other than CONNECTION_OK.
 */
START_TEST(pgsql_connect_handles_errors)
{
	char errmsg[]          = "xxx";
	PQconnectdb_returns    = (PGconn *)1234;
	PQstatus_returns       = ~CONNECTION_OK;
	PQerrorMessage_returns = errmsg;
	*errbuf = '\0';

	ck_assert_ptr_null(db_pgsql_connect("test", 0, "u", "p", "db"));
	ck_assert_str_eq(errbuf, "[pgsql_connect] xxx\n");
	ck_assert(PQfinish_called);
}
END_TEST

/**
 * Test that db_pgsql_connect() works.
 */
START_TEST(test_pgsql_connect)
{
	PQconnectdb_returns = (PGconn *)1234;
	PQstatus_returns    = CONNECTION_OK;
	*errbuf = '\0';

	ck_assert_ptr_nonnull(db_pgsql_connect("test", 0, "u", "p", "db"));
	ck_assert(!*errbuf && !PQfinish_called);
}
END_TEST

/**
 * Test that db_pgsql_query() returns a non-zero value when
 * given invalid arguments.
 */
START_TEST(pgsql_query_invalid_args)
{
	PGconn *dbh = (PGconn *)1234;

	ck_assert_int_ne(db_pgsql_query(NULL, "test", NULL, NULL), 0);
	ck_assert_int_ne(db_pgsql_query(dbh, NULL, NULL, NULL), 0);
	ck_assert_int_ne(db_pgsql_query(NULL, NULL, NULL, NULL), 0);
	ck_assert(!PQexec_called);
}
END_TEST

/**
 * Test that db_pgsql_query() returns a non-zero value when
 * PQexec() fails.
 */
START_TEST(pgsql_query_fails)
{
	char errmsg[] = "xxx";
	PGconn *dbh = (PGconn *)1234;

	PQexec_returns         = 0;
	PQerrorMessage_returns = NULL;
	*errbuf = '\0';

	ck_assert_int_ne(db_pgsql_query(dbh, "test", NULL, NULL), 0);
	ck_assert(!*errbuf && !PQclear_called);

	PQerrorMessage_returns = errmsg;
	ck_assert_int_ne(db_pgsql_query(dbh, "test", NULL, NULL), 0);
	ck_assert_str_eq(errbuf, "query failed: xxx\n");
	ck_assert(!PQclear_called);
}
END_TEST

/**
 * Test that db_pgsql_query() returns a non-zero value when
 * PQexec() returns a result with a status that isn't
 * PGRES_COMMAND_OK or PGRES_TUPLES_OK.
 */
START_TEST(pgsql_query_bad_status)
{
	char errmsg[] = "xxx";
	PGconn *dbh = (PGconn *)1234;

	PQexec_returns         = 1;
	PQresultStatus_returns = ~(PGRES_COMMAND_OK | PGRES_TUPLES_OK);
	PQerrorMessage_returns = NULL;
	*errbuf = '\0';

	ck_assert_int_ne(db_pgsql_query(dbh, "test", NULL, NULL), 0);
	ck_assert(!*errbuf && PQclear_called);

	PQclear_called = 0;
	PQerrorMessage_returns = errmsg;
	ck_assert_int_ne(db_pgsql_query(dbh, "test", NULL, NULL), 0);
	ck_assert_str_eq(errbuf, "query failed: xxx\n");
	ck_assert(PQclear_called);
}
END_TEST

/**
 * Test that db_pgsql_query() returns 0 if the
 * query executes but doesn't produce any result.
 */
START_TEST(pgsql_query_no_result)
{
	PGconn *dbh = (PGconn *)1234;

	PQexec_returns         = 1;
	PQresultStatus_returns = PGRES_COMMAND_OK;
	ck_assert_int_eq(db_pgsql_query(dbh, "test", NULL, NULL), 0);
	ck_assert(PQclear_called);
}
END_TEST

/**
 * Test that db_pgsql_query() returns 0 if the
 * query executes and produces a result which
 * doesn't contain any columns.
 */
START_TEST(pgsql_query_no_cols)
{
	PGconn *dbh = (PGconn *)1234;

	PQntuples_returns      = 0;
	PQnfields_returns      = 1;
	PQexec_returns         = 1;
	PQresultStatus_returns = PGRES_TUPLES_OK;
	ck_assert_int_eq(db_pgsql_query(dbh, "test", row_cb, NULL), 0);
	ck_assert(PQclear_called && !(PQfname_called || PQgetvalue_called));

	PQntuples_returns = -1;
	PQfname_called    = 0;
	PQgetvalue_called = 0;
	PQclear_called    = 0;
	ck_assert_int_eq(db_pgsql_query(dbh, "test", row_cb, NULL), 0);
	ck_assert(PQclear_called && !(PQfname_called || PQgetvalue_called));
}
END_TEST

/**
 * Test that db_pgsql_query() returns 0 if the
 * query executes and produces a result which
 * doesn't contain any rows.
 */
START_TEST(pgsql_query_no_rows)
{
	PGconn *dbh = (PGconn *)1234;

	PQntuples_returns      = 1;
	PQnfields_returns      = 0;
	PQexec_returns         = 1;
	PQresultStatus_returns = PGRES_TUPLES_OK;
	ck_assert_int_eq(db_pgsql_query(dbh, "test", row_cb, NULL), 0);
	ck_assert(PQclear_called && !(PQfname_called || PQgetvalue_called));

	PQnfields_returns = -1;
	PQfname_called    = 0;
	PQgetvalue_called = 0;
	PQclear_called    = 0;
	ck_assert_int_eq(db_pgsql_query(dbh, "test", row_cb, NULL), 0);
	ck_assert(PQclear_called && !(PQfname_called || PQgetvalue_called));
}
END_TEST

/**
 * Test that db_pgsql_query() returns 0 if the
 * query executes and produces a result which
 * doesn't contain any rows.
 */
START_TEST(pgsql_query_no_cb)
{
	PGconn *dbh = (PGconn *)1234;

	PQntuples_returns      = 1;
	PQnfields_returns      = 1;
	PQexec_returns         = 1;
	PQresultStatus_returns = PGRES_TUPLES_OK;
	ck_assert_int_eq(db_pgsql_query(dbh, "test", NULL, NULL), 0);
	ck_assert(PQclear_called && !(PQfname_called || PQgetvalue_called));
}
END_TEST

/**
 * Test that db_pgsql_query() handles 1 row
 * with a NULL field.
 */
START_TEST(pgsql_query_one_row_null_field)
{
	PGconn *dbh = (PGconn *)1234;
	void *ud = (void *)1;
	char fname[] = "col";

	PQntuples_returns      = 1;
	PQnfields_returns      = 1;
	PQexec_returns         = 1;
	PQresultStatus_returns = PGRES_TUPLES_OK;
	PQfname_returns        = fname;
	PQgetisnull_returns    = 1;
	row_cb_called          = 0;
	row_cb_returns         = 0;
	ck_assert_int_eq(db_pgsql_query(dbh, "test", row_cb, ud), 0);
	ck_assert(PQclear_called && PQfname_called && PQgetisnull_called);
	ck_assert(!PQgetvalue_called);
	ck_assert(row_cb_called);
}
END_TEST

/**
 * Test that db_pgsql_query() works.
 */
START_TEST(test_pgsql_query)
{
	PGconn *dbh = (PGconn *)1234;
	void *ud = (void *)2;
	char fname[] = "col";
	char value[] = "value";

	PQntuples_returns      = 1;
	PQnfields_returns      = 1;
	PQexec_returns         = 1;
	PQresultStatus_returns = PGRES_TUPLES_OK;
	PQfname_returns        = fname;
	PQgetisnull_returns    = 0;
	PQgetvalue_returns     = value;
	row_cb_called          = 0;
	row_cb_returns         = 1;
	ck_assert_int_eq(db_pgsql_query(dbh, "test", row_cb, ud), 0);
	ck_assert(PQclear_called && PQfname_called && PQgetvalue_called);
	ck_assert(row_cb_called);
}
END_TEST

/**
 * Test that db_pgsql_disconnect() calls PQfinish() if
 * dbh is not NULL.
 */
START_TEST(test_pgsql_disconnect)
{
	db_pgsql_disconnect(NULL);
	ck_assert(!PQfinish_called);
	db_pgsql_disconnect((void *)1234);
	ck_assert(PQfinish_called);
}
END_TEST

Suite *db_pgsql_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("Database Driver: pgsql");
	t = tcase_create("db_pgsql_connect");
	tcase_add_checked_fixture(t, reset_libpq_stubs, NULL);
	tcase_add_test(t, pgsql_connect_invalid_args);
	tcase_add_test(t, pgsql_connect_null_dbh);
	tcase_add_test(t, pgsql_connect_handles_errors);
	tcase_add_test(t, test_pgsql_connect);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_pgsql_query");
	tcase_add_checked_fixture(t, reset_libpq_stubs, NULL);
	tcase_add_test(t, pgsql_query_invalid_args);
	tcase_add_test(t, pgsql_query_fails);
	tcase_add_test(t, pgsql_query_bad_status);
	tcase_add_test(t, pgsql_query_no_result);
	tcase_add_test(t, pgsql_query_no_cols);
	tcase_add_test(t, pgsql_query_no_rows);
	tcase_add_test(t, pgsql_query_no_cb);
	tcase_add_test(t, pgsql_query_one_row_null_field);
	tcase_add_test(t, test_pgsql_query);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("db_pgsql_disconnect");
	tcase_add_checked_fixture(t, reset_libpq_stubs, NULL);
	tcase_add_test(t, test_pgsql_disconnect);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

