/**
 * Minimal Migration Manager - Database Layer
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "db.h"
#include "db/driver.h"
#include "utils.h"

/* Total number of database drivers */
#define N_DB_DRIVERS 3
static const struct db_driver_vtable *drivers[N_DB_DRIVERS];

/**
 * Driver-independent representation of a database
 * connection.
 */
static struct db_session {
	size_t type; /**< Driver type */
	void *dbh;   /**< Driver-specific connection handle */
} session = { N_DB_DRIVERS, NULL };

/**
 * Lookup a database driver in the table.
 */
static size_t find_driver(const char *name, size_t len)
{
	size_t i;

	for (i = 0; i < N_DB_DRIVERS; i++) {
		/* Skip unusable drivers */
		if (!drivers[i] || !drivers[i]->name)
			continue;

		/* If the lengths aren't equal, skip it. */
		if (len != strlen(drivers[i]->name))
			continue;

		/* Now, we can match on the name */
		if (!memcmp(drivers[i]->name, name, len))
			return i;
	}

	return SIZE_MAX;
}

/**
 * Initialize the database layer.
 *
 * This function \a must be called before any other db_ functions
 * are used, as this sets up the underlying libraries and other
 * internal structures.
 *
 * If an driver fails to initialize, its pointer in the drivers
 * table will be overwritten with NULL to indicate that the
 * driver is unusable.
 */
void db_init(void)
{
	size_t i;

	for (i = 0; i < N_DB_DRIVERS; i++) {
		if (!drivers[i] || !drivers[i]->init) continue;
		if (drivers[i]->init()) {
			error("failed to initialize '%s'",
			      drivers[i]->name);
			drivers[i] = NULL;
		}
	}
}

/**
 * Lookup the configuration callback for an driver.
 *
 * \param[in] driver Name of the driver to look for.
 * \return a pointer to the driver's callback function, or NULL.
 */
config_callback_t db_get_config_cb(const char *driver, size_t len)
{
	size_t i;
	config_callback_t retval = NULL;

	if (driver && len) {
		i = find_driver(driver, len);
		if (i != SIZE_MAX)
			retval = drivers[i]->config;
	}

	return retval;
}

/**
 * Connect to a database.
 *
 * \param[in] driver_type Engine type to connect with.
 * \param[in] host        Hostname to connect to.
 * \param[in] port        Port to connect to.
 * \param[in] username    Username to authenticate with.
 * \param[in] password    Password to authenticate with.
 * \param[in] db          Database to connect to.
 * \return 0 if successful, non-zero on error.
 */
int db_connect(const char *driver, const char *host,
               const unsigned short port,
               const char *username, const char *password, const char *db)
{
	int retval = 1;
	size_t i;

	if (!driver || !db)
		goto ret;

	if (session.dbh) {
		error("another database session is currently active");
		goto ret;
	}

	/* Find the driver */
	i = strlen(driver);
	if (!i) goto ret;

	i = find_driver(driver, strlen(driver));
	if (i == SIZE_MAX) goto ret;

	/* Do the connection */
	session.dbh = drivers[i]->connect(host, port, username,
	                                  password, db);
	if (session.dbh) {
		session.type = i;
		retval = 0;
	}

ret:
	return retval;
}

/**
 * Query a database.
 *
 * \param[in] query    SQL Query to execute.
 * \param[in] callback Callback function, to be called per-row returned.
 * \param[in] userdata Userdata to be passed to the callback.
 * \return 0 on success, non-zero on error.
 */
int db_query(const char *query, db_row_callback_t callback,
             void *userdata)
{
	if (!session.dbh || !query || session.type >= N_DB_DRIVERS)
		goto err;

	if (drivers[session.type] && drivers[session.type]->query) {
		return drivers[session.type]->query(session.dbh, query,
		                                    callback, userdata);
	}

err:
	return -1;
}

/**
 * Determine the database's support for transactional DDL commands.
 *
 * \return 1 if the database enigne supports transactional DDL commands,
 *         0 otherwise.
 */
int db_has_transactional_ddl(void)
{
	return (session.dbh &&
	        drivers[session.type]->has_transactional_ddl);
}

/**
 * Disconnect the database session.
 */
void db_disconnect(void)
{
	if (!session.dbh || session.type >= N_DB_DRIVERS)
		goto ret;

	if (drivers[session.type] && drivers[session.type]->disconnect)
		drivers[session.type]->disconnect(session.dbh);
ret:
	session.type = N_DB_DRIVERS;
	session.dbh = NULL;
}

/**
 * Uninitialize the database layer.
 *
 * This must be called after you're done using the
 * \a db layer. This function calls clean-up handlers in
 * the underlying driver libraries, and additionally
 * cleans-up anything used by the database layer itself.
 */
void db_uninit(void)
{
	size_t i;

	for (i = 0; i < N_DB_DRIVERS; i++) {
		if (!drivers[i] || !drivers[i]->uninit) continue;
		if (drivers[i]->uninit()) {
			error("failed to uninitialize '%s'",
			      drivers[i]->name);
		}
	}
}

/**
 * DB Driver Registry
 *
 * Each usable driver must have as 'extern' reference to its
 * vtable here, and an entry in the drivers array.
 */
#ifdef HAVE_SQLITE3
extern const struct db_driver_vtable sqlite3_vtable;
#endif
#ifdef HAVE_MYSQL
extern const struct db_driver_vtable mysql_vtable;
#endif
#ifdef HAVE_PGSQL
extern const struct db_driver_vtable pgsql_vtable;
#endif

static const struct db_driver_vtable *drivers[N_DB_DRIVERS] = {
#ifdef HAVE_SQLITE3
	&sqlite3_vtable,
#else
	NULL,
#endif
#ifdef HAVE_MYSQL
	&mysql_vtable,
#else
	NULL,
#endif
#ifdef HAVE_PGSQL
	&pgsql_vtable,
#else
	NULL,
#endif
};
