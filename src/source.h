/**
 * \file source.h
 *
 * Minimal Migration Manager - Migration Source Layer
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef SOURCE_H
#define SOURCE_H

#include "config.h"

/**
 * Initialize the migration source layer.
 *
 * This function \a must be called before any other source_ functions
 * are used, as this sets up the sources themselves.
 */
void source_init(void);

/**
 * Lookup the configuration callback for a source.
 *
 * \param[in] source Name of the source to look for.
 * \param[in] len    Length of source.
 * \return a pointer to the source's callback function, or NULL.
 */
config_callback_t source_get_config_cb(const char *source, size_t len);

/**
 * Provide an ordered list of migration files, in the order that
 * they would be applied.
 *
 * \param[in]  source   Name of the source to use.
 * \param[in]  cur_rev  Last applied revision.
 * \param[in]  prev_rev Previous applied revision (for rollbacks.)
 * \param[out] size     Number of migrations in the list.
 * \returns array of pointers to strings representing an ordered
 *          list of migration filenames.
 */
char **source_find_migrations(const char *source, const char *cur_rev,
                              const char *prev_rev, size_t *size);

/**
 * Get the latest local revision.
 *
 * \param[in] source Name of the source to use.
 * \return The latest local revision as a string, or NULL if the
 *         local revision isn't known.
 */
const char *source_get_local_head(const char *source);

/**
 * Get the latest revision of a particular file
 *
 * \param[in] source Name of the source to use.
 * \param[in] file   File name.
 * \return The lastest revision of the file as a string, or NULL if the
 *         revision isn't known.
 */
const char *source_get_file_revision(const char *source, const char *file);

/**
 * Get the base path for migrations.
 *
 * \param[in] source Name of the source to use.
 * \return The base path for migrations for the given source, or
 *         NULL if the source doesn't provide it.
 */
const char *source_get_migration_path(const char *source);

/**
 * Uninitialize the migration source layer.
 *
 * This must be called after you're done using the
 * \a source layer. This function calls clean-up handlers in
 * the underlying libraries.
 */
void source_uninit(void);

#endif /* SOURCE_H */
