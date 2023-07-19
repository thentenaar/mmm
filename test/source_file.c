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

#include <check.h>
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
START_TEST(test_file_init_uninit)
{
	file_config();
	ck_assert_int_eq(file_init(), 0);
	ck_assert_int_eq(file_uninit(), 0);
}
END_TEST

/**
 * Test that file_get_head() works.
 */
START_TEST(test_file_get_head)
{
	*local_head = '\0';
	ck_assert_ptr_nonnull(file_get_head());
	ck_assert(*file_get_head() == '0');
	memcpy(local_head, "1", 2);
	ck_assert_ptr_nonnull(file_get_head());
}
END_TEST

/**
 * Test that file_get_migration_path() works.
 */
START_TEST(test_file_get_migration_path)
{
	*local_head = '\0';
	ck_assert_ptr_eq(file_get_migration_path(), config.migration_path);
}
END_TEST

/**
 * Test that file_find_migrations() returns NULL if
 * size is NULL.
 */
START_TEST(file_find_migrations_null_size)
{
	ck_assert_ptr_null(file_find_migrations("1", "2", NULL));
}
END_TEST

/**
 * Test that file_find_migrations() returns NULL if no
 * migration_path was set.
 */
START_TEST(file_find_migrations_no_migration_path)
{
	size_t size = 9999;

	*config.migration_path = '\0';
	ck_assert_ptr_null(file_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
}
END_TEST

/**
 * Test that file_find_migrations() returns NULL if an
 * error occurs while scanning the migration_path.
 */
START_TEST(file_find_migrations_handles_scan_errors)
{
	size_t size;

	opendir_returns = NULL;
	*local_head = '\0';
	memcpy(config.migration_path, "/tmp", 5);

	ck_assert_ptr_null(file_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
	ck_assert(!*local_head);
	ck_assert(opendir_called && !closedir_called);
}
END_TEST

/**
 * Test that file_find_migrations() returns NULL if the
 * migration directory is empty.
 */
START_TEST(file_find_migrations_empty_dir)
{
	size_t size;

	opendir_returns = (DIR *)1234;
	readdir_returns = NULL;
	*local_head = '\0';
	memcpy(config.migration_path, "/tmp", 5);

	ck_assert_ptr_null(file_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
	ck_assert_str_eq(local_head, "1");
	ck_assert(*local_head);
	ck_assert(opendir_called && readdir_called && closedir_called);
}
END_TEST

/**
 * Test that file_find_migrations() doesn't skip files when
 * current_head isn't valid.
 */
START_TEST(file_find_migrations_invalid_current_head)
{
	char **m = NULL;
	size_t size;

	opendir_returns = (DIR *)1234;
	readdir_returns = &mig_after_head;
	stat_returns_buf.st_mode = S_IFREG;
	memcpy(config.migration_path, "/tmp", 5);

	ck_assert_ptr_nonnull(m = file_find_migrations("xxx", NULL, &size));
	ck_assert_uint_eq(size, 1);
	ck_assert_str_eq(local_head, "100");
	ck_assert(opendir_called && readdir_called && closedir_called);
	if (m) free(m[0]);
	free(m);
}
END_TEST

/**
 * Test that file_find_migrations() skips files with empty
 * names.
 */
START_TEST(file_find_migrations_empty_filename)
{
	size_t size;

	opendir_returns = (DIR *)1234;
	readdir_returns = &mig_empty_name;
	memcpy(config.migration_path, "/tmp", 5);

	ck_assert_ptr_null(file_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
	ck_assert_str_eq(local_head, "1");
	ck_assert(opendir_called && readdir_called && closedir_called);
}
END_TEST

/**
 * Test that file_find_migrations() skips files for which
 * stat() fails.
 */
START_TEST(file_find_migrations_stat_fails)
{
	size_t size;

	opendir_returns = (DIR *)1234;
	readdir_returns = &mig_after_head;
	stat_returns = -1;
	stat_returns_buf.st_mode = S_IFREG;
	memcpy(config.migration_path, "/tmp", 5);

	ck_assert_ptr_null(file_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
	ck_assert_str_eq(local_head, "1");
	ck_assert(opendir_called && readdir_called && closedir_called);
}
END_TEST

/**
 * Test that file_find_migrations() skips files which
 * aren't regular files.
 */
START_TEST(file_find_migrations_not_regular_file)
{
	size_t size;

	opendir_returns = (DIR *)1234;
	readdir_returns = &mig_after_head;
	stat_returns_buf.st_mode = S_IFDIR;
	memcpy(config.migration_path, "/tmp", 5);

	ck_assert_ptr_null(file_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
	ck_assert_str_eq(local_head, "1");
	ck_assert(opendir_called && readdir_called && closedir_called);
}
END_TEST

/**
 * Test that file_find_migrations() skips files with a
 * numeric designation that can't be represented as an
 * unsigned long.
 */
START_TEST(file_find_migrations_designation_out_of_range)
{
	size_t size;

	opendir_returns = (DIR *)1234;
	readdir_returns = &mig_des_range;
	*errbuf = '\0';
	memcpy(config.migration_path, "/tmp", 5);

	ck_assert_ptr_null(file_find_migrations("1", NULL, &size));
	ck_assert_uint_eq(size, 0);
	ck_assert_str_eq(local_head, "1");
	ck_assert(opendir_called && readdir_called && closedir_called);
	ck_assert_str_eq(errbuf, err_des_range);
}
END_TEST

/**
 * Test that file_find_migrations() works without a
 * previous revision.
 */
START_TEST(file_find_migrations_no_prev_rev)
{
	char **m = NULL;
	size_t size;

	opendir_returns = (void *)12345;
	readdir_returns = &mig_name_too_small;
	stat_returns_buf.st_mode = S_IFREG;
	*errbuf = '\0';
	memcpy(config.migration_path, "/tmp", 5);

	ck_assert_ptr_nonnull(m = file_find_migrations("2", NULL, &size));
	ck_assert_uint_eq(size, 1);
	ck_assert_str_eq(local_head, "100");
	ck_assert_str_eq(errbuf, err_no_num);
	if (m) ck_assert_str_eq(m[0], mig_after_head.d_name);
	while (m && size) free(m[--size]);
	free(m);
}
END_TEST

/**
 * Test that file_find_migrations() works when the
 * previous revision is out of range.
 */
START_TEST(file_find_migrations_prev_rev_out_of_range)
{
	char **m = NULL;
	size_t size = 0;

	reset_stubs();
	opendir_returns = (void *)12345;
	readdir_returns = &mig_name_too_small;
	stat_returns_buf.st_mode = S_IFREG;
	*errbuf = '\0';
	memcpy(config.migration_path, "/tmp", 5);

	ck_assert_ptr_nonnull(m = file_find_migrations("2", "999999999999999999999999", &size));
	ck_assert_uint_eq(size, 1);
	ck_assert_str_eq(local_head, "100");
	ck_assert_str_eq(errbuf, err_no_num);
	if (m) ck_assert_str_eq(m[0], mig_after_head.d_name);
	while (m && size) free(m[--size]);
	free(m);
}
END_TEST

/**
 * Test that file_find_migrations() works.
 */
START_TEST(test_file_find_migrations)
{
	char **m = NULL;
	size_t size = 0;

	opendir_returns = (void *)12345;
	readdir_returns = &mig_name_too_small;
	stat_returns_buf.st_mode = S_IFREG;
	*errbuf = '\0';
	memcpy(config.migration_path, "/tmp", 5);

	ck_assert_ptr_nonnull(m = file_find_migrations("100", "2", &size));
	ck_assert_uint_eq(size, 1);
	ck_assert_str_eq(local_head, "100");
	ck_assert_str_eq(errbuf, err_no_num);
	if (m) ck_assert_str_eq(m[0], mig_after_head.d_name);
	while (m && size) free(m[--size]);
	free(m);
}
END_TEST

Suite *source_file_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("Migration Source: File");
	t = tcase_create("file_init");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, test_file_init_uninit);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("file_get_head");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, test_file_get_head);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("file_get_migration_path");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, test_file_get_migration_path);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("file_find_migrations");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, file_find_migrations_null_size);
	tcase_add_test(t, file_find_migrations_no_migration_path);
	tcase_add_test(t, file_find_migrations_handles_scan_errors);
	tcase_add_test(t, file_find_migrations_empty_dir);
	tcase_add_test(t, file_find_migrations_invalid_current_head);
	tcase_add_test(t, file_find_migrations_empty_filename);
	tcase_add_test(t, file_find_migrations_stat_fails);
	tcase_add_test(t, file_find_migrations_not_regular_file);
	tcase_add_test(t, file_find_migrations_designation_out_of_range);
	tcase_add_test(t, file_find_migrations_no_prev_rev);
	tcase_add_test(t, file_find_migrations_prev_rev_out_of_range);
	tcase_add_test(t, test_file_find_migrations);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

