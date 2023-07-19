/**
 * Minimal Migration Manager - Configuration Generator Tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include <check.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

#include "posix_stubs.h"
#include "../src/config_gen.h"
#include "../src/config_gen.c"

/**
 * Test that generate_config() returns the correct error
 * message if stat() on "." is unsuccessful.
 */
START_TEST(generate_config_cant_stat_cwd)
{
	const char *err = "unable to stat the current directory\n";

	*errbuf = '\0';
	stat_returns = -1;

	ck_assert_int_eq(generate_config("config", 1), 1);
	ck_assert_str_eq(errbuf, err);
	ck_assert(stat_called);
}
END_TEST

/**
 * Test that generate_config() returns 1 from gen_config_file()
 * if creating the config file fails.
 */
START_TEST(generate_config_unable_to_create_file)
{
	const char *err = "unable to create 'config'\n";

	*errbuf = '\0';
	stat_fails_at = 2;
	open_returns  = -1;

	ck_assert_int_eq(generate_config("config", 1), 1);
	ck_assert_str_eq(errbuf, err);
	ck_assert_int_eq(stat_called, 1);
	ck_assert(open_called);
	ck_assert(!close_called && !write_called && !unlink_called);
}
END_TEST

/**
 * Test that generate_config() returns 1 if the write()
 * call fails without having written anything.
 */
START_TEST(generate_config_first_write_fails)
{
	*errbuf = '\0';
	stat_fails_at = 2;
	open_returns  = 1;
	write_returns = -1;

	ck_assert_int_eq(generate_config("config", 1), 1);
	ck_assert_int_eq(close_called, 1);
	ck_assert_int_eq(write_called, 1);
	ck_assert(unlink_called);
	ck_assert(!*errbuf);
}
END_TEST

/**
 * Test that generate_config() returns 1 from gen_config_file()
 * if the first write succeeeds, and the second write fails.
 */
START_TEST(generate_config_second_write_fails)
{
	*errbuf = '\0';
	stat_fails_at  = 2;
	open_returns   = 1;
	write_returns  = 1;
	write_fails_at = 2;

	ck_assert_int_eq(generate_config("config", 1), 1);
	ck_assert_int_eq(close_called, 1);
	ck_assert_int_eq(write_called, 2);
	ck_assert(unlink_called);
	ck_assert(!*errbuf);
}
END_TEST

/**
 * Test that generate_config() returns 1 from gen_config_file()
 * if the first write fails due to EINTR, write() is called again,
 * and the subsquent write() fails.
 */
START_TEST(generate_config_write_interrupted)
{
	*errbuf = '\0';
	stat_fails_at  = 2;
	open_returns   = 1;
	write_returns  = (ssize_t)strlen(default_config_1);
	write_errno    = EINTR;
	write_fails_at = 2;

	ck_assert_int_eq(generate_config("config", 1), 1);
	ck_assert_int_eq(close_called, 1);
	ck_assert_int_eq(write_called, 2);
	ck_assert(unlink_called);
	ck_assert(!*errbuf);
}
END_TEST

/**
 * Test that generate_config() returns 1 if the migrations
 * path doesn't exist, and mkdir() fails.
 */
START_TEST(generate_config_creating_migrations_dir_fails)
{
	const char *err = "unable to create 'migrations' directory\n";

	*errbuf = '\0';
	mkdir_returns = -1;
	stat_fails_at = 2;
	stat_returns_buf.st_mode = S_IFREG;
	open_returns   = 1;
	write_returns  = (ssize_t)strlen(default_config_1);
	write_returns += (ssize_t)strlen(default_config_2);

	ck_assert_int_eq(generate_config("config", 0), 1);
	ck_assert_int_eq(stat_called, 1);
	ck_assert(open_called && write_called && close_called);
	ck_assert(!unlink_called);
	ck_assert(mkdir_called);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that generate_config() returns 0 if the migrations
 * path doesn't exist, and mkdir() succeeds.
 */
START_TEST(generate_config_creates_migrations_dir)
{
	*errbuf = '\0';
	stat_fails_at = 2;
	stat_returns_buf.st_mode = S_IFREG;
	open_returns   = 1;
	write_returns  = (ssize_t)strlen(default_config_1);
	write_returns += (ssize_t)strlen(default_config_2);

	ck_assert_int_eq(generate_config("config", 0), 0);
	ck_assert_int_eq(stat_called, 1);
	ck_assert(open_called && write_called && close_called);
	ck_assert(!unlink_called);
	ck_assert(mkdir_called);
	ck_assert(!*errbuf);
}
END_TEST

/**
 * Test that generate_config() returns 0 if gen_config_file()
 * wrote the whole config file.
 */
START_TEST(test_generate_config)
{
	*errbuf = '\0';
	stat_fails_at  = 2;
	open_returns   = 1;
	write_returns  = (ssize_t)strlen(default_config_1);
	write_returns  += (ssize_t)strlen(default_config_2);

	ck_assert_int_eq(generate_config("config", 1), 0);
	ck_assert_int_eq(close_called, 1);
	ck_assert_int_eq(write_called, 2);
	ck_assert(!unlink_called);
	ck_assert(!*errbuf);
}
END_TEST

Suite *config_gen_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("Config Generator");
	t = tcase_create("generate_config");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, generate_config_cant_stat_cwd);
	tcase_add_test(t, generate_config_unable_to_create_file);
	tcase_add_test(t, generate_config_first_write_fails);
	tcase_add_test(t, generate_config_second_write_fails);
	tcase_add_test(t, generate_config_write_interrupted);
	tcase_add_test(t, generate_config_creating_migrations_dir_fails);
	tcase_add_test(t, generate_config_creates_migrations_dir);
	tcase_add_test(t, test_generate_config);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

