/**
 * Minimal Migration Manager - Migration Source Layer
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <inttypes.h>

#include "source.h"
#include "source/backend.h"
#include "utils.h"

/* Total number of source backends */
#define N_SOURCE_BACKENDS 2
static const struct source_backend_vtable *sources[N_SOURCE_BACKENDS];

/**
 * Lookup a source backend in the table.
 */
static size_t find_backend(const char *name, size_t len)
{
	size_t i;

	if (!name || !len) goto ret;
	for (i = 0; i < N_SOURCE_BACKENDS; i++) {
		/* Skip unusable sources */
		if (!sources[i] || !sources[i]->name)
			continue;

		/* If the lengths aren't equal, skip it. */
		if (len != strlen(sources[i]->name))
			continue;

		/* Now, we can match on the name */
		if (!memcmp(sources[i]->name, name, len))
			return i;
	}

ret:
	return SIZE_MAX;
}

/**
 * Initialize the migration source layer.
 *
 * This function \a must be called before any other source_ functions
 * are used, as this sets up the sources themselves.
 */
void source_init(void)
{
	size_t i;

	for (i = 0; i < N_SOURCE_BACKENDS; i++) {
		if (!sources[i] || !sources[i]->init)
			continue;

		if (sources[i]->init()) {
			error("failed to initialize '%s'",
			      sources[i]->name);
			sources[i] = NULL;
		}
	}
}

/**
 * Lookup the configuration callback for a source.
 *
 * \param[in] source Name of the source to look for.
 * \return a pointer to the source's callback function, or NULL.
 */
config_callback_t source_get_config_cb(const char *source, size_t len)
{
	size_t i;

	i = find_backend(source, len);
	if (i == SIZE_MAX) goto ret;

	if (sources[i]->config)
		return sources[i]->config;

ret:
	return NULL;
}

/**
 * Provide an ordered list of migration files, in the order that
 * they would be applied.
 *
 * \param[in]  source   Name of the source to use.
 * \param[in]  cur_rev  Last applied revision.
 * \param[in]  prev_rev Previous revision (for rollbacks.)
 * \param[out] size     Number of migrations in the list.
 * \returns array of pointers to strings representing an ordered
 *          list of migration filenames.
 */
char **source_find_migrations(const char *source, const char *cur_rev,
                              const char *prev_rev, size_t *size)
{
	size_t i;

	if (!source || !size)
		goto ret;

	i = find_backend(source, strlen(source));
	if (i == SIZE_MAX) goto ret;

	if (sources[i]->find_migrations) {
		return sources[i]->find_migrations(cur_rev, prev_rev,
		                                   size);
	}

ret:
	if (size) *size = 0;
	return NULL;
}

/**
 * Get the latest local revision.
 *
 * \param[in] source Name of the source to use.
 * \return The latest local revision as a string, or NULL if the
 *         local revision isn't known.
 */
const char *source_get_local_head(const char *source)
{
	size_t i;

	if (!source) goto err;

	i = find_backend(source, strlen(source));
	if (i != SIZE_MAX)
		return sources[i]->get_head();

err:
	return NULL;
}

/**
 * Get the latest revision of a particular file
 *
 * \param[in] source Name of the source to use.
 * \param[in] file   File name.
 * \return The lastest revision of the file as a string, or NULL if the
 *         revision isn't known.
 */
const char *source_get_file_revision(const char *source, const char *file)
{
	size_t i;

	if (!source || !file) goto err;

	i = find_backend(source, strlen(source));
	if (i != SIZE_MAX && sources[i]->get_file_revision)
		return sources[i]->get_file_revision(file);

err:
	return NULL;
}

/**
 * Get the base path for migrations.
 *
 * \param[in] source Name of the source to use.
 * \return The base path for migrations for the given source, or
 *         NULL if the source doesn't provide it.
 */
const char *source_get_migration_path(const char *source)
{
	size_t i;

	if (!source) goto err;

	i = find_backend(source, strlen(source));
	if (i != SIZE_MAX)
		return sources[i]->get_migration_path();

err:
	return NULL;
}

/**
 * Uninitialize the migration source layer.
 *
 * This must be called after you're done using the
 * \a source layer. This function calls clean-up handlers in
 * the underlying libraries.
 */
void source_uninit(void)
{
	size_t i;

	for (i = 0; i < N_SOURCE_BACKENDS; i++) {
		if (!sources[i] || !sources[i]->uninit)
			continue;

		if (sources[i]->uninit()) {
			error("failed to uninitialize '%s'",
			      sources[i]->name);
		}
	}
}

/**
 * Migration Source Repository
 *
 * Each source must have as 'extern' reference to its vtable here,
 * and an entry in the sources array.
 */
#ifndef IN_TESTS
extern const struct source_backend_vtable file_vtable;
#endif

#ifdef HAVE_GIT
extern const struct source_backend_vtable git_vtable;
#endif

static const struct source_backend_vtable *sources[N_SOURCE_BACKENDS] = {
#ifndef IN_TESTS
	&file_vtable,
#else
	NULL,
#endif

#ifdef HAVE_GIT
	&git_vtable,
#else
	NULL,
#endif
};
