/**
 * Minimal Migration Manager - Configuration Generator
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifndef IN_TESTS
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif

#include "config_gen.h"
#include "utils.h"

static const char *default_migration_path = "migrations";

/**
 * Default config file data.
 */
static const char *default_config_1 =
    "[main]\n"
    "history=3        ; Number of state transitions to keep (max 10.)\n"
    "source=file      ; Source to get migrations from.\n"
    "driver=sqlite3   ; Database driver.\n\n"
    ";\n"
    "; Database connection settings\n"
    ";\nhost=\nport=\nusername=\npassword=\ndb=:memory:\n\n";

static const char *default_config_2 =
    ";\n"
    "; Settings for the 'file' source\n"
    ";\n"
    "[file]\n"
    "; Path (relative or absolute) to the migration files.\n"
    "migration_path=migrations\n\n"
    ";\n"
    "; Settings for the 'git' source\n"
    ";\n"
    "[git]\n"
    "; Path (relative or absolute) to the git repository.\n"
    "repo_path=.\n"
    "; Path relative to the repository where the migration files are.\n"
    "migration_path=migrations\n";

/**
 * Genrate a default config file.
 *
 * \param[in] config_file Path where the config file should be.
 * \param[in] mode        Mode of the current directory.
 * \return 0 on success, 1 on failure.
 */
static int gen_config_file(const char *config_file, mode_t mode)
{
	int fd = 0, retval = 0;
	ssize_t bw = 0;
	const char *tmp;
	size_t bytes_written = 0, size;

	/* Clear the eXecute bits in mode, and create the file */
	mode &= (mode_t)~(S_IXUSR | S_IXGRP | S_IXOTH);
	fd = open(config_file, O_CREAT | O_EXCL | O_TRUNC | O_WRONLY,
	          mode);
	if (fd < 0) {
		error("unable to create '%s'", config_file);
		goto err;
	}

	/* Write the configuration */
	tmp = default_config_1;
	size = strlen(tmp);
write_more:
	errno = 0;
	bw = write(fd, tmp + bytes_written, size - bytes_written);
	if (bw < 0) {
		if (errno == EINTR)
			goto write_more;
		goto err;
	} else bytes_written += (size_t)bw;

	/* ... till we're done. */
	if (bytes_written < size)
		goto write_more;

	/* Make sure we output the whole config file */
	if (tmp == default_config_1) {
		bytes_written = 0;
		size = strlen(default_config_2);
		tmp = default_config_2;
		goto write_more;
	}

ret:
	if (fd > -1) close(fd);
	return retval;

err:
	if (fd > -1) {
		close(fd);
		unlink(config_file);
		fd = -1;
	}

	++retval;
	goto ret;
}

/**
 * Create the intial migrations folders and config.
 *
 * \param[in] config_file Default config file name
 * \param[in] config_only If zero, create the migration dirs too.
 * \return 0 on success, 1 on failure.
 */
int generate_config(const char *config_file, int config_only)
{
	int retval = 0;
	struct stat sbuf;
	mode_t mode;

	/* Get the current directory's mode bits. */
	if (stat(".", &sbuf)) {
		error("unable to stat the current directory");
		goto err;
	} else mode = S_IRWXU | (sbuf.st_mode & (S_IRWXG | S_IRWXO));

	/* Make the config file */
	if (gen_config_file(config_file, mode))
		goto err;
	if (config_only)
		goto ret;

	/**
	 * Create our migrations directory with the same mode
	 * bits as the current directory.
	 */
	if (mkdir(default_migration_path, mode)) {
		error("unable to create '%s' directory",
		      default_migration_path);
		goto err;
	}

ret:
	return retval;

err:
	++retval;
	goto ret;
}
