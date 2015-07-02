/**
 * Minimal Migration Manager - SQLite3 Driver
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <stdint.h>

#ifndef IN_TESTS
#include <sqlite3.h>
#endif

#include "driver.h"
#include "../utils.h"

/**
 * Initialize the sqlite3 library.
 *
 * \return 0 on success, non-zero on error.
 */
static int db_sqlite3_init(void)
{
	return !(sqlite3_initialize() == SQLITE_OK);
}

/**
 * Uninitialize the sqlite3 library.
 *
 * \return 0 on success, non-zero on error.
 */
static int db_sqlite3_uninit(void)
{
	return !(sqlite3_shutdown() == SQLITE_OK);
}

/**
 * Open a sqlite3 database.
 *
 * \param[in] host     Unused.
 * \param[in] port     Unused.
 * \param[in] username Unused.
 * \param[in] password Unused.
 * \param[in] db       Path to the db file, or ":memory:" for an
 *                     in-memory database.
 * \return A pointer to a sqlite3 database handle.
 */
static void *db_sqlite3_connect(const char *UNUSED(host),
                                const unsigned short UNUSED(port),
                                const char *UNUSED(username),
                                const char *UNUSED(password),
                                const char *db)
{
	sqlite3 *dbh = NULL;

	if (db && *db && sqlite3_open(db, &dbh) != SQLITE_OK) {
		ERROR_1("%s", sqlite3_errmsg(dbh));
		dbh = NULL;
	}

	return (void *)dbh;
}

/**
 * Execute a query on a database connection.
 *
 * \param[in] dbh      Pointer to a sqlite3 database handle.
 * \param[in] query    SQL Query to execute.
 * \param[in] callback Callback function, to be called per-row returned.
 * \param[in] userdata Userdata to be passed to the callback.
 * \return 0 on success, non-zero on error.
 */
static int db_sqlite3_query(void *dbh, const char *query,
                            db_row_callback_t callback, void *userdata)
{
	int i;
	char *errmsg = NULL;

	i = sqlite3_exec((sqlite3 *)dbh, query, callback, userdata,
	                 &errmsg);

	/**
	 * The callback requested an abort. This may
	 * be normal.
	 */
	if (i == SQLITE_ABORT) i = SQLITE_OK;

	/* Print an error if we have one */
	if (i != SQLITE_OK && errmsg)
		ERROR_1("query failed: %s", errmsg);
	if (errmsg) sqlite3_free(errmsg);
	return !(i == SQLITE_OK);
}

/**
 * Close a sqlite3 database handle.
 *
 * \param[in] dbh Pointer to a sqlite3 database handle.
 */
static void db_sqlite3_disconnect(void *dbh)
{
	if (dbh) sqlite3_close((sqlite3 *)dbh);
}

const struct db_driver_vtable sqlite3_vtable = {
	"sqlite3",
	1,
	/* config */ NULL,
	db_sqlite3_init,
	db_sqlite3_uninit,
	db_sqlite3_connect,
	db_sqlite3_query,
	db_sqlite3_disconnect
};
