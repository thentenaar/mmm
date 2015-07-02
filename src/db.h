/**
 * \file db.h
 *
 * Minimal Migration Manager - Database Layer
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef DB_H
#define DB_H

#include <limits.h>
#include <inttypes.h>
#include "config.h"

/**
 * Callback for handling database result rows.
 *
 * \param[in] userdata     Userdata passed from \a db_driver_query.
 * \param[in] n_cols       Number of columns in the result.
 * \param[in] fields       Data fields for this row.
 * \param[in] column_names Array of column names corresponding to the row
 *                         data.
 * \return 0 on success, 1 on failure or to stop processing data.
 */
typedef int (*db_row_callback_t)(void *userdata, int n_cols,
                                 char **fields, char **column_names);

/**
 * Initialize the database layer.
 *
 * This function \a must be called before any other db_ functions
 * are used, as this sets up the underlying driver libraries and
 * other internal structures.
 */
void db_init(void);

/**
 * Lookup the configuration callback for an driver.
 *
 * \param[in] driver Name of the driver to look for.
 * \param[in] len    Length of source.
 * \return a pointer to the driver's callback function, or NULL.
 */
config_callback_t db_get_config_cb(const char *driver, size_t len);

/**
 * Connect to a database.
 *
 * \param[in] driver   Engine type to connect with.
 * \param[in] host     Hostname to connect to.
 * \param[in] port     Port to connect to.
 * \param[in] username Username to authenticate with.
 * \param[in] password Password to authenticate with.
 * \param[in] db       Database to connect to.
 * \return 0 on success, non-zero on error.
 */
int db_connect(const char *driver, const char *host,
               const unsigned short port,
               const char *username, const char *password,
               const char *db);

/**
 * Query a database.
 *
 * \param[in] query    SQL Query to execute.
 * \param[in] callback Callback function, to be called per-row returned.
 * \param[in] userdata Userdata to be passed to the callback.
 * \return 0 on success, non-zero on error.
 */
int db_query(const char *query, db_row_callback_t callback,
             void *userdata);

/**
 * Determine the database's support for transactional DDL commands.
 *
 * \return 1 if the database enigne supports transactional DDL commands,
 *         0 otherwise.
 */
int db_has_transactional_ddl(void);

/**
 * Disconnect the database session.
 */
void db_disconnect(void);

/**
 * Uninitialize the database layer.
 *
 * This must be called after you're done using the
 * \a db layer. This function calls clean-up handlers in
 * the underlying driver libraries, and additionally
 * cleans-up anything used by the database layer itself.
 */
void db_uninit(void);

#endif /* DB_H */
