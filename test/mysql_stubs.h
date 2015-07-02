/**
 * Minimal Migration Manager - mysql stubs for tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef TEST_MYSQL_STUBS_H
#define TEST_MYSQL_STUBS_H

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

/* {{{ mysql stub return values */

#define MYSQL_READ_DEFAULT_GROUP 1
#define MYSQL_SET_CHARSET_NAME 2
#define MYSQL_OPT_RECONNECT 3
#define CLIENT_COMPRESS 4
#define CLIENT_MULTI_STATEMENTS 8

typedef struct mysql_field {
	char *name;
} MYSQL_FIELD;

typedef int my_bool;
typedef int MYSQL;
typedef int MYSQL_RES;
typedef char ** MYSQL_ROW;

static int mysql_library_init_returns = 0;
static MYSQL *mysql_init_returns = NULL;
static MYSQL *mysql_real_connect_returns = NULL;
static int mysql_real_query_returns = 0;
static MYSQL_RES *mysql_store_result_returns = NULL;
static unsigned long mysql_num_rows_returns = 0;
static unsigned int mysql_num_fields_returns = 0;
static MYSQL_FIELD *mysql_fetch_fields_returns = NULL;
static MYSQL_ROW mysql_fetch_row_returns = NULL;
static int mysql_next_result_returns = 0;
static char *mysql_error_returns = NULL;

/* call counters */
static int mysql_library_init_called = 0;
static int mysql_library_end_called = 0;
static int mysql_init_called = 0;
static int mysql_options_called = 0;
static int mysql_real_connect_called = 0;
static int mysql_close_called = 0;
static int mysql_real_query_called = 0;
static int mysql_store_result_called = 0;
static int mysql_num_rows_called = 0;
static int mysql_num_fields_called = 0;
static int mysql_fetch_fields_called = 0;
static int mysql_fetch_row_called = 0;
static int mysql_next_result_called = 0;
static int mysql_free_result_called = 0;
static int mysql_error_called = 0;

static void reset_mysql_stubs(void)
{
	mysql_library_init_returns = 0;
	mysql_init_returns = NULL;
	mysql_real_connect_returns = NULL;
	mysql_real_query_returns = 0;
	mysql_store_result_returns = NULL;
	mysql_num_rows_returns = 0;
	mysql_num_fields_returns = 0;
	mysql_fetch_fields_returns = NULL;
	mysql_fetch_row_returns = NULL;
	mysql_next_result_returns = 0;
	mysql_error_returns = NULL;
	mysql_library_init_called = 0;
	mysql_library_end_called = 0;
	mysql_options_called = 0;
	mysql_real_connect_called = 0;
	mysql_close_called = 0;
	mysql_real_query_called = 0;
	mysql_store_result_called = 0;
	mysql_num_rows_called = 0;
	mysql_num_fields_called = 0;
	mysql_fetch_fields_called = 0;
	mysql_fetch_row_called = 0;
	mysql_next_result_called = 0;
	mysql_free_result_called = 0;
	mysql_error_called = 0;
}
/* }}} */

/* {{{ mysql stubs */
static int mysql_library_init(int x, void *y, void *z)
{
	++mysql_library_init_called;
	return mysql_library_init_returns;
}

static void mysql_library_end(void)
{
	++mysql_library_end_called;
}

static MYSQL *mysql_init(void *x)
{
	++mysql_init_called;
	return mysql_init_returns;
}

static void mysql_options(MYSQL *dbh, int opt, const void *value)
{
	++mysql_options_called;
}

static MYSQL *mysql_real_connect(MYSQL *dbh, const char *host,
                                 const char *username,
                                 const char *password,
                                 const char *db,
                                 const unsigned short port,
                                 const char *sock,
                                 unsigned long flags)
{
	++mysql_real_connect_called;
	return mysql_real_connect_returns;
}

static char *mysql_error(MYSQL *dbh)
{
	++mysql_error_called;
	return mysql_error_returns;
}

static void mysql_close(MYSQL *dbh)
{
	++mysql_close_called;
	return mysql_error_returns;
}

static int mysql_real_query(MYSQL *dbh, const char *query, size_t len)
{
	++mysql_real_query_called;
	return mysql_real_query_returns;
}

static MYSQL_RES *mysql_store_result(MYSQL *dbh)
{
	++mysql_store_result_called;
	return mysql_store_result_returns;
}

static unsigned long mysql_num_rows(MYSQL_RES *res)
{
	++mysql_num_rows_called;
	return mysql_num_rows_returns;
}

static unsigned int mysql_num_fields(MYSQL_RES *res)
{
	++mysql_num_fields_called;
	return mysql_num_fields_returns;
}

static MYSQL_FIELD *mysql_fetch_fields(MYSQL_RES *res)
{
	++mysql_fetch_fields_called;
	return mysql_fetch_fields_returns;
}

static MYSQL_ROW mysql_fetch_row(MYSQL_RES *res)
{
	++mysql_fetch_row_called;
	return mysql_fetch_row_returns;
}

static void mysql_free_result(MYSQL_RES *res)
{
	++mysql_free_result_called;
}

static int mysql_next_result(MYSQL *dbh)
{
	++mysql_next_result_called;
	return mysql_next_result_returns;
}

/* }}} */

#endif /* TEST_MYSQL_STUBS_H */
