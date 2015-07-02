/**
 * \file backend.h
 *
 * Minimal Migration Manager - Migration Source Layer
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef BACKEND_H
#define BACKEND_H

#include "../config.h"

struct source_backend_vtable {
	const char *name;

	/**
 	 * Callback for processing configuration values.
 	 *
 	 * These should be handled with the CONFIG_SET_* macros.
 	 */
	config_callback_t config;

	/**
	 * Initialize the backend.
	 *
	 * \return 0 on success, non-zero on failure.
	 */
	int (*init)(void);

	/**
	 * Provide an ordered list of migration files in the order
	 * that they would be applied.
	 *
	 * \param[in]  cur_rev  Last applied revision.
	 * \param[in]  prev_rev Previous revision (for rollbacks.)
	 * \param[out] size     Number of migrations in the list.
	 * \returns array of pointers to strings representing an ordered
	 *          list of migration filenames.
	 */
	char **(*find_migrations)(const char *cur_rev, const char *prev_rev,
	                          size_t *size);

	/**
 	 * Get the latest local revision.
 	 *
 	 * \return The latest local revision as a string, or NULL if
 	 *         the local revision couldn't be determined.
 	 */
	const char *(*get_head)(void);

	/**
	 * Get the base path for migrations.
	 *
	 * \return The base path for migrations.
	 */
	const char *(*get_migration_path)(void);

	/**
	 * Uninitialize the backend, doing any cleanup along the way.
	 *
	 * \return 0 on success, non-zero on failure.
	 */
	int (*uninit)(void);
};

#endif /* BACKEND_H */
