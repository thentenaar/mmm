/**
 * Minimal Migration Manager - Command Processing Tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <CUnit/CUnit.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

/* {{{ stubs */
typedef int (*db_row_callback_t)(void *userdata, int n_cols,
                                 char **fields, char **column_names);

static char *map_file(const char *path, size_t *size);
static void unmap_file(void);
static int db_query(const char *query, db_row_callback_t UNUSED(cb),
                    void *UNUSED(userdata));
static int db_has_transactional_ddl(void);
static int state_create(void);
static const char *state_get_current(void);
static const char *state_get_previous(void);
static int state_cleanup_table(void);
static int state_add_revision(const char *UNUSED(rev));
static int state_destroy(void);
static char **source_find_migrations(const char *UNUSED(source),
                                     const char *UNUSED(cur_rev),
                                     const char *UNUSED(prev_rev),
                                     size_t *size);
static const char *source_get_local_head(const char *UNUSED(source));
static const char *source_get_migration_path(const char *UNUSED(source));
static int migration_upgrade(const char *UNUSED(path));
static int migration_downgrade(const char *UNUSED(path));

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

static char *map_file(const char *UNUSED(path), size_t *size)
{
	++map_file_called;
	*size = map_file_returns_size;
	return map_file_returns;
}

static void unmap_file(void)
{
	++unmap_file_called;
	return;
}

static int db_has_transactional_ddl(void)
{
	++db_has_transactional_ddl_called;
	return db_has_transactional_ddl_returns;
}

static int db_query(const char *query, db_row_callback_t UNUSED(cb),
                    void *UNUSED(userdata))
{
	int retval = db_query_returns;

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

static int state_add_revision(const char *UNUSED(rev))
{
	++state_add_revision_called;
	return state_add_revision_returns;
}

static int state_destroy(void)
{
	++state_destroy_called;
	return state_destroy_returns;
}

static char **source_find_migrations(const char *UNUSED(source),
                                     const char *UNUSED(cur_rev),
                                     const char *UNUSED(prev_rev),
                                     size_t *size)
{
	++source_find_migrations_called;
	if (size) *size = source_find_migrations_returns_size;
	return source_find_migrations_returns;
}

static const char *source_get_local_head(const char *UNUSED(source))
{
	++source_get_local_head_called;
	return source_get_local_head_returns;
}

static const char *source_get_migration_path(const char *UNUSED(source))
{
	++source_get_migration_path_called;
	return source_get_migration_path_returns;
}

static int migration_upgrade(const char *UNUSED(path))
{
	++migration_upgrade_called;
	if (migration_upgrade_returns > 1) {
		--migration_upgrade_returns;
		return 0;
	}

	return migration_upgrade_returns;
}

static int migration_downgrade(const char *UNUSED(path))
{
	++migration_downgrade_called;
	if (migration_downgrade_returns > 1) {
		--migration_downgrade_returns;
		return 0;
	}

	return migration_downgrade_returns;
}

/* }}} */

static char *query = "query";

/**
 * Test that head prints nothing if no local_head is
 * given by the source.
 */
static void head_no_local_head(void)
{
	char *argv[1] = { "head" };

	reset_stubs();
	*errbuf = '\0';
	state_get_current_returns = "xxx";
	source_get_local_head_returns = NULL;
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	CU_ASSERT_EQUAL(EXIT_SUCCESS, run_command("head", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "\0");
}

/**
 * Test that head works.
 */
static void test_head(void)
{

	char **migs;
	char *argv[1] = { "head" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_local_head_returns = "yyy";

	CU_ASSERT_EQUAL(EXIT_SUCCESS, run_command("head", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "yyy\n");
}

/**
 * Test that run_command() behaves correctly when argc is 0.
 */
static void run_command_argc_0(void)
{
	char *argv[1] = { "test" };

	reset_stubs();
	CU_ASSERT_EQUAL(COMMAND_INVALID_ARGS, run_command("s", 0, argv));
}

/**
 * Test that run_command() behaves correctly when argv is NULL.
 */
static void run_command_argv_null(void)
{
	reset_stubs();
	CU_ASSERT_EQUAL(COMMAND_INVALID_ARGS, run_command("s", 1, NULL));
}

/**
 * Test that run_command() behaves correctly when argv[0] is NULL.
 */
static void run_command_argv0_null(void)
{
	char *argv[1] = { NULL };

	reset_stubs();
	CU_ASSERT_EQUAL(COMMAND_INVALID_ARGS, run_command("s", 1, argv));
}

/**
 * Test that run_command() behaves correctly when argv[0] is empty.
 */
static void run_command_argv0_empty(void)
{
	char *argv[1] = { "\0" };

	reset_stubs();
	CU_ASSERT_EQUAL(COMMAND_INVALID_ARGS, run_command("s", 1, argv));
}

/**
 * Test that run_command() behaves correctly when argv is too long.
 */
static void run_command_argv0_too_long(void)
{
	char argv0[MAX_COMMAND_LEN + 2];
	char *argv[1];

	reset_stubs();
	memset(argv0, 'x', sizeof(argv0));
	argv0[sizeof(argv0) - 1] = '\0';
	argv[0] = argv0;

	CU_ASSERT_EQUAL(COMMAND_INVALID_ARGS, run_command("s", 1, argv));
}

/**
 * Test that run_command() behaves correctly when the given command
 * isn't in the array of commands.
 */
static void run_command_not_found(void)
{
	char argv0[MAX_COMMAND_LEN - 1];
	char *argv[1];

	reset_stubs();
	memset(argv0, 'x', sizeof(argv0));
	argv0[sizeof(argv0) - 1] = '\0';
	argv[0] = argv0;

	CU_ASSERT_EQUAL(COMMAND_INVALID_ARGS, run_command("s", 1, argv));
}

/**
 * Test that run_command() behaves correctly when insufficient arguments
 * are passed for the command.
 */
static void run_command_insufficient_args(void)
{
	char *argv[1] = { "seed" };

	reset_stubs();
	CU_ASSERT_EQUAL(COMMAND_INVALID_ARGS, run_command("seed", 1, argv));
}

/**
 * Test that run_command() yields an error if the command needs the
 * current revision, and it was not loaded.
 */
static void run_command_no_current_revision(void)
{
	char *argv[1] = { "migrate" };

	reset_stubs();
	state_get_current_returns = NULL;
	CU_ASSERT_EQUAL(COMMAND_INVALID_ARGS,
	                run_command("migrate", 1, argv));
}

/**
 * Test that run_command() works.
 *
 * This also covers the case for seed() where argv[0]
 * is NULL.
 */
static void test_run_command(void)
{
	char *argv[2] = { "seed", NULL };

	reset_stubs();
	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("seed", 2, argv));
}

/**
 * Test that the seed command returns EXIT_FAILURE if
 * the specifiied file cannot be mapped.
 */
static void seed_cant_map_file(void)
{
	char *argv[2] = { "seed", "test.sql" };

	reset_stubs();
	map_file_returns = NULL;
	map_file_returns_size = 0;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("seed", 2, argv));
}

/**
 * Test that the seed command returns EXIT_FAILURE if
 * running the seed file fails.
 */
static void seed_query_fails(void)
{
	char *argv[2] = { "seed", "test.sql" };

	reset_stubs();
	map_file_returns = query;
	map_file_returns_size = 1;
	db_query_returns = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("seed", 2, argv));
}

/**
 * Test that the seed command returns EXIT_FAILURE if
 * creating the state table fails.
 */
static void seed_creating_state_fails(void)
{
	char *argv[2] = { "seed", "test.sql" };

	reset_stubs();
	map_file_returns = query;
	map_file_returns_size = 1;
	db_query_returns = 0;
	state_create_returns = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("seed", 2, argv));
}

/**
 * Test that the seed command works.
 */
static void test_seed(void)
{
	char *argv[2] = { "seed", "test.sql" };

	reset_stubs();
	map_file_returns = query;
	map_file_returns_size = 1;
	db_query_returns = 0;
	state_create_returns = 0;

	CU_ASSERT_EQUAL(EXIT_SUCCESS, run_command("seed", 2, argv));
}

/**
 * Test the pending command when no migrations are present.
 */
static void pending_no_migrations(void)
{
	char *argv[1] = { "pending" };

	reset_stubs();
	*errbuf = '\0';
	state_get_current_returns = "xxx";
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	CU_ASSERT_EQUAL(EXIT_SUCCESS, run_command("pending", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "0 migrations pending:\n");
}

/**
 * Test that pending correctly displays pending migrations.
 */
static void test_pending(void)
{
	char **migs;
	char *argv[1] = { "pending" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;

	CU_ASSERT_EQUAL(EXIT_SUCCESS, run_command("pending", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "  + test.sql\n");
}

/**
 * Test that migrate fails if no migrations are present.
 */
static void migrate_no_migrations(void)
{
	char *argv[1] = { "migrate" };

	reset_stubs();
	*errbuf = '\0';
	state_get_current_returns = "xxx";
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	CU_ASSERT_EQUAL(EXIT_SUCCESS, run_command("migrate", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "migrate: no migrations found\n");
}

/**
 * Test that migrate fails if the source doesn't report the
 * migration path.
 */
static void migrate_no_migration_path(void)
{
	char **migs;
	char *argv[1] = { "migrate" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = NULL;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("migrate", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "migrate: unable to get migration "
	                               "path\n");

}

/**
 * Test that the migrate command issues the proper error message
 * if the BEGIN command fails.
 */
static void migrate_begin_fails(void)
{
	char **migs;
	char *argv[1] = { "migrate" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	db_query_begin_fails = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("migrate", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "migrate: failed to BEGIN "
	                               "transaction\n");
	CU_ASSERT_FALSE(source_get_local_head_called);
	CU_ASSERT_EQUAL(1, db_query_called);
}

/**
 * Test that the migrate command issues the proper error message
 * if the migration fails to run.
 */
static void migrate_upgrade_fails(void)
{
	char **migs;
	char *argv[1] = { "migrate" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_upgrade_returns = 1;
	db_has_transactional_ddl_returns = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("migrate", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, " FAILED\n");
	CU_ASSERT_FALSE(source_get_local_head_called);
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the migrate command issues the proper error message
 * if the ROLLBACK command fails.
 */
static void migrate_rollback_fails(void)
{
	char **migs;
	char *argv[1] = { "migrate" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_upgrade_returns = 1;
	db_has_transactional_ddl_returns = 1;
	db_query_rollback_fails = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("migrate", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "migrate: failed to ROLLBACK "
	                               "transaction\n");
	CU_ASSERT_FALSE(source_get_local_head_called);
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the migrate command issues the proper error message
 * if a migration fails, and the underlying database engine doesn't
 * support transactional DDL.
 */
static void migrate_rollback_no_transactional_ddl(void)
{
	char **migs;
	char *argv[1] = { "migrate" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_upgrade_returns = 1;
	db_has_transactional_ddl_returns = 0;
	db_query_rollback_fails = 0;
	migration_downgrade_returns = 0;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("migrate", 1, argv));
	CU_ASSERT_FALSE(memcmp(errbuf, "migrate: your database", 22));
	CU_ASSERT_FALSE(source_get_local_head_called);
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the migrate command rolls back applied migrations
 * if a migration fails, and the underlying database engine doesn't
 * support transactional DDL.
 */
static void migrate_rollback_no_transactional_ddl_2(void)
{
	char **migs;
	char *argv[1] = { "migrate" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *) * 3);
	migs[0] = strdup("test.sql");
	migs[1] = strdup("test2.sql");
	migs[2] = strdup("test3.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 3;
	source_get_migration_path_returns = "/tmp";
	migration_upgrade_returns = 3;
	db_has_transactional_ddl_returns = 0;
	db_query_rollback_fails = 0;
	migration_downgrade_returns = 2;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("migrate", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, " FAILED\n");
	CU_ASSERT_FALSE(source_get_local_head_called);
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the migrate command issues the proper error message
 * if the COMMIT command fails.
 */
static void migrate_commit_fails(void)
{
	char **migs;
	char *argv[1] = { "migrate" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_upgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	db_query_commit_fails = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("migrate", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "migrate: failed to COMMIT "
	                               "transaction\n");
	CU_ASSERT_FALSE(source_get_local_head_called);
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the migrate command fails if state_add_revision()
 * fails.
 */
static void migrate_add_revision_fails(void)
{
	char **migs;
	char *argv[1] = { "migrate" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	source_get_local_head_returns = "test";
	migration_upgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("migrate", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "migrate: unable to set current "
	                               "revision\n");
	CU_ASSERT_TRUE(source_get_local_head_called);
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the migrate command fails if state_cleanup_table()
 * fails.
 */
static void migrate_cleanup_table_fails(void)
{
	char **migs;
	char *argv[1] = { "migrate" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_upgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 0;
	state_cleanup_table_returns = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("migrate", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "migrate: unable to set current "
	                               "revision\n");
	CU_ASSERT_TRUE(source_get_local_head_called);
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the migrate command works.
 */
static void test_migrate(void)
{
	char **migs;
	char *argv[1] = { "migrate" };

	reset_stubs();
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "xxx";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_upgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 0;
	state_cleanup_table_returns = 0;

	CU_ASSERT_EQUAL(EXIT_SUCCESS, run_command("migrate", 1, argv));
	CU_ASSERT_TRUE(source_get_local_head_called);
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that rollback defaults to the current revision's
 * previous revision if not specified.
 */
static void rollback_to_previous(void)
{
	char *argv[1] = { "rollback" };

	reset_stubs();
	*errbuf = '\0';
	state_get_current_returns = "xxx";
	state_get_previous_returns = "xxx";
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	CU_ASSERT_EQUAL(EXIT_SUCCESS, run_command("rollback", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "rollback: nothing to roll back\n");
}

/**
 * Test that rollback fails if the current revision is specified
 * as the target for the rollback.
 */
static void rollback_no_target(void)
{
	char *argv[2] = { "rollback", "xxx" };

	reset_stubs();
	*errbuf = '\0';
	state_get_current_returns = "xxx";
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	CU_ASSERT_EQUAL(EXIT_SUCCESS, run_command("rollback", 2, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "rollback: nothing to roll back\n");
}

/**
 * Test that rollback fails if no migrations are present.
 */
static void rollback_no_migrations(void)
{
	char *argv[2] = { "rollback", "xxx" };

	reset_stubs();
	*errbuf = '\0';
	state_get_current_returns = "yyy";
	source_find_migrations_returns = NULL;
	source_find_migrations_returns_size = 0;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("rollback", 2, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "rollback: no migrations found\n");
}

/**
 * Test that rollback fails if the source doesn't report the
 * migration path.
 */
static void rollback_no_migration_path(void)
{
	char **migs;
	char *argv[2] = { "rollback", "xxx" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = NULL;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("rollback", 2, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "rollback: unable to get migration "
	                               "path\n");

}

/**
 * Test that the rollback command issues the proper error message
 * if the BEGIN command fails.
 */
static void rollback_begin_fails(void)
{
	char **migs;
	char *argv[2] = { "rollback", "xxx" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	db_query_begin_fails = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("rollback", 2, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "rollback: failed to BEGIN "
	                               "transaction\n");
	CU_ASSERT_EQUAL(1, db_query_called);
}

/**
 * Test that the rollback command issues the proper error message
 * if the migration fails to run.
 */
static void rollback_downgrade_fails(void)
{
	char **migs;
	char *argv[2] = { "rollback", "xxx" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_downgrade_returns = 1;
	db_has_transactional_ddl_returns = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("rollback", 2, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, " FAILED\n");
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the rollback command issues the proper error message
 * if a downgrade fails, and the underlying database engine doesn't
 * support transactional DDL.
 */
static void rollback_downgrade_no_transactional_ddl(void)
{
	char **migs;
	char *argv[2] = { "rollback", "xxx" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *) * 2);
	migs[0] = strdup("test.sql");
	migs[1] = strdup("test2.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 2;
	source_get_migration_path_returns = "/tmp";
	migration_downgrade_returns = 2;
	db_has_transactional_ddl_returns = 0;
	db_query_rollback_fails = 0;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("rollback", 2, argv));
	CU_ASSERT_FALSE(memcmp(errbuf, "rollback: your database", 23));
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the rollback command issues the proper error message
 * if the ROLLBACK command fails.
 */
static void rollback_rollback_fails(void)
{
	char **migs;
	char *argv[2] = { "rollback", "xxx" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_downgrade_returns = 1;
	db_has_transactional_ddl_returns = 1;
	db_query_rollback_fails = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("rollback", 2, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "rollback: failed to ROLLBACK "
	                               "transaction\n");
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the rollback command issues the proper error message
 * if the COMMIT command fails.
 */
static void rollback_commit_fails(void)
{
	char **migs;
	char *argv[2] = { "rollback", "xxx" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_downgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	db_query_commit_fails = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("rollback", 2, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "rollback: failed to COMMIT "
	                               "transaction\n");
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the rollback command fails if state_add_revision()
 * fails.
 */
static void rollback_add_revision_fails(void)
{
	char **migs;
	char *argv[2] = { "rollback", "xxx" };

	reset_stubs();
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_downgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("rollback", 2, argv));
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the rollback command fails if state_cleanup_table()
 * fails.
 */
static void rollback_cleanup_table_fails(void)
{
	char **migs;
	char *argv[2] = { "rollback", "xxx" };

	reset_stubs();
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_downgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 0;
	state_cleanup_table_returns = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("rollback", 2, argv));
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the rollback command works.
 */
static void test_rollback(void)
{
	char **migs;
	char *argv[2] = { "rollback", "xxx" };

	reset_stubs();
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	source_get_migration_path_returns = "/tmp";
	migration_downgrade_returns = 0;
	db_has_transactional_ddl_returns = 1;
	state_add_revision_returns = 0;
	state_cleanup_table_returns = 0;

	CU_ASSERT_EQUAL(EXIT_SUCCESS, run_command("rollback", 2, argv));
	CU_ASSERT_EQUAL(2, db_query_called);
}

/**
 * Test that the assimilate command issues the appropriate
 * error message if state_create() fails.
 */
static void assimilate_state_create_fails(void)
{
	char *argv[1] = { "assimilate" };

	reset_stubs();
	*errbuf = '\0';
	state_create_returns = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("assimilate", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "assimilate: unable to create "
	                               "state table\n");
}

/**
 * Test that the assimilate command issues the appropriate
 * error message if state_add_revision() fails.
 */
static void assimilate_add_revision_fails(void)
{
	char **migs;
	char *argv[1] = { "assimilate" };

	reset_stubs();
	*errbuf = '\0';
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	state_create_returns = 0;
	state_add_revision_returns = 1;

	CU_ASSERT_EQUAL(EXIT_FAILURE, run_command("assimilate", 1, argv));
	CU_ASSERT_STRING_EQUAL(errbuf, "assimilate: unable to set "
	                               "current revision\n");
	CU_ASSERT_TRUE(state_destroy_called);
}

/**
 * Test that the assimilate command works.
 */
static void test_assimilate(void)
{
	char **migs;
	char *argv[1] = { "assimilate" };

	reset_stubs();
	migs = malloc(sizeof(char *));
	migs[0] = strdup("test.sql");
	state_get_current_returns = "yyy";
	source_find_migrations_returns = migs;
	source_find_migrations_returns_size = 1;
	state_create_returns = 0;
	state_add_revision_returns = 0;

	CU_ASSERT_EQUAL(EXIT_SUCCESS, run_command("assimilate", 1, argv));
	CU_ASSERT_FALSE(state_destroy_called);
}

static CU_TestInfo commands_tests[] = {
	{
		"run_command - argc == 0",
		run_command_argc_0
	},
	{
		"run_command - argv NULL",
		run_command_argv_null
	},
	{
		"run_command - argv[0] NULL",
		run_command_argv0_null
	},
	{
		"run_command - argv[0] empty",
		run_command_argv0_empty
	},
	{
		"run_command - argv[0] too long",
		run_command_argv0_too_long
	},
	{
		"run_command - command not found",
		run_command_not_found
	},
	{
		"run_command - insufficient args",
		run_command_insufficient_args
	},
	{
		"run_command - no current revision",
		run_command_no_current_revision
	},
	{
		"run_command - works",
		test_run_command
	},
	{
		"head command - no local_head",
		head_no_local_head
	},
	{
		"head command - works",
		test_head
	},
	{
		"seed command - can't map file",
		seed_cant_map_file
	},
	{
		"seed command - query fails",
		seed_query_fails
	},
	{
		"seed command - can't create state",
		seed_creating_state_fails
	},
	{
		"seed command - works",
		test_seed
	},
	{
		"pending command - no migrations",
		pending_no_migrations
	},
	{
		"pending command - works",
		test_pending
	},
	{
		"migrate command - no migrations",
		migrate_no_migrations
	},
	{
		"migrate command - no migration path",
		migrate_no_migration_path
	},
	{
		"migrate command - BEGIN fails",
		migrate_begin_fails
	},
	{
		"migrate command - upgrade fails",
		migrate_upgrade_fails
	},
	{
		"migrate command - rollback - no transactional DDL",
		migrate_rollback_no_transactional_ddl
	},
	{
		"migrate command - rollback - no transactional DDL 2",
		migrate_rollback_no_transactional_ddl_2
	},
	{
		"migrate command - ROLLBACK fails",
		migrate_rollback_fails
	},
	{
		"migrate command - COMMIT fails",
		migrate_commit_fails
	},
	{
		"migrate command - state_add_revision() fails",
		migrate_add_revision_fails
	},
	{
		"migrate command - state_cleanup_table() fails",
		migrate_cleanup_table_fails
	},
	{
		"migrate command - works",
		test_migrate
	},
	{
		"rollback command - to previous revision",
		rollback_to_previous
	},
	{
		"rollback command - no target",
		rollback_no_target
	},
	{
		"rollback command - no migrations",
		rollback_no_migrations
	},
	{
		"rollback command - no migration path",
		rollback_no_migration_path
	},
	{
		"rollback command - BEGIN fails",
		rollback_begin_fails
	},
	{
		"rollback command - downgrade fails",
		rollback_downgrade_fails
	},
	{
		"rollback command - downgrade fails - no transactional DDL",
		rollback_downgrade_no_transactional_ddl
	},
	{
		"rollback command - ROLLBACK fails",
		rollback_rollback_fails
	},
	{
		"rollback command - COMMIT fails",
		rollback_commit_fails
	},
	{
		"rollback command - state_add_revision() fails",
		rollback_add_revision_fails
	},
	{
		"rollback command - state_cleanup_table() fails",
		rollback_cleanup_table_fails
	},
	{
		"rollback command - works",
		test_rollback
	},
	{
		"assimilate command - state_create() fails",
		assimilate_state_create_fails
	},
	{
		"assimilate command - state_add_revision() fails",
		assimilate_add_revision_fails
	},
	{
		"assimilate command - works",
		test_assimilate
	},

	CU_TEST_INFO_NULL
};

void commands_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Command Processing", NULL, NULL);
	while (commands_tests[i].pName) {
		CU_add_test(suite, commands_tests[i].pName,
		            commands_tests[i].pTestFunc);
		i++;
	}
}

