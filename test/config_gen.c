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
#include <CUnit/CUnit.h>
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
static void generate_config_cant_stat_cwd(void)
{
	const char *err = "generate_config: unable to stat the current "
	                  "directory\n";

	errbuf[0] = '\0';
	reset_stubs();
	stat_returns = -1;
	CU_ASSERT_EQUAL(1, generate_config("config", 1));
	CU_ASSERT_TRUE(stat_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that generate_config() returns 1 from gen_config_file()
 * if creating the config file fails.
 */
static void gen_config_file_unable_to_create(void)
{
	const char *err = "gen_config_file: unable to create 'config'\n";

	errbuf[0] = '\0';
	reset_stubs();
	stat_fails_at = 2;
	open_returns = -1;
	CU_ASSERT_EQUAL(1, generate_config("config", 1));
	CU_ASSERT_EQUAL(stat_called, 1);
	CU_ASSERT_TRUE(open_called);
	CU_ASSERT_FALSE(close_called || write_called || unlink_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that generate_config() returns 1 if the write()
 * call fails without having written anything.
 */
static void gen_config_file_first_write_fails(void)
{
	errbuf[0] = '\0';
	reset_stubs();
	stat_fails_at = 2;
	open_returns = 1;
	write_returns = -1;
	CU_ASSERT_EQUAL(1, generate_config("config", 1));
	CU_ASSERT_EQUAL(close_called, 1);
	CU_ASSERT_EQUAL(write_called, 1);
	CU_ASSERT_TRUE(unlink_called);
	CU_ASSERT_EQUAL(errbuf[0], '\0');
}

/**
 * Test that generate_config() returns 1 from gen_config_file()
 * if the first write succeeeds, and the second write fails.
 */
static void gen_config_file_second_write_fails(void)
{
	errbuf[0] = '\0';
	reset_stubs();
	stat_fails_at  = 2;
	open_returns  = 1;
	write_returns  = 1;
	write_fails_at = 2;
	CU_ASSERT_EQUAL(1, generate_config("config", 1));
	CU_ASSERT_EQUAL(write_called, 2);
	CU_ASSERT_EQUAL(close_called, 1);
	CU_ASSERT_TRUE(unlink_called);
	CU_ASSERT_EQUAL(errbuf[0], '\0');
}

/**
 * Test that generate_config() returns 1 from gen_config_file()
 * if the first write fails due to EINTR, write() is called again,
 * and the subsquent write() fails.
 */
static void gen_config_file_write_einval(void)
{
	errbuf[0] = '\0';
	reset_stubs();
	stat_fails_at  = 2;
	open_returns   = 1;
	write_returns  = (ssize_t)strlen(default_config_1);
	write_errno    = EINTR;
	write_fails_at = 2;

	CU_ASSERT_EQUAL(1, generate_config("config", 1));
	CU_ASSERT_EQUAL(write_called, 2);
	CU_ASSERT_EQUAL(close_called, 1);
	CU_ASSERT_TRUE(unlink_called);
	CU_ASSERT_EQUAL(errbuf[0], '\0');
}

/**
 * Test that generate_config() returns 0 if gen_config_file()
 * wrote the whole config file.
 */
static void test_gen_config_file(void)
{
	errbuf[0] = '\0';
	reset_stubs();
	stat_fails_at  = 2;
	open_returns   = 1;
	write_returns  = (ssize_t)strlen(default_config_1);
	write_returns  += (ssize_t)strlen(default_config_2);

	CU_ASSERT_EQUAL(0, generate_config("config", 1));
	CU_ASSERT_EQUAL(write_called, 2);
	CU_ASSERT_EQUAL(close_called, 1);
	CU_ASSERT_FALSE(unlink_called);
	CU_ASSERT_EQUAL(errbuf[0], '\0');
}

/**
 * Test that generate_config() returns 1 if the migrations
 * path doesn't exist, and mkdir() fails.
 */
static void generate_config_creating_migrations_dir_fails(void)
{
	const char *err = "generate_config: unable to create 'migrations' "
	                  "directory\n";

	errbuf[0] = '\0';
	reset_stubs();
	mkdir_returns = -1;
	stat_fails_at = 2;
	stat_returns_buf.st_mode = S_IFREG;
	open_returns = 1;
	write_returns  = (ssize_t)strlen(default_config_1);
	write_returns  += (ssize_t)strlen(default_config_2);

	CU_ASSERT_EQUAL(1, generate_config("config", 0));
	CU_ASSERT_EQUAL(stat_called, 1);
	CU_ASSERT_TRUE(open_called && write_called && close_called);
	CU_ASSERT_FALSE(unlink_called);
	CU_ASSERT_TRUE(mkdir_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that generate_config() returns 0 if the migrations
 * path doesn't exist, and mkdir() succeeds.
 */
static void generate_config_creates_migrations_dir(void)
{
	errbuf[0] = '\0';
	reset_stubs();
	stat_fails_at = 2;
	stat_returns_buf.st_mode = S_IFREG;
	open_returns = 1;
	write_returns  = (ssize_t)strlen(default_config_1);
	write_returns  += (ssize_t)strlen(default_config_2);

	CU_ASSERT_EQUAL(0, generate_config("config", 0));
	CU_ASSERT_EQUAL(stat_called, 1);
	CU_ASSERT_TRUE(open_called && write_called && close_called);
	CU_ASSERT_FALSE(unlink_called);
	CU_ASSERT_TRUE(mkdir_called);
	CU_ASSERT_EQUAL_FATAL(errbuf[0], '\0');
}

static CU_TestInfo config_gen_tests[] = {
	{
		"generate_config - can't stat() cwd",
		generate_config_cant_stat_cwd
	},
	{
		"gen_config_file - unable to create file",
		gen_config_file_unable_to_create
	},
	{
		"gen_config_file - first write fails",
		gen_config_file_first_write_fails
	},
	{
		"gen_config_file - second write fails",
		gen_config_file_second_write_fails
	},
	{
		"gen_config_file - handles EINTR from write",
		gen_config_file_write_einval
	},
	{
		"gen_config_file - writes whole file",
		test_gen_config_file
	},
	{
		"generate_config - creating migrations dir fails",
		generate_config_creating_migrations_dir_fails
	},
	{
		"generate_config - creates migrations dir",
		generate_config_creates_migrations_dir
	},

	CU_TEST_INFO_NULL
};

void config_gen_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Config Generator", NULL, NULL);
	while (config_gen_tests[i].pName) {
		CU_add_test(suite, config_gen_tests[i].pName,
		            config_gen_tests[i].pTestFunc);
		i++;
	}
}

