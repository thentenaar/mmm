/**
 * Minimal Migration Manager - Command Processing
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "file.h"
#include "config.h"
#include "db.h"
#include "source.h"
#include "utils.h"
#include "state.h"
#include "stringbuf.h"
#include "migration.h"
#include "commands.h"

/**
 * Get the local HEAD revision.
 */
static int head(const char *source, const char *current,
                int UNUSED(argc), char *UNUSED(argv[]))
{
	const char *local_head;
	char **migrations;
	size_t size = 0;

	/* Get the migrations */
	migrations = source_find_migrations(source, current, NULL, &size);

	/* Get local HEAD and set the state */
	local_head = source_get_local_head(source);
	if (local_head && *local_head) {
		PRINT_1("%s\n", local_head);
	}

	if (migrations) {
		while (size) free(migrations[--size]);
		free(migrations);
	}

	return EXIT_SUCCESS;
}

/**
 * Seed the database from a .sql file.
 *
 * This command has one argument: The path to the file to seed the
 * database with.
 */
static int seed(const char *UNUSED(source), const char *UNUSED(current),
                int UNUSED(argc), char *argv[])
{
	int retval = EXIT_SUCCESS;
	const char *sfile = NULL;
	size_t size;

	if (!argv[0]) goto err;

	/* Map the seed file, and execute its contents */
	sfile = map_file(argv[0], &size);
	if (!sfile) goto err;

	PRINT("Running seed file...\n");
	if (db_query(sfile, NULL, NULL))
		goto err;

	/* Create our state table */
	if (state_create())
		goto err;

ret:
	unmap_file();
	return retval;

err:
	retval = EXIT_FAILURE;
	goto ret;
}

/**
 * List all pending migrations.
 */
static int pending(const char *source, const char *current,
                   int UNUSED(argc), char *UNUSED(argv[]))
{
	char **migrations = NULL;
	size_t size = 0, i;

	/* Get the migrations */
	migrations = source_find_migrations(source, current, NULL, &size);
	PRINT_1("%lu migrations pending:\n", size);

	/* ... and print them out. */
	if (migrations) {
		for (i = 0; i < size; i++) {
			PRINT_1("  + %s\n", migrations[i]);
			free(migrations[i]);
		}
	}

	free(migrations);
	return EXIT_SUCCESS;
}

/**
 * Apply all pending migrations.
 */
static int migrate(const char *source, const char *current,
                   int UNUSED(argc), char *UNUSED(argv[]))
{
	int retval = EXIT_FAILURE;
	char **migrations = NULL;
	const char *migration_path;
	const char *local_head;
	size_t size = 0, mp_len;
	unsigned int i;

	/* Get the migrations */
	migrations = source_find_migrations(source, current, NULL, &size);
	if (!migrations) {
		ERROR("no migrations found");
		retval = EXIT_SUCCESS;
		goto ret;
	}

	migration_path = source_get_migration_path(source);
	if (!migration_path) {
		ERROR("unable to get migration path");
		goto ret;
	} else {
		mp_len = strlen(migration_path);
		sbuf_reset(1);
		sbuf_add_str(migration_path, 0, 0);
		if (migration_path[mp_len - 1] != '/')
			sbuf_add_str("/", 0, mp_len);
	}

	if (db_query("BEGIN", NULL, NULL)) {
		ERROR("failed to BEGIN transaction");
		goto ret;
	}

	/* ... and run them. */
	for (i = 0; i < size; i++) {
		PRINT_1("Applying %s...", migrations[i]);
		sbuf_add_str(migrations[i], 0, mp_len + 1);
		if (migration_upgrade(sbuf_get_buffer()))
			goto rollback;
		PRINT(" OK\n");
	}

	if (db_query("COMMIT", NULL, NULL)) {
		ERROR("failed to COMMIT transaction");
		goto ret;
	}

	/* Get local HEAD and set the state */
	retval = EXIT_SUCCESS;
	local_head = source_get_local_head(source);
	if (state_add_revision(local_head)
	    || state_cleanup_table()) {
		ERROR("unable to set current revision");
		retval = EXIT_FAILURE;
	}

ret:
	sbuf_reset(1);
	if (migrations) {
		while (size) free(migrations[--size]);
		free(migrations);
	}
	return retval;

rollback:
	PRINT(" FAILED\n");
	if (db_query("ROLLBACK", NULL, NULL)) {
		ERROR("failed to ROLLBACK transaction");
	}

	/**
	 * This should only be required for databases which lack
	 * transactional DDL support (like MySQL.)
	 */
	if (db_has_transactional_ddl())
		goto ret;

	ERROR("your database lacks transactional DDL support. "
	      "Performing a manual rollback.");
	while (--i <= size) {
		PRINT_1("--> Rolling back %s...", migrations[i]);
		sbuf_add_str(migrations[i], 0, mp_len + 1);
		if (migration_downgrade(sbuf_get_buffer())) {
			PRINT(" FAILED\n");
		} else PRINT(" OK\n");
	}
	goto ret;
}

/**
 * Rollback migrations between HEAD and the given revision.
 */
static int rollback(const char *source, const char *current,
                    int argc, char *argv[])
{
	int retval = EXIT_FAILURE;
	char **migrations = NULL;
	const char *migration_path = NULL;
	const char *revision = NULL;
	size_t size = 0, i, mp_len;

	if (!argc) revision = state_get_previous();
	else revision = argv[0];

	if (!revision || !strcmp(current, revision)) {
		ERROR("nothing to roll back");
		retval = EXIT_SUCCESS;
		goto ret;
	}

	/* Get the migrations */
	migrations = source_find_migrations(source, current, revision,
	                                    &size);
	if (!migrations) {
		ERROR("no migrations found");
		goto ret;
	}

	migration_path = source_get_migration_path(source);
	if (!migration_path) {
		ERROR("unable to get migration path");
		goto ret;
	} else {
		mp_len = strlen(migration_path);
		sbuf_reset(1);
		sbuf_add_str(migration_path, 0, 0);
		if (migration_path[mp_len - 1] != '/')
			sbuf_add_str("/", 0, mp_len);
	}

	if (db_query("BEGIN", NULL, NULL)) {
		ERROR("failed to BEGIN transaction");
		goto ret;
	}

	/* ... and roll them back. */
	for (i = size - 1; i <= size; i--) {
		PRINT_1("Rolling back %s...", migrations[i]);
		sbuf_add_str(migrations[i], 0, mp_len + 1);
		if (migration_downgrade(sbuf_get_buffer()))
			goto rollback;
		else PRINT(" OK\n");
	}

	if (db_query("COMMIT", NULL, NULL)) {
		ERROR("failed to COMMIT transaction");
		goto ret;
	}

	/* Set the state */
	retval = EXIT_SUCCESS;
	if (state_add_revision(revision)
	    || state_cleanup_table())
		retval = EXIT_FAILURE;

ret:
	sbuf_reset(1);
	if (migrations) {
		while (size) free(migrations[--size]);
		free(migrations);
	}
	return retval;

rollback:
	PRINT(" FAILED\n");
	if (db_query("ROLLBACK", NULL, NULL)) {
		ERROR("failed to ROLLBACK transaction");
	}

	if (db_has_transactional_ddl())
		goto ret;

	ERROR("your database lacks transactional DDL support. "
	      "Please check your database manually as it may "
	      "be in an unexpected state.");
	goto ret;
}

/**
 * Create a state table in a database, and update it
 * to the current revision.
 */
static int assimilate(const char *source,
                      const char *UNUSED(current),
                      int UNUSED(argc), char *UNUSED(argv[]))
{
	char **migrations;
	size_t size = 0;
	int retval = EXIT_FAILURE;

	/* Create our state table. */
	PRINT("Resistance is futile...\n");
	if (state_create()) {
		ERROR("unable to create state table");
		goto ret;
	}

	/* Get the migrations to get the current head. */
	migrations = source_find_migrations(source, NULL, NULL, &size);
	if (migrations) {
		while (size) free(migrations[--size]);
		free(migrations);
	}

	/* Add the local HEAD revision to the table. */
	retval = EXIT_SUCCESS;
	if (state_add_revision(source_get_local_head(source))) {
		state_destroy();
		ERROR("unable to set current revision");
		retval = EXIT_FAILURE;
	}

ret:
	return retval;
}

#define N_COMMANDS 6
#define MIN_COMMAND_LEN 4
#define MAX_COMMAND_LEN 10

static const struct command {
	const char *name;
	size_t name_len;
	int argc;
	int need_current;
	int (*proc)(const char *source, const char *current,
	            int argc, char *argv[]);
} commands[N_COMMANDS] = {
	{ "head", 4, 0, 1, head },
	{ "seed", 4, 1, 0, seed }, /* argv: <seed_file> */
	{ "pending", 7, 0, 1, pending },
	{ "migrate", 7, 0, 1, migrate },
	{ "rollback", 8, 0, 1, rollback }, /* argv: <revision> */
	{ "assimilate", 10, 0, 0, assimilate }
};

/**
 * Run a command.
 *
 * \param[in] source Migration source
 * \param[in] argc   Number of arguments
 * \param[in] argv   Arguments (argv[0] = command to run)
 * \return EXIT_SUCCESS on success, COMMAND_INVALID_ARGS if the args
 *         aren't valid for the command, and EXIT_FAILURE if the
 *         command fails.
 */
int run_command(const char *source, int argc, char *argv[])
{
	int i = -1;
	size_t len;
	const char *current = NULL;
	int retval = COMMAND_INVALID_ARGS;

	if (argc < 1 || !argv || !argv[0])
		goto ret;

	/* Find the command */
	len = strlen(argv[0]);
	if (len < MIN_COMMAND_LEN || len > MAX_COMMAND_LEN)
		goto ret;

	while (++i < N_COMMANDS) {
		if (len != commands[i].name_len)
			continue;
		if (!memcmp(argv[0], commands[i].name, len))
			break;
	}

	/* Return if we didn't find the command */
	if (i >= N_COMMANDS)
		goto ret;

	/* Make sure we have sufficient args */
	if (argc - 1 < commands[i].argc)
		goto ret;

	/* Get the current revision, if needed */
	if (commands[i].need_current) {
		current = state_get_current();
		if (!current) {
			ERROR("Unable to get the current revision");
			goto ret;
		}
	}

	/* Run the command */
	retval = commands[i].proc(source, current, argc - 1, &argv[1]);

ret:
	return retval;
}
