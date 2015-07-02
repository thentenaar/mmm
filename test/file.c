/**
 * Minimal Migration Manager - Mapped File Tests
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
#include "../src/file.h"
#include "../src/file.c"

/**
 * Test that map_file() returns NULL when given
 * a NULL path argument.
 */
static void map_file_invalid_args(void)
{
	errbuf[0] = '\0';
	reset_stubs();
	file_mapped = 0;
	file_size = 0;

	CU_ASSERT_PTR_NULL(map_file(NULL, NULL));
	CU_ASSERT_FALSE(stat_called || close_called);
	CU_ASSERT_EQUAL_FATAL(errbuf[0], '\0');

	CU_ASSERT_PTR_NULL(map_file("test", NULL));
	CU_ASSERT_FALSE(stat_called || close_called);
	CU_ASSERT_EQUAL_FATAL(errbuf[0], '\0');
}

/**
 * Test that map_file() returns NULL and yields
 * the proper error message if a file is already
 * mapped.
 */
static void map_file_already_mapped(void)
{
	size_t size = 1;
	const char *err = "map_file: failed to map 'test': "
	                  "another file is already mapped\n";

	errbuf[0] = '\0';
	reset_stubs();
	file_mapped = 1;
	file_size = 0;

	CU_ASSERT_PTR_NULL(map_file("test", &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_FALSE(stat_called || close_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that map_file() returns NULL when it
 * cannot stat the file given by its path
 * argument; and that it yields the correct
 * error message.
 */
static void map_file_cant_stat_file(void)
{
	size_t size = 1;
	const char *err1 = "map_file: failed to map 'test'\n";
	char err2[128];

	errbuf[0] = '\0';
	reset_stubs();
	fstat_returns = -1;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = 100;
	open_returns = 1;
	file_mapped = 0;
	file_size = 0;

	/* Test it without errno */
	CU_ASSERT_PTR_NULL(map_file("test", &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_TRUE(open_called && fstat_called && close_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err1));
	CU_ASSERT_STRING_EQUAL(errbuf, err1);

	/* With errno */
	sprintf(err2, "map_file: failed to map 'test': %s\n",
	        strerror(EACCES));
	fstat_errno = EACCES;
	CU_ASSERT_PTR_NULL(map_file("test", &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_TRUE(open_called && fstat_called && close_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err2));
	CU_ASSERT_STRING_EQUAL(errbuf, err2);
}

/**
 * Test hat map_file() won't try to map a file with
 * a size of 0.
 */
static void map_file_wont_map_empty_file(void)
{
	size_t size = 1;
	const char *err = "map_file: failed to map 'test'\n";

	errbuf[0] = '\0';
	reset_stubs();
	fstat_returns = 0;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = 0;
	file_mapped = 0;
	file_size = 0;
	open_returns = 1;

	CU_ASSERT_PTR_NULL(map_file("test", &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_TRUE(open_called && fstat_called && close_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that map_file() generates the proper error message
 * if asked to mapp a non-regular file.
 */
static void map_file_wont_map_non_regular_file(void)
{
	size_t size = 1;
	const char *err = "map_file: 'test' is not a regular file\n";

	errbuf[0] = '\0';
	reset_stubs();
	fstat_returns = 0;
	stat_returns = 0;
	stat_returns_buf.st_mode = S_IFDIR;
	stat_returns_buf.st_size = 100;
	file_mapped = 0;
	file_size = 0;
	open_returns = 1;

	CU_ASSERT_PTR_NULL(map_file("test", &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_TRUE(open_called && fstat_called && close_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that map_file() generates the proper error message
 * if asked to mapp a file with an incorrect file size
 * (e.g. -1, SSIZE_MAX).
 */
static void map_file_wont_map_bad_file_size(void)
{
	size_t size = 1;
	char err[256];

	errbuf[0] = '\0';
	reset_stubs();
	fstat_returns = 0;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = -1;
	file_mapped = 0;
	file_size = 0;
	open_returns = 1;

	/* < 0 */
	sprintf(err, "map_file: bad size for 'test' (%ld bytes)\n",
	        stat_returns_buf.st_size);
	CU_ASSERT_PTR_NULL(map_file("test", &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_TRUE(open_called && fstat_called && close_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);

	/* == SSIZE_MAX */
	stat_returns_buf.st_size = SSIZE_MAX;
	sprintf(err, "map_file: bad size for 'test' (%ld bytes)\n",
	        stat_returns_buf.st_size);
	CU_ASSERT_PTR_NULL(map_file("test", &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_TRUE(open_called && fstat_called && close_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test hat map_file() returns NULL and yields the proper
 * error message if open() fails.
 */
static void map_file_error_if_open_fails(void)
{
	size_t size = 1;
	const char *err = "map_file: failed to map 'test'\n";

	errbuf[0] = '\0';
	reset_stubs();
	open_returns = -1;
	fstat_returns = 0;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = 100;
	file_mapped = 0;
	file_size = 0;

	CU_ASSERT_PTR_NULL(map_file("test", &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_TRUE(open_called);
	CU_ASSERT_FALSE(fstat_called && close_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that map_file() returns NULL if lseek() fails
 * during read_file().
 */
static void read_file_lseek_fails(void)
{
	size_t size = 1;
	const char *err = "map_file: failed to map 'test'\n";

	errbuf[0] = '\0';
	reset_stubs();
	open_returns = 1;
	fstat_returns = 0;
	lseek_returns = -1;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = sizeof(void *);
	file_mapped = 0;
	file_size = 0;

	CU_ASSERT_PTR_NULL(map_file("test", &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_EQUAL(file_mapped, 0);
	CU_ASSERT_TRUE(fstat_called && open_called);
	CU_ASSERT_FALSE(read_called);
	CU_ASSERT_TRUE(lseek_called && close_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that the proper error message gets generated if read()
 * fails with errno set to something other than EINTR.
 */
static void read_file_read_fails(void)
{
	size_t size = 1;
	char err[256];

	sprintf(err, "map_file: failed to map 'test': %s\n",
	        strerror(EBADF));

	errbuf[0] = '\0';
	reset_stubs();
	open_returns = 1;
	fstat_returns = 0;
	lseek_returns = 0;
	read_returns = -1;
	read_errno = EBADF;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = sizeof(void *);
	file_mapped = 0;
	file_size = 0;

	CU_ASSERT_PTR_NULL(map_file("test", &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_EQUAL(file_mapped, 0)
	CU_ASSERT_TRUE(fstat_called && open_called);
	CU_ASSERT_TRUE(read_called);
	CU_ASSERT_TRUE(lseek_called && close_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that the proper error message gets generated if read()
 * gets an EOF before the expected end of the file.
 */
static void read_file_premature_eof(void)
{
	size_t size = 1;
	char err[256];

	sprintf(err, "map_file: failed to map 'test'\n");

	errbuf[0] = '\0';
	reset_stubs();
	open_returns = 1;
	fstat_returns = 0;
	lseek_returns = 0;
	read_returns = 0;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = sizeof(void *);
	file_mapped = 0;
	file_size = 0;

	CU_ASSERT_PTR_NULL(map_file("test", &size));
	CU_ASSERT_EQUAL(size, 0);
	CU_ASSERT_EQUAL(file_mapped, 0);
	CU_ASSERT_TRUE(fstat_called && open_called);
	CU_ASSERT_TRUE(read_called);
	CU_ASSERT_TRUE(lseek_called && close_called);
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that read_file() succeeds.
 *
 * This test will cause read() to return -1 and set errno to
 * EINTR on the first call. The 2nd and 3rd calls will return
 * 1/2 of stat_returns_buf.st_size. This way, all the paths in
 * that loop get tested.
 */
static void test_read_file(void)
{
	size_t size = 1;

	errbuf[0] = '\0';
	reset_stubs();
	open_returns = 1;
	fstat_returns = 0;
	lseek_returns = 0;
	read_errno   = EINTR;
	read_returns = sizeof(void *) >> 1;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = sizeof(void *);
	file_mapped = 0;
	file_size = 0;

	CU_ASSERT_EQUAL(map_file("test", &size), filebuf);
	CU_ASSERT_EQUAL(file_mapped, 1);
	CU_ASSERT_EQUAL(file_size, size);
	CU_ASSERT_TRUE(fstat_called && open_called);
	CU_ASSERT_EQUAL(read_called, 3);
	CU_ASSERT_TRUE(lseek_called && close_called);
	CU_ASSERT_EQUAL(errbuf[0], '\0');
}

/**
 * Test that unmap_file() works.
 */
static void test_unmap_file(void)
{
	file_mapped = 0;
	unmap_file();

	file_mapped = 1;
	file_size = 10;
	*filebuf = 'x';
	unmap_file();
	CU_ASSERT_NOT_EQUAL(filebuf[0], 'x');
	CU_ASSERT_EQUAL(file_mapped, 0);
	CU_ASSERT_EQUAL(file_size, 0);
}

static CU_TestInfo file_tests[] = {
	{
		"map_file - invalid args",
		map_file_invalid_args
	},
	{
		"map_file - file already mapped",
		map_file_already_mapped
	},
	{
		"map_file - can't stat file",
		map_file_cant_stat_file
	},
	{
		"map_file - won't map empty file",
		map_file_wont_map_empty_file
	},
	{
		"map_file - won't map non-regular file",
		map_file_wont_map_non_regular_file
	},
	{
		"map_file - won't mmap badly sized file",
		map_file_wont_map_bad_file_size
	},
	{
		"map_file - yields error if open() fails",
		map_file_error_if_open_fails
	},
	{
		"read_file - lseek() fails",
		read_file_lseek_fails
	},
	{
		"read_file - read() fails",
		read_file_read_fails
	},
	{
		"read_file - premature EOF",
		read_file_premature_eof
	},
	{
		"read_file - succeeds",
		test_read_file
	},
	{
		"unmap_file - works",
		test_unmap_file
	},

	CU_TEST_INFO_NULL
};

void file_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("File Handling", NULL, NULL);
	while (file_tests[i].pName) {
		CU_add_test(suite, file_tests[i].pName,
		            file_tests[i].pTestFunc);
		i++;
	}
}

