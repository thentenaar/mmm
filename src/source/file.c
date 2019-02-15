/**
 * Minimal Migration Manager - File Migration Source
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>

#ifndef IN_TESTS
#include <unistd.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "backend.h"
#include "../stringbuf.h"
#include "../utils.h"

/* Configurable variables */
static struct config {
	char migration_path[256]; /**< This will usually be relative. */
} config;

/* Buffer for concatenating migration_path + filenames. */
static const char *pathbuf;

/* Local HEAD revision */
static char local_head[50];

/**
 * Callback for receiving configuration key/value pairs.
 *
 * Valid values for this module are:
 *
 * migration_path - Path to the directory in which migrations are stored.
 *   Vaiid values are a string less than 1024 bytes.
 */
static void file_config(void)
{
	CONFIG_SET_STRING("migration_path", 14, config.migration_path);
}

/**
 * Add a migration to the migration list.
 *
 * \param[in,out] migrations List of migrations.
 * \param[in,out] offset     Offset at which to append this migration
 * \param[in]     file       Filename to add
 * \return 0 on success, non-zero on error.
 */
static int add_migration(char ***migrations, size_t *offset, char *file)
{
	char **m = NULL, *tmp = NULL;
	size_t off;
	int err = 0;

	/* Create a copy of file */
	off = strlen(file);
	errno = 0;
	tmp = malloc(off + 1);
	if (!tmp) goto malloc_err;
	memcpy(tmp, file, off + 1);

	/* Now, enlarge the migrations array */
	off = *offset;
	errno = 0;
	m = realloc(*migrations, (off + 1) * sizeof(char *));
	if (!m) goto malloc_err;

	/* Finally, add the migration */
	m[off++] = tmp;
	*offset = off;
	*migrations = m;

ret:
	return err;

malloc_err:
	free(tmp);
	free(m);
	if (errno == ENOMEM)
		error("memory allocation failed: %s", strerror(ENOMEM));
	++err;
	goto ret;
}

/**
 * Scan the migration path for migrations.
 *
 * \param[out]  migrations  Pointer to the array of migrations
 * \param[out] size         Pointer holding the size of the array
 * \param[in]  pathbuf_pos  Index of the end of the string in \a pathbuf.
 * \param[in]  head         Current head revision
 * \param[in]  prev         Previous revision (for rollbacks.)
 * \return 0 on success, 1 on failure
 */
static int scan_path_for_migrations(char ***migrations, size_t *size,
                                    size_t pathbuf_pos,
                                    unsigned long head,
                                    unsigned long prev)
{
	DIR *dir;
	struct dirent *d;
	struct stat sbuf;
	size_t i;
	unsigned long x;
	char *tmp;
	int err = 0;

	if (!(dir = opendir(pathbuf))) {
		++err;
		goto ret;
	}

	while ((d = readdir(dir))) {
		/**
		 * We only want file names that are at least
		 * 5 chars long (length of *.sql),
		 */
		i = strlen(d->d_name);
		if (i < 5) continue;

		/* that begin with a number, */
		errno = 0;
		x = strtoul(d->d_name, &tmp, 0);
		if (!tmp || tmp == d->d_name ||
		    (x == ULONG_MAX && errno == ERANGE)) {
			error("warning: '%s' lacks a valid "
			      "numeric designation", d->d_name);
			continue;
		}

		/* that have a .sql extension, */
		if (memcmp(d->d_name + i - 4, ".sql", 4))
			continue;

		/* and fit in our buffer. */
		if (sbuf_add_str(d->d_name, 0, pathbuf_pos)) {
			error("warning: path too long: '%s/%s'",
			      pathbuf, d->d_name);
			continue;
		}

		/* Skip anything we can't stat() */
		if (stat(pathbuf, &sbuf))
			continue;

		/* or isn't a regular file. */
		if (!S_ISREG(sbuf.st_mode))
			continue;

		/* Skip anything before the previous revision */
		if (prev < ULONG_MAX) {
			/* ... or after the current head */
			if (x <= prev || x > head)
				continue;
		} else {
			/* Skip anything up to the current head */
			if (head < ULONG_MAX && x <= head)
				continue;
		}

		if (add_migration(migrations, size, d->d_name)) {
			++err;
			break;
		}
	}

	closedir(dir);

ret:
	return err;
}

/**
 * Copy the numeric designation in the given filename to
 * be used as the local HEAD revision.
 *
 * \param[in] head New HEAD revision.
 */
static void update_local_head(const char *head)
{
	size_t i = 0;

	while (head[i] && head[i] >= '0' && head[i] <= '9')
		i++;

	i = (i >= sizeof(local_head)) ? sizeof(local_head) - 1 : i;
	if (i) {
		memcpy(local_head, head, i);
		local_head[i] = '\0';
	}
}

/**
 * Provide an ordered list of migration files depending on
 * whether or not they should be applied.
 *
 * NOTE: The returned list of migrations must be freed.
 *
 * \param[in]  cur_rev  Last applied file
 * \param[in]  prev_rev Previous revision (for rollbacks.)
 * \param[out] size     Number of migrations in the list
 * \returns array of pointers to strings representing an ordered
 *          list of migration filenames.
 */
static char **file_find_migrations(const char *cur_rev,
                                   const char *prev_rev, size_t *size)
{
	size_t i;
	unsigned long hnum = ULONG_MAX;
	unsigned long pnum = ULONG_MAX;
	char **migrations = NULL, *tmp;

	if (size) *size = 0;
	else goto ret;

	if (!*config.migration_path) {
		error("no migration_path specified");
		goto err;
	}

	/* If we have a cur_rev, get its designation */
	if (cur_rev) {
		errno = 0;
		hnum = strtoul(cur_rev, &tmp, 0);
		if (!tmp || tmp == cur_rev || errno == ERANGE)
			hnum = ULONG_MAX;
	}

	/* If we have a prev_rev, get its designation */
	if (prev_rev) {
		errno = 0;
		pnum = strtoul(prev_rev, &tmp, 0);
		if (!tmp || tmp == prev_rev || errno == ERANGE)
			pnum = ULONG_MAX;
	}

	/* Use the common string buffer for concatenating paths */
	sbuf_reset(0);
	pathbuf = sbuf_get_buffer();
	if (sbuf_add_str(config.migration_path, 0, 0))
		goto err;

	/* Ensure we have a trailing '/' */
	i = strlen(config.migration_path);
	if (pathbuf[i - 1] != '/') {
		if (sbuf_add_str("/", 0, i++))
			goto err;
	}

	if (scan_path_for_migrations(&migrations, size, i, hnum, pnum))
		goto err;

	if (migrations && *size) {
		bubblesort(migrations, *size);
		update_local_head(migrations[*size - 1]);
	} else {
		if (hnum != ULONG_MAX)
			update_local_head(cur_rev);
	}

ret:
	return migrations;

err:
	free(migrations);
	migrations = NULL;
	goto ret;
}

/**
 * Get the latest local revision.
 *
 * \return The latest local revision as a string, or NULL if
 *         the local revision couldn't be determined.
 */
static const char *file_get_head(void)
{
	return (*local_head ? local_head : NULL);
}

/**
 * Get the latest revision of a particular file
 *
 * \param[in] file   File name.
 * \return "0" since this is only used for the seed file.
 */
static const char *file_get_file_revision(const char *file)
{
	(void)file;
	return "0";
}

/**
 * Get the base path for migrations.
 *
 * \return The base path for migrations.
 */
static const char *file_get_migration_path(void)
{
	return config.migration_path;
}

/**
 * Initialize the file source backend.
 *
 * \return 0 on success, non-zero on failure.
 */
static int file_init(void)
{
	memset(&config, 0, sizeof(config));
	memset(&local_head, 0, sizeof(local_head));
	return 0;
}

/**
 * Uninitialize the file source backend.
 *
 * \return 0 on success, non-zero on failure.
 */
static int file_uninit(void)
{
	return 0;
}

struct source_backend_vtable file_vtable = {
	"file",
	file_config,
	file_init,
	file_find_migrations,
	file_get_head,
	file_get_file_revision,
	file_get_migration_path,
	file_uninit
};
