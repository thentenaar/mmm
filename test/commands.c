/**
 * Minimal Migration Manager - Command Processing Tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include <check.h>

#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

/* {{{ stubs */
typedef int (*db_row_callback_t)(void *userdata, int n_cols,
                                 char **fields, char **column_names);

static char *map_file(const char *path, size_t *size);
static void unmap_file(char *mem, size_t size);
static int db_query(const char *query, db_row_callback_t cb,
                    void *userdata);
static int db_has_transactional_ddl(void);
static int state_create(void);
static const char *state_get_current(void);
static const char *state_get_previous(void);
static int state_cleanup_table(void);
static int state_add_revision(const char *rev);
static int state_destroy(void);
static char **source_find_migrations(const char *source,
                                     const char *cur_rev,
                                     const char *prev_rev,
                                     size_t *size);
static const char *source_get_local_head(const char *source);
static const char *source_get_file_revision(const char *source,
                                            const char *file);
static const char *source_get_migration_path(const char *source);
static int migration_upgrade(const char *path);
static int migration_downgrade(const char *path);

#define FILE_H
#define CONFIG_H
#define DB_H
#define SOURCE_H
#define STATE_H
#define MIGRATION_H
#include "../src/commands.c"

static size_t map_file_returns_size = 0;
static char *map_file_returns = NULL;
static int db_query_returns = 0;
static int db_has_transactional_ddl_returns = 0;
static int state_create_returns = 0;
static const char *state_get_current_returns = NULL;
static const char *state_get_previous_returns = NULL;
static int state_cleanup_table_returns = 0;
static int state_add_revision_returns = 0;
static int state_destroy_returns = 0;
static char **source_find_migrations_returns = NULL;
static size_t source_find_migrations_returns_size = 0;
static char *source_get_local_head_returns = NULL;
static char *source_get_migration_path_returns = NULL;
static int migration_upgrade_returns = 0;
static int migration_downgrade_returns = 0;

static int map_file_called = 0;
static int unmap_file_called = 0;
static int db_query_called = 0;
static int db_has_transactional_ddl_called = 0;
static int state_create_called = 0;
static int state_get_current_called = 0;
static int state_get_previous_called = 0;
static int state_cleanup_table_called = 0;
static int state_add_revision_called = 0;
static int state_destroy_called = 0;
static int source_find_migrations_called = 0;
static int source_get_local_head_called = 0;
static int source_get_migration_path_called = 0;
static int migration_upgrade_called = 0;
static int migration_downgrade_called = 0;

static int db_query_begin_fails = 0;
static int db_query_commit_fails = 0;
static int db_query_rollback_fails = 0;

static void reset_stubs(void)
{
	map_file_returns_size = 0;
	map_file_returns = NULL;
	db_query_returns = 0;
	db_has_transactional_ddl_returns = 0;
	state_create_returns = 0;
	state_get_current_returns = NULL;
	state_get_previous_returns = NULL;
	state_cleanup_table_returns = 0;
	state_add_revision_returns = 0;
	state_destroy_returns = 0;
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;
	source_get_local_head_returns = NULL;
	source_get_migration_path_returns = NULL;
	migration_upgrade_returns = 0;
	migration_downgrade_returns = 0;

	map_file_called = 0;
	unmap_file_called = 0;
	db_query_called = 0;
	db_has_transactional_ddl_called = 0;
	state_create_called = 0;
	state_get_current_called = 0;
	state_get_previous_called = 0;
	state_cleanup_table_called = 0;
	state_add_revision_called = 0;
	state_destroy_called = 0;
	source_find_migrations_called = 0;
	source_get_local_head_called = 0;
	source_get_migration_path_called = 0;
	migration_upgrade_called = 0;
	migration_downgrade_called = 0;

	db_query_begin_fails = 0;
	db_query_commit_fails = 0;
	db_query_rollback_fails = 0;
}

static char *map_file(const char *path, size_t *size)
{
	(void)path;
	++map_file_called;
	*size = map_file_returns_size;
	return map_file_returns;
}

static void unmap_file(char *mem, size_t size)
{
	(void)mem;
	(void)size;
	++unmap_file_called;
	return;
}

static int db_has_transactional_ddl(void)
{
	++db_has_transactional_ddl_called;
	return db_has_transactional_ddl_returns;
}

static int db_query(const char *query, db_row_callback_t cb,
                    void *userdata)
{
	int retval = db_query_returns;

	(void)cb;
	(void)userdata;
	++db_query_called;
	if (query) {
		if (db_query_begin_fails && !strcmp(query, "BEGIN"))
			retval = 1;
		if (db_query_commit_fails && !strcmp(query, "COMMIT"))
			retval = 1;
		if (db_query_rollback_fails && !strcmp(query, "ROLLBACK"))
			retval = 1;
	}

	return retval;
}

static int state_create(void)
{
	++state_create_called;
	return state_create_returns;
}

static const char *state_get_current(void)
{
	++state_get_current_called;
	return state_get_current_returns;
}

static const char *state_get_previous(void)
{
	++state_get_previous_called;
	return state_get_previous_returns;
}

static int state_cleanup_table(void)
{
	++state_cleanup_table_called;
	return state_cleanup_table_returns;
}

static int state_add_revision(const char *rev)
{
	(void)rev;
	++state_add_revision_called;
	return state_add_revision_returns;
}

static int state_destroy(void)
{
	++state_destroy_called;
	return state_destroy_returns;
}

static char **source_find_migrations(const char *source,
                                     const char *cur_rev,
                                     const char *prev_rev,
                                     size_t *size)
{
	(void)source;
	(void)cur_rev;
	(void)prev_rev;
	++source_find_migrations_called;
	if (size) *size = source_find_migrations_returns_size;
	return source_find_migrations_returns;
}

static const char *source_get_file_revision(const char *source,
                                            const char *file)
{
	(void)source;
	(void)file;
	return NULL;
}

static const char *source_get_local_head(const char *source)
{
	(void)source;
	++source_get_local_head_called;
	return source_get_local_head_returns;
}

static const char *source_get_migration_path(const char *source)
{
	(void)source;
	++source_get_migration_path_called;
	return source_get_migration_path_returns;
}

static int migration_upgrade(const char *path)
{
	(void)path;
	++migration_upgrade_called;
	if (migration_upgrade_returns > 1) {
		--migration_upgrade_returns;
		return 0;
	}

	return migration_upgrade_returns;
}

static int migration_downgrade(const char *path)
{
	(void)path;
	++migration_downgrade_called;
	if (migration_downgrade_returns > 1) {
		--migration_downgrade_returns;
		return 0;
	}

	return migration_downgrade_returns;
}

/**
 * A simple strdup(3) clone.
 *
 * Normally, I'm not a big fan of cloning a function
 * which is specified in SUSv2, however on OSX,
 * with our chosen CFLAGS, strdup isn't exposed.
 */
static char *my_strdup(const char *s)
{
	char *d;

	if (!(d = malloc(strlen(s) + 1)))
		return NULL;
	strcpy(d, s);
	d[strlen(s)] = '\0';
	return d;
}

static char xxx[]         = "xxx";
static char query[]       = "query";
static char xtest[]       = "test";
static char xseed[]       = "seed";
static char xhead[]       = "head";
static char xempty[]      = "";
static char xmigrate[]    = "migrate";
static char xpending[]    = "pending";
static char xrollback[]   = "rollback";
static char xassimilate[] = "assimilate";
static char xtest_sql[]   = "test.sql";
static char xtmp[]        = "/tmp";

/* }}} */

/**
 * Test that run_command() behaves correctly when given invalid args.
 */
START_TEST(run_command_invalid_args)
{
	char argvx[MAX_COMMAND_LEN + 2];
	char *argv[4] = { xtest, xempty, NULL, NULL };

	argv[3] = argvx;
	memset(argvx, 'x', sizeof argvx);
	argvx[sizeof argvx - 1] = '\0';
	ck_assert_int_eq(run_command("seed", 1, NULL),     COMMAND_INVALID_ARGS);
	ck_assert_int_eq(run_command("seed", 0, argv),     COMMAND_INVALID_ARGS);
	ck_assert_int_eq(run_command("seed", 1, argv + 1), COMMAND_INVALID_ARGS);
	ck_assert_int_eq(run_command("seed", 1, argv + 2), COMMAND_INVALID_ARGS);
	ck_assert_int_eq(run_command("seed", 1, argv + 3), COMMAND_INVALID_ARGS);
}
END_TEST

/**
 * Test that run_command() behaves correctly when the given command
 * isn't in the array of commands.
 */
START_TEST(run_command_not_found)
{
	char argv0[MAX_COMMAND_LEN - 1];
	char *argv[1];

	*argv = argv0;
	memset(argv0, 'x', sizeof argv0);
	argv0[sizeof argv0 - 1] = '\0';
	ck_assert_int_eq(run_command("s", 1, argv), COMMAND_NOT_FOUND);
}
END_TEST

/**
 * Test that run_command() behaves correctly when insufficient arguments
 * are passed for the command.
 */
START_TEST(run_command_insufficient_args)
{
	char *argv[1] = { xseed };
	ck_assert_int_eq(run_command("seed", 1, argv), COMMAND_INVALID_ARGS);
}
END_TEST

/**
 * Test that run_command() yields an error if the command needs the
 * current revision, and it was not loaded.
 */
START_TEST(run_command_no_current_revision)
{
	char *argv[1] = { xmigrate };
	state_get_current_returns = NULL;
	ck_assert_int_eq(run_command("migrate", 1, argv), COMMAND_INVALID_ARGS);
}
END_TEST

/**
 * Test that run_command() works.
 *
 * This also covers the case for seed() where argv[0]
 * is NULL.
 */
START_TEST(test_run_command)
{
	char *argv[2] = { xseed, NULL };
	ck_assert_int_eq(run_command("seed", 2, argv), EXIT_FAILURE);
}
END_TEST

/**
 * Test that head prints nothing if no local_head is
 * given by the source.
 */
START_TEST(head_no_local_head)
{
	char *argv[1] = { xhead };

	*errbuf = '\0';
	state_get_current_returns = "xxx";
	source_get_local_head_returns = NULL;
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	ck_assert_int_eq(run_command("head", 1, argv), EXIT_SUCCESS);
	ck_assert(!*errbuf);
}
END_TEST

/**
 * Test that head works.
 */
START_TEST(test_head)
{
	char **migs;
	char *argv[1] = { xhead };
	char head[] = "yyy";

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_local_head_returns = head;

	ck_assert_int_eq(run_command("head", 1, argv), EXIT_SUCCESS);
	ck_assert_str_eq(errbuf, "yyy\n");
}
END_TEST

/**
 * Test that the seed command returns EXIT_FAILURE if
 * the specifiied file cannot be mapped.
 */
START_TEST(seed_cant_map_file)
{
	char *argv[2] = { xseed, xtest_sql };

	map_file_returns = NULL;
	map_file_returns_size = 0;
	ck_assert_int_eq(run_command("seed", 2, argv), EXIT_FAILURE);
}
END_TEST

/**
 * Test that the seed command returns EXIT_FAILURE if
 * running the seed file fails.
 */
START_TEST(seed_query_fails)
{
	char *argv[2] = { xseed, xtest_sql };

	map_file_returns = query;
	map_file_returns_size = 1;
	db_query_returns = 1;
	ck_assert_int_eq(run_command("seed", 2, argv), EXIT_FAILURE);
}
END_TEST

/**
 * Test that the seed command returns EXIT_FAILURE if
 * creating the state table fails.
 */
START_TEST(seed_creating_state_fails)
{
	char *argv[2] = { xseed, xtest_sql };

	map_file_returns = query;
	map_file_returns_size = 1;
	db_query_returns = 0;
	state_create_returns = 1;
	ck_assert_int_eq(run_command("seed", 2, argv), EXIT_FAILURE);
}
END_TEST

/**
 * Test that the seed command works.
 */
START_TEST(test_seed)
{
	char *argv[2] = { xseed, xtest_sql };

	map_file_returns = query;
	map_file_returns_size = 1;
	db_query_returns = 0;
	state_create_returns = 0;
	ck_assert_int_eq(run_command("seed", 2, argv), EXIT_SUCCESS);
}
END_TEST

/**
 * Test the pending command when no migrations are present.
 */
START_TEST(pending_no_migrations)
{
	char *argv[1] = { xpending };

	*errbuf = '\0';
	state_get_current_returns = "xxx";
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	ck_assert_int_eq(run_command("pending", 1, argv), EXIT_SUCCESS);
	ck_assert_str_eq(errbuf, "0 migrations pending:\n");
}
END_TEST

/**
 * Test that pending correctly displays pending migrations.
 */
START_TEST(test_pending)
{
	char **migs;
	char *argv[1] = { xpending };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;

	ck_assert_int_eq(run_command("pending", 1, argv), EXIT_SUCCESS);
	ck_assert_str_eq(errbuf, "  + test.sql\n");
}
END_TEST

/**
 * Test that migrate fails if no migrations are present.
 */
START_TEST(migrate_no_migrations)
{
	char *argv[1] = { xmigrate };

	*errbuf = '\0';
	state_get_current_returns = "xxx";
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	ck_assert_int_eq(run_command("migrate", 1, argv), EXIT_SUCCESS);
	ck_assert_str_eq(errbuf, "migrate: no migrations found\n");
}
END_TEST

/**
 * Test that migrate fails if the source doesn't report the
 * migration path.
 */
START_TEST(migrate_no_migration_path)
{
	char **migs;
	char *argv[1] = { xmigrate };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = NULL;

	ck_assert_int_eq(run_command("migrate", 1, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, "migrate: unable to get migration path\n");
}
END_TEST

/**
 * Test that the migrate command issues the proper error message
 * if the BEGIN command fails.
 */
START_TEST(migrate_begin_fails)
{
	char **migs;
	char *argv[1] = { xmigrate };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	db_query_begin_fails = 1;

	ck_assert_int_eq(run_command("migrate", 1, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, "migrate: failed to BEGIN transaction\n");
	ck_assert_int_eq(db_query_called, 1);
	ck_assert(!source_get_local_head_called);
}
END_TEST

/**
 * Test that the migrate command issues the proper error message
 * if the migration fails to run.
 */
START_TEST(migrate_upgrade_fails)
{
	char **migs;
	char *argv[1] = { xmigrate };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_upgrade_returns = 1;
	db_has_transactional_ddl_returns = 1;

	ck_assert_int_eq(run_command("migrate", 1, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, " FAILED\n");
	ck_assert_int_eq(db_query_called, 2);
	ck_assert(!source_get_local_head_called);
}
END_TEST

/**
 * Test that the migrate command issues the proper error message
 * if the ROLLBACK command fails.
 */
START_TEST(migrate_rollback_fails)
{
	char **migs;
	char *argv[1] = { xmigrate };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_upgrade_returns = 1;
	db_has_transactional_ddl_returns = 1;
	db_query_rollback_fails = 1;

	ck_assert_int_eq(run_command("migrate", 1, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, "migrate: failed to ROLLBACK transaction\n");
	ck_assert_int_eq(db_query_called, 2);
	ck_assert(!source_get_local_head_called);
}
END_TEST

/**
 * Test that the migrate command issues the proper error message
 * if a migration fails, and the underlying database engine doesn't
 * support transactional DDL.
 */
START_TEST(migrate_rollback_no_transactional_ddl)
{
	char **migs;
	char *argv[1] = { xmigrate };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_upgrade_returns = 1;
	db_has_transactional_ddl_returns = 0;
	db_query_rollback_fails = 0;
	migration_downgrade_returns = 0;

	ck_assert_int_eq(run_command("migrate", 1, argv), EXIT_FAILURE);
	ck_assert_mem_eq(errbuf, "migrate: your database", 22);
	ck_assert_int_eq(db_query_called, 2);
	ck_assert(!source_get_local_head_called);
}
END_TEST

/**
 * Test that the migrate command rolls back applied migrations
 * if a migration fails, and the underlying database engine doesn't
 * support transactional DDL.
 */
START_TEST(migrate_rollback_no_transactional_ddl_2)
{
	char **migs;
	char *argv[1] = { xmigrate };

	*errbuf = '\0';
	migs    = malloc(sizeof(char *) * 3);
	migs[0] = my_strdup("test.sql");
	migs[1] = my_strdup("test2.sql");
	migs[2] = my_strdup("test3.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 3;
	source_get_migration_path_returns = xtmp;
	migration_upgrade_returns = 3;
	db_has_transactional_ddl_returns = 0;
	db_query_rollback_fails = 0;
	migration_downgrade_returns = 2;

	ck_assert_int_eq(run_command("migrate", 1, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, " FAILED\n");
	ck_assert_int_eq(db_query_called, 2);
	ck_assert(!source_get_local_head_called);
}
END_TEST

/**
 * Test that the migrate command issues the proper error message
 * if the COMMIT command fails.
 */
START_TEST(migrate_commit_fails)
{
	char **migs;
	char *argv[1] = { xmigrate };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_upgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	db_query_commit_fails = 1;

	ck_assert_int_eq(run_command("migrate", 1, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, "migrate: failed to COMMIT transaction\n");
	ck_assert_int_eq(db_query_called, 2);
	ck_assert(!source_get_local_head_called);
}
END_TEST

/**
 * Test that the migrate command fails if state_add_revision()
 * fails.
 */
START_TEST(migrate_add_revision_fails)
{
	char **migs;
	char *argv[1] = { xmigrate };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	source_get_local_head_returns = xtest;
	migration_upgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 1;

	ck_assert_int_eq(run_command("migrate", 1, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, "migrate: unable to set current revision\n");
	ck_assert_int_eq(db_query_called, 2);
	ck_assert(!!source_get_local_head_called);
}
END_TEST

/**
 * Test that the migrate command fails if state_cleanup_table()
 * fails.
 */
START_TEST(migrate_cleanup_table_fails)
{
	char **migs;
	char *argv[1] = { xmigrate };

	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = my_strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_upgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 0;
	state_cleanup_table_returns = 1;

	ck_assert_int_eq(run_command("migrate", 1, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, "migrate: unable to set current revision\n");
	ck_assert_int_eq(db_query_called, 2);
	ck_assert(!!source_get_local_head_called);
}
END_TEST

/**
 * Test that the migrate command works.
 */
START_TEST(test_migrate)
{
	char **migs;
	char *argv[1] = { xmigrate };

	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_upgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 0;
	state_cleanup_table_returns = 0;

	ck_assert_int_eq(run_command("migrate", 1, argv), EXIT_SUCCESS);
	ck_assert_int_eq(db_query_called, 2);
	ck_assert(!!source_get_local_head_called);
}
END_TEST

/**
 * Test that rollback defaults to the current revision's
 * previous revision if not specified.
 */
START_TEST(rollback_to_previous)
{
	char *argv[1] = { xrollback };

	*errbuf = '\0';
	state_get_current_returns = "xxx";
	state_get_previous_returns = "xxx";
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	ck_assert_int_eq(run_command("rollback", 1, argv), EXIT_SUCCESS);
	ck_assert_str_eq(errbuf, "rollback: nothing to roll back\n");
}
END_TEST

/**
 * Test that rollback fails if the current revision is specified
 * as the target for the rollback.
 */
START_TEST(rollback_no_target)
{
	char *argv[2] = { xrollback, xxx };

	*errbuf = '\0';
	state_get_current_returns = "xxx";
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	ck_assert_int_eq(run_command("rollback", 2, argv), EXIT_SUCCESS);
	ck_assert_str_eq(errbuf, "rollback: nothing to roll back\n");
}
END_TEST

/**
 * Test that rollback fails if no migrations are present.
 */
START_TEST(rollback_no_migrations)
{
	char *argv[2] = { xrollback, xxx };

	*errbuf = '\0';
	state_get_current_returns = "yyy";
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	ck_assert_int_eq(run_command("rollback", 2, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, "rollback: no migrations found\n");
}
END_TEST

/**
 * Test that rollback fails if the source doesn't report the
 * migration path.
 */
START_TEST(rollback_no_migration_path)
{
	char **migs;
	char *argv[2] = { xrollback, xxx };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = NULL;

	ck_assert_int_eq(run_command("rollback", 2, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, "rollback: unable to get migration path\n");
}
END_TEST

/**
 * Test that the rollback command issues the proper error message
 * if the BEGIN command fails.
 */
START_TEST(rollback_begin_fails)
{
	char **migs;
	char *argv[2] = { xrollback, xxx };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	db_query_begin_fails = 1;

	ck_assert_int_eq(run_command("rollback", 2, argv), EXIT_FAILURE);
	ck_assert_int_eq(db_query_called, 1);
	ck_assert_str_eq(errbuf, "rollback: failed to BEGIN transaction\n");
}
END_TEST

/**
 * Test that the rollback command issues the proper error message
 * if the migration fails to run.
 */
START_TEST(rollback_downgrade_fails)
{
	char **migs;
	char *argv[2] = { xrollback, xxx };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_downgrade_returns = 1;
	db_has_transactional_ddl_returns = 1;

	ck_assert_int_eq(run_command("rollback", 2, argv), EXIT_FAILURE);
	ck_assert_int_eq(db_query_called, 2);
	ck_assert_str_eq(errbuf, " FAILED\n");
}
END_TEST

/**
 * Test that the rollback command issues the proper error message
 * if a downgrade fails, and the underlying database engine doesn't
 * support transactional DDL.
 */
START_TEST(rollback_downgrade_no_transactional_ddl)
{
	char **migs;
	char *argv[2] = { xrollback, xxx };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *) * 2);
	*migs = my_strdup("test.sql");
	migs[1] = my_strdup("test2.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 2;
	source_get_migration_path_returns = xtmp;
	migration_downgrade_returns = 2;
	db_has_transactional_ddl_returns = 0;
	db_query_rollback_fails = 0;

	ck_assert_int_eq(run_command("rollback", 2, argv), EXIT_FAILURE);
	ck_assert_int_eq(db_query_called, 2);
	ck_assert_mem_eq(errbuf, "rollback: your database", 23);
}
END_TEST

/**
 * Test that the rollback command issues the proper error message
 * if the ROLLBACK command fails.
 */
START_TEST(rollback_rollback_fails)
{
	char **migs;
	char *argv[2] = { xrollback, xxx };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_downgrade_returns = 1;
	db_has_transactional_ddl_returns = 1;
	db_query_rollback_fails = 1;

	ck_assert_int_eq(run_command("rollback", 2, argv), EXIT_FAILURE);
	ck_assert_int_eq(db_query_called, 2);
	ck_assert_str_eq(errbuf, "rollback: failed to ROLLBACK transaction\n");
}
END_TEST

/**
 * Test that the rollback command issues the proper error message
 * if the COMMIT command fails.
 */
START_TEST(rollback_commit_fails)
{
	char **migs;
	char *argv[2] = { xrollback, xxx };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_downgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	db_query_commit_fails = 1;

	ck_assert_int_eq(run_command("rollback", 2, argv), EXIT_FAILURE);
	ck_assert_int_eq(db_query_called, 2);
	ck_assert_str_eq(errbuf, "rollback: failed to COMMIT transaction\n");
}
END_TEST

/**
 * Test that the rollback command fails if state_add_revision()
 * fails.
 */
START_TEST(rollback_add_revision_fails)
{
	char **migs;
	char *argv[2] = { xrollback, xxx };

	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_downgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 1;

	ck_assert_int_eq(run_command("rollback", 2, argv), EXIT_FAILURE);
	ck_assert_int_eq(db_query_called, 2);
}
END_TEST

/**
 * Test that the rollback command fails if state_cleanup_table()
 * fails.
 */
START_TEST(rollback_cleanup_table_fails)
{
	char **migs;
	char *argv[2] = { xrollback, xxx };

	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_downgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 0;
	state_cleanup_table_returns = 1;

	ck_assert_int_eq(run_command("rollback", 2, argv), EXIT_FAILURE);
	ck_assert_int_eq(db_query_called, 2);
}
END_TEST

/**
 * Test that the rollback command works.
 */
START_TEST(test_rollback)
{
	char **migs;
	char *argv[2] = { xrollback, xxx };

	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = xtmp;
	migration_downgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 0;
	state_cleanup_table_returns = 0;

	ck_assert_int_eq(run_command("rollback", 2, argv), EXIT_SUCCESS);
	ck_assert_int_eq(db_query_called, 2);
}
END_TEST

/**
 * Test that the assimilate command issues the appropriate
 * error message if state_create() fails.
 */
START_TEST(assimilate_state_create_fails)
{
	char *argv[1] = { xassimilate };

	*errbuf = '\0';
	state_create_returns = 1;

	ck_assert_int_eq(run_command("assimilate", 1, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, "assimilate: unable to create state table\n");
}
END_TEST

/**
 * Test that the assimilate command issues the appropriate
 * error message if state_add_revision() fails.
 */
START_TEST(assimilate_add_revision_fails)
{
	char **migs;
	char *argv[1] = { xassimilate };

	*errbuf = '\0';
	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	state_create_returns = 0;
	state_add_revision_returns = 1;

	ck_assert_int_eq(run_command("assimilate", 1, argv), EXIT_FAILURE);
	ck_assert_str_eq(errbuf, "assimilate: unable to set current revision\n");
	ck_assert(!!state_destroy_called);
}
END_TEST

/**
 * Test that the assimilate command works.
 */
START_TEST(test_assimilate)
{
	char **migs;
	char *argv[1] = { xassimilate };

	migs  = malloc(sizeof(char *));
	*migs = my_strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	state_create_returns = 0;
	state_add_revision_returns = 0;

	ck_assert_int_eq(run_command("assimilate", 1, argv), EXIT_SUCCESS);
	ck_assert(!state_destroy_called);
}
END_TEST

Suite *commands_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("Command Processing");
	t = tcase_create("run_command");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, run_command_invalid_args);
	tcase_add_test(t, run_command_not_found);
	tcase_add_test(t, run_command_insufficient_args);
	tcase_add_test(t, run_command_no_current_revision);
	tcase_add_test(t, test_run_command);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("head");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, head_no_local_head);
	tcase_add_test(t, test_head);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("seed");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, seed_cant_map_file);
	tcase_add_test(t, seed_query_fails);
	tcase_add_test(t, seed_creating_state_fails);
	tcase_add_test(t, test_seed);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("pending");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, pending_no_migrations);
	tcase_add_test(t, test_pending);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("migrate");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, migrate_no_migrations);
	tcase_add_test(t, migrate_no_migration_path);
	tcase_add_test(t, migrate_begin_fails);
	tcase_add_test(t, migrate_upgrade_fails);
	tcase_add_test(t, migrate_rollback_no_transactional_ddl);
	tcase_add_test(t, migrate_rollback_no_transactional_ddl_2);
	tcase_add_test(t, migrate_rollback_fails);
	tcase_add_test(t, migrate_commit_fails);
	tcase_add_test(t, migrate_add_revision_fails);
	tcase_add_test(t, migrate_cleanup_table_fails);
	tcase_add_test(t, test_migrate);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("rollback");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, rollback_to_previous);
	tcase_add_test(t, rollback_no_target);
	tcase_add_test(t, rollback_no_migrations);
	tcase_add_test(t, rollback_no_migration_path);
	tcase_add_test(t, rollback_begin_fails);
	tcase_add_test(t, rollback_downgrade_fails);
	tcase_add_test(t, rollback_downgrade_no_transactional_ddl);
	tcase_add_test(t, rollback_rollback_fails);
	tcase_add_test(t, rollback_commit_fails);
	tcase_add_test(t, rollback_add_revision_fails);
	tcase_add_test(t, rollback_cleanup_table_fails);
	tcase_add_test(t, test_rollback);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("assimilate");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, assimilate_state_create_fails);
	tcase_add_test(t, assimilate_add_revision_fails);
	tcase_add_test(t, test_assimilate);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);
	return s;
}

