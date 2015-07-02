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
#include <CUnit/CUnit.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

/* {{{ DB / file stubs */
static int db_query(const char *query, void *cb, void *userdata);
static char *map_file(const char *path, size_t *size);
static void unmap_file(void);

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
static int db_query(const char *query, void *UNUSED(cb),
                    void *UNUSED(userdata))
{
	++db_query_called;
	if (!query || !strlen(query))
		return 1;

	/* Check the query */
	if (expected_query)
		CU_ASSERT_STRING_EQUAL_FATAL(query, expected_query);
	return 0;
}

static char *map_file(const char *UNUSED(path), size_t *size)
{
	*size = map_file_returns_size;
	return map_file_returns;
}

static void unmap_file(void)
{
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
static void migration_upgrade_map_file_fails(void)
{
	db_query_called = 0;
	map_file_returns = NULL;
	map_file_returns_size = 0;
	CU_ASSERT_EQUAL(1, migration_upgrade("test"));
	CU_ASSERT_FALSE(db_query_called);
}

/**
 * Test the behavior of migration_upgrade() when the migration
 * only contains an "up" portion.
 */
static void migration_upgrade_up_only(void)
{
	db_query_called = 0;
	map_file_returns = migration_up_only;
	map_file_returns_size = strlen(migration_up_only);
	expected_query = migration_up_only + up_len + 1;
	CU_ASSERT_EQUAL(0, migration_upgrade("test"));
	CU_ASSERT_TRUE(db_query_called);
}

/**
 * Test the behavior of migration_upgrade() when the migration
 * only contains an "down" portion.
 */
static void migration_upgrade_down_only(void)
{
	db_query_called = 0;
	map_file_returns = migration_down_only;
	map_file_returns_size = strlen(migration_down_only);
	expected_query = NULL;
	CU_ASSERT_EQUAL(0, migration_upgrade("test"));
	CU_ASSERT_FALSE(db_query_called);
}

/**
 * Test that migration_upgrade() works independent of having
 * whitespace between the "up" and "down" portions.
 */
static void migration_upgrade_no_space_before_down(void)
{
	db_query_called = 0;
	map_file_returns = migration_up_works_no_space;
	map_file_returns_size = strlen(migration_up_works_no_space);
	expected_query = migration_up_down_expected_query_up;
	CU_ASSERT_EQUAL(0, migration_upgrade("test"));
	CU_ASSERT_TRUE(db_query_called);
}

/**
 * Test that migration_upgrade() works.
 */
static void test_migration_upgrade(void)
{
	db_query_called = 0;
	map_file_returns = migration_up_works;
	map_file_returns_size = strlen(migration_up_works);
	expected_query = migration_up_down_expected_query_up;
	CU_ASSERT_EQUAL(0, migration_upgrade("test"));
	CU_ASSERT_TRUE(db_query_called);
}

/**
 * Test the behavior of migration_downgrade() when map_file()
 * fails.
 */
static void migration_downgrade_map_file_fails(void)
{
	db_query_called = 0;
	map_file_returns = NULL;
	map_file_returns_size = 0;
	CU_ASSERT_EQUAL(1, migration_downgrade("test"));
	CU_ASSERT_FALSE(db_query_called);
}

/**
 * Test the behavior of migration_downgrade() when the migration
 * only contains an "up" portion.
 */
static void migration_downgrade_up_only(void)
{
	db_query_called = 0;
	map_file_returns = migration_up_only;
	map_file_returns_size = strlen(migration_up_only);
	expected_query = NULL;
	CU_ASSERT_EQUAL(0, migration_downgrade("test"));
	CU_ASSERT_FALSE(db_query_called);
}

/**
 * Test the behavior of migration_downgrade() when the migration
 * only contains an "down" portion.
 */
static void migration_downgrade_down_only(void)
{
	db_query_called = 0;
	map_file_returns = migration_down_only;
	map_file_returns_size = strlen(migration_down_only);
	expected_query = migration_down_only + down_len + 1;
	CU_ASSERT_EQUAL(0, migration_downgrade("test"));
	CU_ASSERT_TRUE(db_query_called);
}

/**
 * Test that migration_downgrade() works independent of having
 * whitespace between the "up" and "down" portions.
 */
static void migration_downgrade_no_space_before_up(void)
{
	db_query_called = 0;
	map_file_returns = migration_down_works_no_space;
	map_file_returns_size = strlen(migration_down_works_no_space);
	expected_query = migration_up_down_expected_query_down;
	CU_ASSERT_EQUAL(0, migration_downgrade("test"));
	CU_ASSERT_TRUE(db_query_called);
}

/**
 * Test that migration_downgrade() works.
 */
static void test_migration_downgrade(void)
{
	db_query_called = 0;
	map_file_returns = migration_down_works;
	map_file_returns_size = strlen(migration_down_works);
	expected_query = migration_up_down_expected_query_down;
	CU_ASSERT_EQUAL(0, migration_downgrade("test"));
	CU_ASSERT_TRUE(db_query_called);
}

static CU_TestInfo migration_tests[] = {
	{
		"migration_upgrade() - map_file() fails",
		migration_upgrade_map_file_fails
	},
	{
		"migration_upgrade() - up only",
		migration_upgrade_up_only
	},
	{
		"migration_upgrade() - down only",
		migration_upgrade_down_only
	},
	{
		"migration_upgrade() - no space before down",
		migration_upgrade_no_space_before_down
	},
	{
		"migration_upgrade() - works",
		test_migration_upgrade
	},
	{
		"migration_downgrade() - map_file() fails",
		migration_downgrade_map_file_fails
	},
	{
		"migration_downgrade() - up only",
		migration_downgrade_up_only
	},
	{
		"migration_downgrade() - down only",
		migration_downgrade_down_only
	},
	{
		"migration_downgrade() - no space before up",
		migration_downgrade_no_space_before_up
	},
	{
		"migration_downgrade() - works",
		test_migration_downgrade
	},

	CU_TEST_INFO_NULL
};

void migration_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Migration Handling", NULL, NULL);
	while (migration_tests[i].pName) {
		CU_add_test(suite, migration_tests[i].pName,
		            migration_tests[i].pTestFunc);
		i++;
	}
}

