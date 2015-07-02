/**
 * Minimal Migration Manager - Migration Handling
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <string.h>
#include <ctype.h>

#include "db.h"
#include "file.h"
#include "migration.h"

static const char *down = "-- [down]";
static const char *up   = "-- [up]";

static const unsigned int down_len = 9;
static const unsigned int up_len   = 7;

/**
 * Trim leading whitespace
 *
 * \param[in] s String
 * \return A pointer to the first byte in s beyond any leading
 *         whitespace.
 */
static char *ltrim(char *s)
{
	char *tmp = s;

	while (tmp && isspace(*tmp)) ++tmp;
	return tmp;
}

/**
 * Trim trailing whitespace
 *
 * \param[in] s String
 */
static void rtrim(char *s)
{
	char *tmp;

	if (s) {
		tmp = s + strlen(s) - 1;
		while (tmp >= s && isspace(*tmp)) --tmp;
		if (tmp > s) *++tmp = 0;
	}
}

/**
 * Locate the desired query and run it.
 *
 * \param[in] path Migration path
 * \param[in] mode 1 if upgrade, 0 if downgrade
 * \return 0 on success, 1 on error.
 */
static int run_migration(const char *path, int mode)
{
	size_t size;
	char *buf, *tmp;
	int retval = 1;

	buf = map_file(path, &size);
	if (!buf) goto ret;

	/* Check for our desired query */
	tmp = strstr(buf, (mode ? up : down));
	if (!tmp) {
		retval = 0;
		goto ret;
	}

	/* Skip leading whitespace */
	buf = ltrim(tmp + (mode ? up_len : down_len) + 1);

	/* Check for the opposite query, and ignore it. */
	tmp = strstr(buf, mode ? down : up);
	if (tmp) *tmp = 0;

	/* Skip trailing whitespace */
	rtrim(buf);

	/* Run the query */
	if (*buf) retval = db_query(buf, NULL, NULL);

ret:
	unmap_file();
	return retval;
}

/**
 * Run the "up" portion of a migration.
 *
 * \param[in] path Migration to run
 * \return 0 on success, non-zero on failure.
 */
int migration_upgrade(const char *path)
{
	return run_migration(path, 1);
}

/**
 * Run the "down" portion of a migration.
 *
 * \param[in] path Migration to run
 * \return 0 on success, non-zero on failure.
 */
int migration_downgrade(const char *path)
{
	return run_migration(path, 0);
}

