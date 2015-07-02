/**
 * Minimal Migration Manager - libpq stubs for tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef TEST_LIBPQ_STUBS_H
#define TEST_LIBPQ_STUBS_H

#include "../src/db.h"

/* {{{ GCC: Disable warnings
 *
 * The parameters in the functions below are intentionally unused,
 * and not all functions may be used in the translation unit including
 * this file. Thus, we want GCC to see this as a system header and
 * not complain about unused functions and the like.
 */
#if defined(__GNUC__) && __GNUC__ >= 3
#pragma GCC system_header
#endif /* GCC >= 3 }}} */

/* {{{ libpq stub return values */

#define CONNECTION_OK 1
#define PGRES_COMMAND_OK 2
#define PGRES_TUPLES_OK 3

typedef int PGconn;
typedef int PGresult;

static PGconn *PQconnectdb_returns = NULL;
static int PQstatus_returns = 0;
static char *PQerrorMessage_returns = NULL;
static int PQexec_returns = 0;
static int PQresultStatus_returns = 0;
static int PQntuples_returns = 0;
static int PQnfields_returns = 0;
static char *PQfname_returns = NULL;
static char *PQgetvalue_returns = NULL;
static int PQgetisnull_returns = 0;

/* call counters */
static int PQconnectdb_called = 0;
static int PQstatus_called = 0;
static int PQerrorMessage_called = 0;
static int PQexec_called = 0;
static int PQresultStatus_called = 0;
static int PQclear_called = 0;
static int PQntuples_called = 0;
static int PQnfields_called = 0;
static int PQfname_called = 0;
static int PQgetvalue_called = 0;
static int PQgetisnull_called = 0;
static int PQfinish_called = 0;

static void reset_libpq_stubs(void)
{
	PQconnectdb_returns = NULL;
	PQstatus_returns = 0;
	PQerrorMessage_returns = NULL;
	PQexec_returns = 0;
	PQresultStatus_returns = 0;
	PQntuples_returns = 0;
	PQnfields_returns = 0;
	PQfname_returns = NULL;
	PQgetvalue_returns = NULL;
	PQgetisnull_returns = 0;
	PQconnectdb_called = 0;
	PQstatus_called = 0;
	PQerrorMessage_called = 0;
	PQexec_called = 0;
	PQresultStatus_called = 0;
	PQclear_called = 0;
	PQntuples_called = 0;
	PQnfields_called = 0;
	PQfname_called = 0;
	PQgetvalue_called = 0;
	PQgetisnull_called = 0;
	PQfinish_called = 0;
}
/* }}} */

/* {{{ libpq stubs */
static PGconn *PQconnectdb(const char *s)
{
	++PQconnectdb_called;
	return PQconnectdb_returns;
}

static int PQstatus(PGconn *conn)
{
	++PQstatus_called;
	return PQstatus_returns;
}

static char *PQerrorMessage(PGconn *conn)
{
	++PQerrorMessage_called;
	return PQerrorMessage_returns;
}

static PGresult *PQexec(PGconn *conn, const char *query)
{
	++PQexec_called;
	return PQexec_returns;
}

static int PQresultStatus(PGresult *res)
{
	++PQresultStatus_called;
	return PQresultStatus_returns;
}

static void PQclear(PGresult *res)
{
	++PQclear_called;
	return;
}

static int PQntuples(PGresult *res)
{
	++PQntuples_called;
	return PQntuples_returns;
}

static int PQnfields(PGresult *res)
{
	++PQnfields_called;
	return PQnfields_returns;
}

static char *PQfname(PGresult *res, int col)
{
	++PQfname_called;
	return PQfname_returns;
}

static char *PQgetvalue(PGresult *res, int row, int col)
{
	++PQgetvalue_called;
	return PQgetvalue_returns;
}

static char *PQgetisnull(PGresult *res, int row, int col)
{
	++PQgetisnull_called;
	return PQgetisnull_returns;
}

static void PQfinish(PGconn *conn)
{
	++PQfinish_called;
}
/* }}} */

#endif /* TEST_LIBPQ_STUBS_H */
