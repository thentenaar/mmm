/**
 * Minimal Migration Manager - MySQL Driver
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>

#ifndef IN_TESTS
#include <mysql/mysql.h>
#endif

#include "driver.h"
#include "../utils.h"

/**
 * Constant representing a value of 'TRUE' for
 * MySQL options.
 */
static const my_bool tr = 1;

/**
 * Initialize the mysql library.
 *
 * \return 0 on success, non-zero on error.
 */
static int db_mysql_init(void)
{
	return mysql_library_init(0, NULL, NULL);
}

/**
 * Uninitialize the mysql library.
 *
 * \return 0 on success, non-zero on error.
 */
static int db_mysql_uninit(void)
{
	mysql_library_end();
	return 0;
}

/**
 * Open a connection to a mysql database.
 *
 * This function reads the my.cnf [mysql] section to acquire
 * its defaults, if available; and sets the default session
 * character set to UTF-8.
 *
 * Passing a host that starts with '/' will cause this function
 * to attempt to connect via the UNIX socket located at the path
 * specified in host.
 *
 * \param[in] host     Host / Socket to connect to.
 * \param[in] port     Port to connect on.
 * \param[in] username Username to authenticate with.
 * \param[in] password Password to authenticate with.
 * \param[in] db       Database to use.
 * \return A pointer to a MYSQL connection handle.
 */
static void *db_mysql_connect(const char *host, const unsigned short port,
                              const char *username, const char *password,
                              const char *db)
{
	MYSQL *dbh = NULL, *conn = NULL;

	if (!username || !password || !db)
		goto ret;

	dbh = mysql_init(NULL);
	if (!dbh) goto ret;

	mysql_options(dbh, MYSQL_READ_DEFAULT_GROUP, "mysql");
	mysql_options(dbh, MYSQL_SET_CHARSET_NAME, "utf8");
	mysql_options(dbh, MYSQL_OPT_RECONNECT, &tr);

	/* Socket connections should be specified via 'host' */
	if (host && *host == '/') {
		conn = mysql_real_connect(dbh, NULL, username, password,
		                          db, 0, host,
		                          CLIENT_COMPRESS |
		                          CLIENT_MULTI_STATEMENTS);
	} else {
		conn = mysql_real_connect(dbh, host, username, password,
		                          db, port, NULL,
		                          CLIENT_COMPRESS |
		                          CLIENT_MULTI_STATEMENTS);
	}

	if (!conn) {
		error("[mysql_connect] %s", mysql_error(dbh));
		if (dbh) mysql_close(dbh);
		dbh = NULL;
	}

ret:
	return (void *)dbh;
}

/**
 * Execute a query on a database connection.
 *
 * NOTE: If we don't handle ALL results from the server, it may
 * drop the connection.
 *
 * \param[in] dbh      MYSQL connection handle.
 * \param[in] query    SQL Query to execute.
 * \param[in] callback Callback function, to be called per-row returned.
 * \param[in] userdata Userdata to be passed to the callback.
 * \return 0 on success, non-zero on error.
 */
static int db_mysql_query(void *dbh, const char *query,
                          db_row_callback_t callback, void *userdata)
{
	MYSQL_RES *res = NULL;
	MYSQL_FIELD *fields;
	MYSQL_ROW row;
	char **columns = NULL;
	const char *errmsg;
	int retval = 0, x = 0;
	unsigned int ncols, i;
	unsigned long nrows;

	if (!dbh || !query) goto err;

	/* Perform the query */
	if (mysql_real_query(dbh, query, strlen(query)))
		goto err_msg;

	do {
		/* Get the result */
		res = mysql_store_result(dbh);
		if (!res) goto next_result;

		/* If we don't need/want more results, skip processing. */
		if (retval) goto next_result;

		/* Ensure we have at least 1 row and column */
		nrows = mysql_num_rows(res);
		ncols = mysql_num_fields(res);
		if (!nrows || !ncols || !callback)
			goto next_result;

		/* Fetch the fields */
		fields = mysql_fetch_fields(res);
		if (!fields) goto next_result;

		/* Allocate an array for the field names */
		columns = malloc(sizeof(char *) * ncols);
		if (!columns) {
			retval = 1;
			goto next_result;
		} else {
			for (i = 0; i < ncols; i++)
				columns[i] = fields[i].name;
		}

		/* Fetch the rows and pass them to the callback */
		for (i = 0; i < nrows; i++) {
			row = mysql_fetch_row(res);
			retval = callback(userdata, (int)ncols, row,
			                  columns);
			if (retval) break;
		}

		free(columns);

next_result:
		if (res) mysql_free_result(res);
		x = mysql_next_result(dbh);
	} while (!x);

	if (x > 0) goto err_msg;
	retval = 0;

ret:
	return retval;

err_msg:
	errmsg = mysql_error(dbh);
	if (errmsg) error("query failed: %s", errmsg);

err:
	++retval;
	goto ret;
}

/**
 * Close a mysql connection.
 *
 * \param[in] dbh MYSQL database handle.
 */
static void db_mysql_disconnect(void *dbh)
{
	if (dbh) mysql_close((MYSQL *)dbh);
}

const struct db_driver_vtable mysql_vtable = {
	"mysql",
	0,
	/* config */ NULL,
	db_mysql_init,
	db_mysql_uninit,
	db_mysql_connect,
	db_mysql_query,
	db_mysql_disconnect
};
