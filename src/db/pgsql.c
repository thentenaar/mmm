/**
 * Minimal Migration Manager - PostgreSQL Driver
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>

#ifndef IN_TESTS
#include <libpq-fe.h>
#endif

#include "driver.h"
#include "../stringbuf.h"
#include "../utils.h"

/**
 * Open a connection to a postgresql database.
 *
 * NOTE: The client encoding is assumed to be UTF-8.
 *
 * \param[in] host     Host / Socket to connect to.
 * \param[in] port     Port to connect on.
 * \param[in] username Username to authenticate with.
 * \param[in] password Password to authenticate with.
 * \param[in] db       Database to use.
 * \return A pointer to a PGconn connection handle.
 */
static void *db_pgsql_connect(const char *host, const unsigned short port,
                              const char *username, const char *password,
                              const char *db)
{
	PGconn *dbh = NULL;

	if (!db) goto ret;

	/* Build the connection string */
	sbuf_reset(0);
	if (sbuf_add_param_str("client_encoding", "UTF-8") ||
	    sbuf_add_param_str("host", host) ||
	    sbuf_add_param_num("port", port) ||
	    sbuf_add_param_str("user", username) ||
	    sbuf_add_param_str("password", password) ||
	    sbuf_add_param_str("dbname", db))
		goto ret;

	/* Connect to the db */
	dbh = PQconnectdb(sbuf_get_buffer());
	if (dbh && PQstatus(dbh) != CONNECTION_OK) {
		ERROR_1("%s", PQerrorMessage(dbh));
		PQfinish(dbh);
		dbh = NULL;
	}

ret:
	sbuf_reset(1);
	return dbh;
}

/**
 * Execute a query on a database connection.
 *
 * \param[in] dbh      PGconn connection handle.
 * \param[in] query    SQL Query to execute.
 * \param[in] callback Callback function, to be called per-row returned.
 * \param[in] userdata Userdata to be passed to the callback.
 * \return 0 on success, non-zero on error.
 */
static int db_pgsql_query(void *dbh, const char *query,
                          db_row_callback_t callback, void *userdata)
{
	PGresult *res;
	char **columns = NULL, **row = NULL, *errmsg;
	int i, j, nrows, ncols, retval = 0;

	if (!dbh || !query) goto err;

	/* Perform the query */
	res = PQexec(dbh, query);
	if (!res) goto err_msg;

	/* The command ran successfully */
	if (PQresultStatus(res) == PGRES_COMMAND_OK)
		goto done;

	/* The command ran successfully, and returned results */
	if (PQresultStatus(res) != PGRES_TUPLES_OK) {
		PQclear(res);
		goto err_msg;
	}

	/* Process the result */
	nrows = PQntuples(res);
	ncols = PQnfields(res);
	if (nrows <= 0 || ncols <= 0 || !callback)
		goto done;

	/* Allocate an array for the field names */
	columns = malloc(sizeof(char *) * (unsigned int)ncols);
	if (!columns) {
		retval = 1;
		goto done;
	} else {
		for (i = 0; i < ncols; i++)
			columns[i] = PQfname(res, i);
	}

	row = malloc(sizeof(char *) * (unsigned int)ncols);
	if (!row) {
		retval = 1;
		free(columns);
		goto done;
	}

	/* Fetch the rows and pass them to the callback */
	for (i = 0; i < nrows; i++) {
		for (j = 0; j < ncols; j++) {
			if (PQgetisnull(res, i, j))
				row[j] = NULL;
			else row[j] = PQgetvalue(res, i, j);
		}

		if (callback(userdata, ncols, row, columns))
			break;
	}

	free(row);
	free(columns);

done:
	PQclear(res);

ret:
	return retval;

err_msg:
	errmsg = PQerrorMessage(dbh);
	if (errmsg) ERROR_1("query failed: %s", errmsg);

err:
	++retval;
	goto ret;
}

/**
 * Close a postgresql connection.
 *
 * \param[in] dbh PGconn database handle.
 */
static void db_pgsql_disconnect(void *dbh)
{
	if (dbh) PQfinish((PGconn *)dbh);
}

const struct db_driver_vtable pgsql_vtable = {
	"pgsql",
	1,
	/* config */ NULL,
	/* init   */ NULL,
	/* uninit */ NULL,
	db_pgsql_connect,
	db_pgsql_query,
	db_pgsql_disconnect
};
