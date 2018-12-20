/**
 * Minimal Migration Manager - File Source Tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <CUnit/CUnit.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

#include "posix_stubs.h"
#include "../src/source/file.c"

/* {{{ Test cases for file_find_migrations() */

/* File after current head */
static struct dirent mig_after_head = {
	"100.sql",
	NULL
};

/* File before current head */
static struct dirent mig_before_head = {
	"1.sql",
	&mig_after_head
};

/* No .sql extension */
static struct dirent mig_no_sql = {
	"999999",
	&mig_before_head
};

/* No numeric designation */
static const char *err_no_num =
	"warning: 'yyyyyy' lacks a valid "
	"numeric designation\n";

static struct dirent mig_no_num = {
	"yyyyyy",
	&mig_no_sql
};

/* Filename too small */
static struct dirent mig_name_too_small = {
	"xxx",
	&mig_no_num
};

/* File with an empty name */
static struct dirent mig_empty_name = {
	"\0",
	NULL
};

/* Designation out of range */
const char *err_des_range =
	"warning: "
	"'999999999999999999999999999.sql' "
	"lacks a valid numeric designation\n";

static struct dirent mig_des_range = {
	"999999999999999999999999999.sql",
	NULL
};
/* }}} */

/**
 * Test that file_init() and file_uninit() work.
 */
static void test_file_init_uninit(void)
{
	reset_stubs();
	file_config();
	CU_ASSERT_EQUAL(0, file_init());
	CU_ASSERT_EQUAL(0, file_uninit());
}

/**
 * Test that file_get_head() works.
 */
static void test_file_get_head(void)
{
	local_head[0] = '\0';
	CU_ASSERT_PTR_NULL(file_get_head());
	memcpy(local_head, "1", 2);
	CU_ASSERT_PTR_NOT_NULL(file_get_head());
}

/**
 * Test that file_get_migration_path() works.
 */
static void test_file_get_migration_path(void)
{
	local_head[0] = '\0';
	CU_ASSERT_EQUAL(file_get_migration_path(), config.migration_path);
}

/**
 * Test that file_find_migrations() returns NULL if
 * size is NULL.
 */
static void file_find_migrations_null_size(void)
{
	reset_stubs();
	CU_ASSERT_PTR_NULL(file_find_migrations("1", "2", NULL));
}

/**
 * Test that file_find_migrations() returns NULL if no
 * migration_path was set.
 */
static void file_find_migrations_no_migration_path(void)
{
	size_t size = 9999;

	reset_stubs();
	config.migration_path[0] = '\0';
	CU_ASSERT_PTR_NULL(file_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
}

/**
 * Test that file_find_migrations() returns NULL if an
 * error occurs while scanning the migraiton_path.
 */
static void file_find_migrations_handles_scan_errors(void)
{
	size_t size = 9999;

	/* opendir() returns NULL */
	reset_stubs();
	opendir_returns = NULL;
	local_head[0] = '\0';
	memcpy(config.migration_path, "/tmp", 5);

	CU_ASSERT_PTR_NULL(file_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_EQUAL(local_head[0], '\0');
	CU_ASSERT_TRUE(opendir_called);
	CU_ASSERT_FALSE(closedir_called);
}

/**
 * Test that file_find_migrations() returns NULL if the
 * migration directory is empty.
 */
static void file_find_migrations_empty_dir(void)
{
	size_t size = 9999;

	reset_stubs();
	opendir_returns = (DIR *)1234;
	readdir_returns = NULL;
	local_head[0] = '\0';
	memcpy(config.migration_path, "/tmp", 5);

	CU_ASSERT_PTR_NULL(file_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_NOT_EQUAL(local_head[0], '\0');
	CU_ASSERT_STRING_EQUAL(local_head, "1");
	CU_ASSERT_TRUE(opendir_called && readdir_called);
	CU_ASSERT_TRUE(closedir_called);
}

/**
 * Test that file_find_migrations() doesn't skip files when
 * current_head isn't valid.
 */
static void file_find_migrations_invalid_current_head(void)
{
	char **m = NULL;
	size_t size;

	reset_stubs();
	opendir_returns = (DIR *)1234;
	readdir_returns = &mig_after_head;
	stat_returns_buf.st_mode = S_IFREG;
	memcpy(config.migration_path, "/tmp", 5);

	m = file_find_migrations("xxx", NULL, &size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_EQUAL(size, 1);
	CU_ASSERT_STRING_EQUAL(local_head, "100");
	CU_ASSERT_TRUE(opendir_called && readdir_called);
	CU_ASSERT_TRUE(closedir_called);
	free(m[0]);
	free(m);
}

/**
 * Test that file_find_migrations() skips files with empty
 * names.
 */
static void file_find_migrations_empty_filename(void)
{
	size_t size;

	reset_stubs();
	opendir_returns = (DIR *)1234;
	readdir_returns = &mig_empty_name;
	memcpy(config.migration_path, "/tmp", 5);

	CU_ASSERT_PTR_NULL(file_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_NOT_EQUAL(local_head[0], '\0');
	CU_ASSERT_STRING_EQUAL(local_head, "1");
	CU_ASSERT_TRUE(opendir_called && readdir_called);
	CU_ASSERT_TRUE(closedir_called);
}

/**
 * Test that file_find_migrations() skips files for which
 * stat() fails.
 */
static void file_find_migrations_stat_fails(void)
{
	size_t size;

	reset_stubs();
	opendir_returns = (DIR *)1234;
	readdir_returns = &mig_after_head;
	stat_returns = -1;
	stat_returns_buf.st_mode = S_IFREG;
	memcpy(config.migration_path, "/tmp", 5);

	CU_ASSERT_PTR_NULL(file_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_NOT_EQUAL(local_head[0], '\0');
	CU_ASSERT_STRING_EQUAL(local_head, "1");
	CU_ASSERT_TRUE(opendir_called && readdir_called);
	CU_ASSERT_TRUE(closedir_called);
}

/**
 * Test that file_find_migrations() skips files which
 * aren't regular files.
 */
static void file_find_migrations_not_regular_file(void)
{
	size_t size;

	reset_stubs();
	opendir_returns = (DIR *)1234;
	readdir_returns = &mig_after_head;
	stat_returns_buf.st_mode = S_IFDIR;
	memcpy(config.migration_path, "/tmp", 5);

	CU_ASSERT_PTR_NULL(file_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_NOT_EQUAL(local_head[0], '\0');
	CU_ASSERT_STRING_EQUAL(local_head, "1");
	CU_ASSERT_TRUE(opendir_called && readdir_called);
	CU_ASSERT_TRUE(closedir_called);
}

/**
 * Test that file_find_migrations() skips files with a
 * numeric designation that can't be represented as an
 * unsigned long.
 */
static void file_find_migrations_designation_out_of_range(void)
{
	size_t size;

	reset_stubs();
	opendir_returns = (DIR *)1234;
	readdir_returns = &mig_des_range;
	*errbuf = '\0';
	memcpy(config.migration_path, "/tmp", 5);

	CU_ASSERT_PTR_NULL(file_find_migrations("1", NULL, &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_NOT_EQUAL(local_head[0], '\0');
	CU_ASSERT_STRING_EQUAL(local_head, "1");
	CU_ASSERT_TRUE(opendir_called && readdir_called);
	CU_ASSERT_TRUE(closedir_called);
	CU_ASSERT_STRING_EQUAL(errbuf, err_des_range);
}

/**
 * Test that file_find_migrations() works without a
 * previous revision.
 */
static void file_find_migrations_no_prev_rev(void)
{
	char **m = NULL;
	size_t size = 0;

	reset_stubs();
	opendir_returns = (void *)12345;
	readdir_returns = &mig_name_too_small;
	stat_returns_buf.st_mode = S_IFREG;
	*errbuf = '\0';

	m = file_find_migrations("2", NULL, &size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_EQUAL(1, size);
	CU_ASSERT_STRING_EQUAL(local_head, "100");
	CU_ASSERT_STRING_EQUAL(m[0], mig_after_head.d_name);
	CU_ASSERT_STRING_EQUAL(errbuf, err_no_num);
	while (size) free(m[--size]);
	free(m);
}

/**
 * Test that file_find_migrations() works when the
 * previous revision is out of range.
 */
static void file_find_migrations_prev_rev_out_of_range(void)
{
	char **m = NULL;
	size_t size = 0;

	reset_stubs();
	opendir_returns = (void *)12345;
	readdir_returns = &mig_name_too_small;
	stat_returns_buf.st_mode = S_IFREG;
	*errbuf = '\0';

	m = file_find_migrations("2", "999999999999999999999999", &size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_EQUAL(1, size);
	CU_ASSERT_STRING_EQUAL(local_head, "100");
	CU_ASSERT_STRING_EQUAL(m[0], mig_after_head.d_name);
	CU_ASSERT_STRING_EQUAL(errbuf, err_no_num);
	while (size) free(m[--size]);
	free(m);
}

/**
 * Test that file_find_migrations() works.
 */
static void test_file_find_migrations(void)
{
	char **m = NULL;
	size_t size = 0;

	reset_stubs();
	opendir_returns = (void *)12345;
	readdir_returns = &mig_name_too_small;
	stat_returns_buf.st_mode = S_IFREG;
	*errbuf = '\0';

	m = file_find_migrations("100", "2", &size);
	CU_ASSERT_PTR_NOT_NULL_FATAL(m);
	CU_ASSERT_EQUAL(1, size);
	CU_ASSERT_STRING_EQUAL(local_head, "100");
	CU_ASSERT_STRING_EQUAL(m[0], mig_after_head.d_name);
	CU_ASSERT_STRING_EQUAL(errbuf, err_no_num);

	while (size) free(m[--size]);
	free(m);
}

static CU_TestInfo source_file_tests[] = {
	{
		"file_init() / file_uninit() work",
		test_file_init_uninit
	},
	{
		"file_get_head() - works",
		test_file_get_head
	},
	{
		"file_get_migration_path() - works",
		test_file_get_migration_path
	},
	{
		"file_find_migrations() - NULL size",
		file_find_migrations_null_size
	},
	{
		"file_find_migrations() - no migration path",
		file_find_migrations_no_migration_path
	},
	{
		"file_find_migrations() - handles errors when scanning",
		file_find_migrations_handles_scan_errors
	},
	{
		"file_find_migrations() - scans empty dir",
		file_find_migrations_empty_dir
	},
	{
		"file_find_migrations() - invalid current_head",
		file_find_migrations_invalid_current_head
	},
	{
		"file_find_migrations() - file with empty name",
		file_find_migrations_empty_filename
	},
	{
		"file_find_migrations() - stat() fails",
		file_find_migrations_stat_fails
	},
	{
		"file_find_migrations() - not regular file",
		file_find_migrations_not_regular_file
	},
	{
		"file_find_migrations() - designation out of range",
		file_find_migrations_designation_out_of_range
	},
	{
		"file_find_migrations() - no previous revision",
		file_find_migrations_no_prev_rev
	},
	{
		"file_find_migrations() - prevous revision out of range",
		file_find_migrations_prev_rev_out_of_range
	},
	{
		"file_find_migrations() - works",
		test_file_find_migrations
	},

	CU_TEST_INFO_NULL
};

void source_file_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Migration Source: File", NULL, NULL);
	while (source_file_tests[i].pName) {
		CU_add_test(suite, source_file_tests[i].pName,
		            source_file_tests[i].pTestFunc);
		i++;
	}
}

