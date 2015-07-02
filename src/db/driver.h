/**
 * \file driver.h
 *
 * Minimal Migration Manager - Database Driver Interface
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef DB_DRIVER_H
#define DB_DRIVER_H

#include "../db.h"
#include "../config.h"

/**
 * Driver Vector Table
 *
 * This vector table defines the interface between individual database
 * drivers and the \a db layer.
 */
struct db_driver_vtable {
	/**
	 * Driver name (e.g. "sqlite3")
	 */
	const char *name;

	/**
	 * If the databaase engine has transactional DDL support,
	 * this should be non-zero.
	 */
	const int has_transactional_ddl;

	/**
 	 * Callback for processing configuration values.
 	 *
 	 * These should be handled with the CONFIG_SET_* macros.
 	 */
	config_callback_t config;

	/**
	 * Initialize the database driver.
	 *
	 * \return 0 on success, non-zero on error.
	 */
	int (*init)(void);

	/**
	 * Uninitialize the database driver.
	 *
	 * \return 0 on success, non-zero on error.
	 */
	int (*uninit)(void);

	/**
	 * Open a new database connection.
	 *
	 * \param[in] host     Hostname to connect to.
	 * \param[in] port     Port to connect to.
	 * \param[in] username Username to authenticate with.
	 * \param[in] password Password to authenticate with.
	 * \param[in] db       Database to connect to.
	 * \return A pointer to an driver-specific connection handle.
	 */
	void *(*connect)(const char *host, const unsigned short port,
	                 const char *username, const char *password,
	                 const char *db);

	/**
	 * Execute a query on a database connection.
	 *
	 * \param[in] dbh      Engine-specific connection handle.
	 * \param[in] query    SQL Query to execute.
	 * \param[in] callback Callback function, to be called per-row returned.
	 * \param[in] userdata Userdata to be passed to the callback.
	 * \return 0 on success, non-zero on error.
	 */
	int (*query)(void *dbh, const char *query,
	             db_row_callback_t callback, void *userdata);

    /**
     * Disconnect a database connection.
     *
     * \param[in] dbh Engine-specific connection handle.
     */
	void (*disconnect)(void *dbh);
};

#endif /* DB_DRIVER_H */
