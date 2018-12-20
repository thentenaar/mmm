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
#include <CUnit/CUnit.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

#include "libpq_stubs.h"
#include "../src/db/pgsql.c"

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
 * Test that db_pgsql_connect() handles invalid arguments.
 */
static void pgsql_connect_invalid_args(void)
{
	reset_libpq_stubs();
	PQconnectdb_returns = (PGconn *)1234;
	PQstatus_returns = CONNECTION_OK;

	CU_ASSERT_PTR_NULL(db_pgsql_connect("test", 1, "u", "p", NULL));
	CU_ASSERT_PTR_NULL(db_pgsql_connect(NULL, 1, "u", "p", "db"));
	CU_ASSERT_PTR_NULL(db_pgsql_connect("test", 1, NULL, "p", "db"));
	CU_ASSERT_PTR_NULL(db_pgsql_connect("test", 1, "u", NULL, "db"));
	CU_ASSERT_PTR_NULL(db_pgsql_connect("test", 1, "u", "p", ""));
	CU_ASSERT_PTR_NULL(db_pgsql_connect("", 1, "u", "p", "db"));
	CU_ASSERT_PTR_NULL(db_pgsql_connect("test", 1, "", "p", "db"));
	CU_ASSERT_PTR_NULL(db_pgsql_connect("test", 1, "u", "", "db"));
}

/**
 * Test that db_pgsql_connect() returns NULL if PQconnectdb()
 * returns NULL.
 */
static void pgsql_connect_null_dbh(void)
{
	reset_libpq_stubs();
	PQconnectdb_returns = NULL;
	PQstatus_returns = CONNECTION_OK;

	CU_ASSERT_PTR_NULL(db_pgsql_connect("test", 0, "u", "p", "db"));
	CU_ASSERT_FALSE(PQfinish_called);
}

/**
 * Test that db_pgsql_connect() handles error messages if
 * the database handle has a status other than CONNECTION_OK.
 */
static void pgsql_connect_handles_errors(void)
{
	reset_libpq_stubs();
	PQconnectdb_returns = (PGconn *)1234;
	PQstatus_returns = ~CONNECTION_OK;
	PQerrorMessage_returns = "xxx";
	errbuf[0] = '\0';

	CU_ASSERT_PTR_NULL(db_pgsql_connect("test", 0, "u", "p", "db"));
	CU_ASSERT_STRING_EQUAL(errbuf, "[pgsql_connect] xxx\n");
	CU_ASSERT_TRUE(PQfinish_called);
}

/**
 * Test that db_pgsql_connect() works.
 */
static void test_pgsql_connect(void)
{
	reset_libpq_stubs();
	PQconnectdb_returns = (PGconn *)1234;
	PQstatus_returns = CONNECTION_OK;
	errbuf[0] = '\0';

	CU_ASSERT_PTR_NOT_NULL(db_pgsql_connect("test", 0, "u", "p",
	                                        "db"));
	CU_ASSERT_EQUAL(errbuf[0], '\0');
	CU_ASSERT_FALSE(PQfinish_called);
}

/**
 * Test that db_pgsql_query() returns a non-zero value when
 * given invalid arguments.
 */
static void pgsql_query_invalid_args(void)
{
	PGconn *dbh = (PGconn *)1234;

	reset_libpq_stubs();
	CU_ASSERT_EQUAL(1, db_pgsql_query(NULL, "test", NULL, NULL));
	CU_ASSERT_EQUAL(1, db_pgsql_query(dbh, NULL, NULL, NULL));
	CU_ASSERT_EQUAL(1, db_pgsql_query(NULL, NULL, NULL, NULL));
	CU_ASSERT_FALSE(PQexec_called);
}

/**
 * Test that db_pgsql_query() returns a non-zero value when
 * PQexec() fails.
 */
static void pgsql_query_fails(void)
{
	PGconn *dbh = (PGconn *)1234;

	reset_libpq_stubs();
	PQexec_returns = 0;
	PQerrorMessage_returns = NULL;
	errbuf[0] = '\0';
	CU_ASSERT_EQUAL(1, db_pgsql_query(dbh, "test", NULL, NULL));
	CU_ASSERT_FALSE(PQclear_called);
	CU_ASSERT_EQUAL(errbuf[0], '\0');

	PQerrorMessage_returns = "xxx";
	CU_ASSERT_EQUAL(1, db_pgsql_query(dbh, "test", NULL, NULL));
	CU_ASSERT_FALSE(PQclear_called);
	CU_ASSERT_STRING_EQUAL(errbuf, "query failed: xxx\n");
}

/**
 * Test that db_pgsql_query() returns a non-zero value when
 * PQexec() returns a result with a status that isn't
 * PGRES_COMMAND_OK or PGRES_TUPLES_OK.
 */
static void pgsql_query_bad_status(void)
{
	PGconn *dbh = (PGconn *)1234;

	reset_libpq_stubs();
	PQexec_returns = 1;
	PQresultStatus_returns = ~(PGRES_COMMAND_OK | PGRES_TUPLES_OK);
	PQerrorMessage_returns = NULL;
	errbuf[0] = '\0';
	CU_ASSERT_EQUAL(1, db_pgsql_query(dbh, "test", NULL, NULL));
	CU_ASSERT_TRUE(PQclear_called);
	CU_ASSERT_EQUAL(errbuf[0], '\0');

	PQclear_called = 0;
	PQerrorMessage_returns = "xxx";
	CU_ASSERT_EQUAL(1, db_pgsql_query(dbh, "test", NULL, NULL));
	CU_ASSERT_TRUE(PQclear_called);
	CU_ASSERT_STRING_EQUAL(errbuf, "query failed: xxx\n");
}

/**
 * Test that db_pgsql_query() returns 0 if the
 * query executes but doesn't produce any result.
 */
static void pgsql_query_no_result(void)
{
	PGconn *dbh = (PGconn *)1234;

	reset_libpq_stubs();
	PQexec_returns = 1;
	PQresultStatus_returns = PGRES_COMMAND_OK;
	CU_ASSERT_EQUAL(0, db_pgsql_query(dbh, "test", NULL, NULL));
	CU_ASSERT_TRUE(PQclear_called);
}

/**
 * Test that db_pgsql_query() returns 0 if the
 * query executes and produces a result which
 * doesn't contain any columns.
 */
static void pgsql_query_no_cols(void)
{
	PGconn *dbh = (PGconn *)1234;

	reset_libpq_stubs();
	PQexec_returns = 1;
	PQresultStatus_returns = PGRES_TUPLES_OK;
	PQntuples_returns = 0;
	PQnfields_returns = 1;
	CU_ASSERT_EQUAL(0, db_pgsql_query(dbh, "test", row_cb, NULL));
	CU_ASSERT_TRUE(PQclear_called);
	CU_ASSERT_FALSE(PQfname_called || PQgetvalue_called);

	PQntuples_returns = -1;
	PQfname_called = 0;
	PQgetvalue_called = 0;
	PQclear_called = 0;
	CU_ASSERT_EQUAL(0, db_pgsql_query(dbh, "test", row_cb, NULL));
	CU_ASSERT_TRUE(PQclear_called);
	CU_ASSERT_FALSE(PQfname_called || PQgetvalue_called);
}

/**
 * Test that db_pgsql_query() returns 0 if the
 * query executes and produces a result which
 * doesn't contain any rows.
 */
static void pgsql_query_no_rows(void)
{
	PGconn *dbh = (PGconn *)1234;

	reset_libpq_stubs();
	PQexec_returns = 1;
	PQresultStatus_returns = PGRES_TUPLES_OK;
	PQntuples_returns = 1;
	PQnfields_returns = 0;
	CU_ASSERT_EQUAL(0, db_pgsql_query(dbh, "test", row_cb, NULL));
	CU_ASSERT_TRUE(PQclear_called);
	CU_ASSERT_FALSE(PQfname_called || PQgetvalue_called);

	PQnfields_returns = -1;
	PQfname_called = 0;
	PQgetvalue_called = 0;
	PQclear_called = 0;
	CU_ASSERT_EQUAL(0, db_pgsql_query(dbh, "test", row_cb, NULL));
	CU_ASSERT_TRUE(PQclear_called);
	CU_ASSERT_FALSE(PQfname_called || PQgetvalue_called);
}

/**
 * Test that db_pgsql_query() returns 0 if the
 * query executes and produces a result which
 * doesn't contain any rows.
 */
static void pgsql_query_no_cb(void)
{
	PGconn *dbh = (PGconn *)1234;

	reset_libpq_stubs();
	PQexec_returns = 1;
	PQresultStatus_returns = PGRES_TUPLES_OK;
	PQntuples_returns = 1;
	PQnfields_returns = 1;
	CU_ASSERT_EQUAL(0, db_pgsql_query(dbh, "test", NULL, NULL));
	CU_ASSERT_TRUE(PQclear_called);
	CU_ASSERT_FALSE(PQfname_called || PQgetvalue_called);
}

/**
 * Test that db_pgsql_query() handles 1 row
 * with a NULL field.
 */
static void pgsql_query_1_row_null_field(void)
{
	PGconn *dbh = (PGconn *)1234;
	void *ud = (void *)1;

	reset_libpq_stubs();
	PQexec_returns = 1;
	PQresultStatus_returns = PGRES_TUPLES_OK;
	PQntuples_returns = 1;
	PQnfields_returns = 1;
	PQfname_returns = "col";
	PQgetisnull_returns = 1;
	row_cb_called = 0;
	row_cb_returns = 0;

	CU_ASSERT_EQUAL(0, db_pgsql_query(dbh, "test", row_cb, ud));
	CU_ASSERT_TRUE(PQclear_called);
	CU_ASSERT_TRUE(PQfname_called && PQgetisnull_called);
	CU_ASSERT_FALSE(PQgetvalue_called);
	CU_ASSERT_TRUE(row_cb_called);
}

/**
 * Test that db_pgsql_query() works.
 */
static void test_pgsql_query(void)
{
	PGconn *dbh = (PGconn *)1234;
	void *ud = (void *)2;

	reset_libpq_stubs();
	PQexec_returns = 1;
	PQresultStatus_returns = PGRES_TUPLES_OK;
	PQntuples_returns = 1;
	PQnfields_returns = 1;
	PQfname_returns = "col";
	PQgetisnull_returns = 0;
	PQgetvalue_returns = "value";
	row_cb_called = 0;
	row_cb_returns = 1;

	CU_ASSERT_EQUAL(0, db_pgsql_query(dbh, "test", row_cb, ud));
	CU_ASSERT_TRUE(PQclear_called);
	CU_ASSERT_TRUE(PQfname_called && PQgetvalue_called);
	CU_ASSERT_TRUE(row_cb_called);
}

/**
 * Test that db_pgsql_disconnect() calls PQfinish() if
 * dbh is not NULL.
 */
static void test_pgsql_disconnect(void)
{
	reset_libpq_stubs();
	db_pgsql_disconnect(NULL);
	CU_ASSERT_EQUAL(PQfinish_called, 0);
	db_pgsql_disconnect((void *)1234);
	CU_ASSERT_EQUAL(PQfinish_called, 1);
}

static CU_TestInfo db_pgsql_tests[] = {
	{
		"db_pgsql_connect() - invalid args",
		pgsql_connect_invalid_args
	},
	{
		"db_pgsql_connect() - null dbh",
		pgsql_connect_null_dbh
	},
	{
		"db_pgsql_connect() - handles errors",
		pgsql_connect_handles_errors
	},
	{
		"db_pgsql_connect() - works",
		test_pgsql_connect
	},
	{
		"db_pgsql_query() - invalid args",
		pgsql_query_invalid_args
	},
	{
		"db_pgsql_query() - query fails",
		pgsql_query_fails
	},
	{
		"db_pgsql_query() - succeds / bad status",
		pgsql_query_bad_status
	},
	{
		"db_pgsql_query() - succeds / no result",
		pgsql_query_no_result
	},
	{
		"db_pgsql_query() - succeds / no cols",
		pgsql_query_no_cols
	},
	{
		"db_pgsql_query() - succeds / no rows",
		pgsql_query_no_rows
	},
	{
		"db_pgsql_query() - succeds / no cb",
		pgsql_query_no_cb
	},
	{
		"db_pgsql_query() - 1 row / null field",
		pgsql_query_1_row_null_field
	},
	{
		"db_pgsql_query() - works",
		test_pgsql_query
	},
	{
		"db_pgsql_disconnect() - works",
		test_pgsql_disconnect
	},

	CU_TEST_INFO_NULL
};

void db_pgsql_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Database Driver: pgsql", NULL, NULL);
	while (db_pgsql_tests[i].pName) {
		CU_add_test(suite, db_pgsql_tests[i].pName,
		            db_pgsql_tests[i].pTestFunc);
		i++;
	}
}

