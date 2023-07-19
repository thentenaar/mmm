/**
 * Minimal Migration Manager - Migration Handling Tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <check.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

/* {{{ DB / file stubs */
static int db_query(const char *query, void *cb, void *userdata);
static char *map_file(const char *path, size_t *size);
static void unmap_file(char *mem, size_t size);

#define DB_H
#define FILE_H
#include "../src/migration.h"
#include "../src/migration.c"

static const char *expected_query = NULL;
static size_t map_file_returns_size = 0;
static char *map_file_returns = NULL;
static int db_query_called = 0;

/**
 * Database query stub
 */
static int db_query(const char *query, void *cb, void *userdata)
{
	(void)cb;
	(void)userdata;
	++db_query_called;
	if (!query || !strlen(query))
		return 1;

	/* Check the query */
	if (expected_query)
		ck_assert_str_eq(query, expected_query);
	return 0;
}

static char *map_file(const char *path, size_t *size)
{
	(void)path;
	*size = map_file_returns_size;
	return map_file_returns;
}

static void unmap_file(char *mem, size_t size)
{
	(void)mem;
	(void)size;
	return;
}

/* }}} */

/* {{{ migration test data */
static char migration_up_only[] =
	"-- [up]\n"
	"CREATE TABLE test(id INTEGER);";

static char migration_down_only[] =
	"-- [down]\n"
	"DROP TABLE test;";

static char migration_up_works[] =
	"-- [up]\n"
	"CREATE TABLE test(xxx VARCHAR(5));\n\n"
	"-- [down]\n"
	"DROP TABLE test;";

static char migration_up_works_no_space[] =
	"-- [up]\n"
	"CREATE TABLE test(xxx VARCHAR(5));"
	"-- [down]\n"
	"DROP TABLE test;";

static char migration_down_works[] =
	"-- [up]\n"
	"CREATE TABLE test(xxx VARCHAR(5));\n\n"
	"-- [down]\n"
	"DROP TABLE test;";

static char migration_down_works_no_space[] =
	"-- [down]\n"
	"DROP TABLE test;"
	"-- [up]\n"
	"CREATE TABLE test(xxx VARCHAR(5));";

static char migration_up_down_expected_query_up[] =
	"CREATE TABLE test(xxx VARCHAR(5));";

static char migration_up_down_expected_query_down[] =
	"DROP TABLE test;";
/* }}} */

/**
 * Test the behavior of migration_upgrade() when map_file()
 * fails.
 */
START_TEST(migration_upgrade_map_file_fails)
{
	db_query_called       = 0;
	map_file_returns      = NULL;
	map_file_returns_size = 0;
	ck_assert_int_ne(migration_upgrade("test"), 0);
	ck_assert(!db_query_called);
}
END_TEST

/**
 * Test the behavior of migration_upgrade() when the migration
 * only contains an "up" portion.
 */
START_TEST(migration_upgrade_up_only)
{
	db_query_called       = 0;
	map_file_returns      = migration_up_only;
	map_file_returns_size = strlen(migration_up_only);
	expected_query        = migration_up_only + up_len + 1;
	ck_assert_int_eq(migration_upgrade("test"), 0);
	ck_assert(db_query_called);
}
END_TEST

/**
 * Test the behavior of migration_upgrade() when the migration
 * only contains a "down" portion.
 */
START_TEST(migration_upgrade_down_only)
{
	db_query_called       = 0;
	map_file_returns      = migration_down_only;
	map_file_returns_size = strlen(migration_down_only);
	expected_query        = NULL;
	ck_assert_int_eq(migration_upgrade("test"), 0);
	ck_assert(!db_query_called);
}
END_TEST

/**
 * Test that migration_upgrade() works independent of having
 * whitespace between the "up" and "down" portions.
 */
START_TEST(migration_upgrade_no_space_before_down)
{
	db_query_called       = 0;
	map_file_returns      = migration_up_works_no_space;
	map_file_returns_size = strlen(migration_up_works_no_space);
	expected_query        = migration_up_down_expected_query_up;
	ck_assert_int_eq(migration_upgrade("test"), 0);
	ck_assert(db_query_called);
}
END_TEST

/**
 * Test that migration_upgrade() works.
 */
START_TEST(test_migration_upgrade)
{
	db_query_called       = 0;
	map_file_returns      = migration_up_works;
	map_file_returns_size = strlen(migration_up_works);
	expected_query        = migration_up_down_expected_query_up;
	ck_assert_int_eq(migration_upgrade("test"), 0);
	ck_assert(db_query_called);
}
END_TEST

/**
 * Test the behavior of migration_downgrade() when map_file()
 * fails.
 */
START_TEST(migration_downgrade_map_file_fails)
{
	db_query_called       = 0;
	map_file_returns      = NULL;
	map_file_returns_size = 0;
	ck_assert_int_ne(migration_downgrade("test"), 0);
	ck_assert(!db_query_called);
}
END_TEST

/**
 * Test the behavior of migration_downgrade() when the migration
 * only contains an "up" portion.
 */
START_TEST(migration_downgrade_up_only)
{
	db_query_called       = 0;
	map_file_returns      = migration_up_only;
	map_file_returns_size = strlen(migration_up_only);
	expected_query        = NULL;

	ck_assert_int_eq(migration_downgrade("test"), 0);
	ck_assert(!db_query_called);
}
END_TEST

/**
 * Test the behavior of migration_downgrade() when the migration
 * only contains an "down" portion.
 */
START_TEST(migration_downgrade_down_only)
{
	db_query_called       = 0;
	map_file_returns      = migration_down_only;
	map_file_returns_size = strlen(migration_down_only);
	expected_query        = migration_down_only + down_len + 1;
	ck_assert_int_eq(migration_downgrade("test"), 0);
	ck_assert(db_query_called);
}
END_TEST

/**
 * Test that migration_downgrade() works independent of having
 * whitespace between the "up" and "down" portions.
 */
START_TEST(migration_downgrade_no_space_before_up)
{
	db_query_called       = 0;
	map_file_returns      = migration_down_works_no_space;
	map_file_returns_size = strlen(migration_down_works_no_space);
	expected_query        = migration_up_down_expected_query_down;
	ck_assert_int_eq(migration_downgrade("test"), 0);
	ck_assert(db_query_called);
}
END_TEST

/**
 * Test that migration_downgrade() works.
 */
START_TEST(test_migration_downgrade)
{
	db_query_called       = 0;
	map_file_returns      = migration_down_works;
	map_file_returns_size = strlen(migration_down_works);
	expected_query        = migration_up_down_expected_query_down;
	ck_assert_int_eq(migration_downgrade("test"), 0);
	ck_assert(db_query_called);
}
END_TEST

Suite *migration_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("Migration Handling");
	t = tcase_create("migration_upgrade");
	tcase_add_test(t, migration_upgrade_map_file_fails);
	tcase_add_test(t, migration_upgrade_up_only);
	tcase_add_test(t, migration_upgrade_down_only);
	tcase_add_test(t, migration_upgrade_no_space_before_down);
	tcase_add_test(t, test_migration_upgrade);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("migration_downgrade");
	tcase_add_test(t, migration_downgrade_map_file_fails);
	tcase_add_test(t, migration_downgrade_up_only);
	tcase_add_test(t, migration_downgrade_down_only);
	tcase_add_test(t, migration_downgrade_no_space_before_up);
	tcase_add_test(t, test_migration_downgrade);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

