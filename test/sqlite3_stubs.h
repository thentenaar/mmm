/**
 * Minimal Migration Manager - sqlite3 stubs for tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef TEST_SQLITE3_STUBS_H
#define TEST_SQLITE3_STUBS_H

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

/* {{{ sqlite3 stub return values */

#define SQLITE_OK 1
#define SQLITE_ABORT 2

typedef int sqlite3;

static sqlite3 *sqlite3_open_dbh = NULL;
static int sqlite3_initialize_returns = SQLITE_OK;
static int sqlite3_shutdown_returns = SQLITE_OK;
static int sqlite3_open_returns = SQLITE_OK;
static int sqlite3_exec_returns = SQLITE_OK;
static const char *sqlite3_errmsg_returns = NULL;
static char *sqlite3_exec_errmsg = NULL;

/* }}} */

/* {{{ sqlite3 stubs */

static int sqlite3_initialize(void)
{
	return sqlite3_initialize_returns;
}

static int sqlite3_shutdown(void)
{
	return sqlite3_shutdown_returns;
}

static int sqlite3_open(const char *db, sqlite3 **dbh)
{
	*dbh = sqlite3_open_dbh;
	return sqlite3_open_returns;
}

static const char *sqlite3_errmsg(sqlite3 *dbh)
{
	return sqlite3_errmsg_returns;
}

static int sqlite3_exec(sqlite3 *dbh, const char *query,
                        db_row_callback_t callback, void *userdata,
                        char **errmsg)
{
	*errmsg = sqlite3_exec_errmsg;
	return sqlite3_exec_returns;
}

static void sqlite3_close(sqlite3 *dbh)
{
	return;
}

static void sqlite3_free(void *x)
{
	return;
}

/* }}} */

#endif /* TEST_SQLITE3_STUBS_H */
