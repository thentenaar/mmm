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

#include <check.h>
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
START_TEST(map_file_invalid_args)
{
	*errbuf = '\0';
	ck_assert_ptr_null(map_file(NULL, NULL));
	ck_assert(!fstat_called && !close_called && !*errbuf);

	*errbuf = '\0';
	ck_assert_ptr_null(map_file("test", NULL));
	ck_assert(!fstat_called && !close_called && !*errbuf);
}
END_TEST

/**
 * Test that map_file() returns NULL when it
 * cannot stat the file given by its path
 * argument; and that it yields the correct
 * error message.
 */
START_TEST(map_file_cant_stat)
{
	size_t size;
	char err[128];

	*errbuf = '\0';
	fstat_returns = -1;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = 100;
	open_returns = 1;

	/* without errno */
	ck_assert_ptr_null(map_file("test", &size));
	ck_assert_uint_eq(size, 0);
	ck_assert(open_called && fstat_called && close_called);
	ck_assert_str_eq(errbuf, "failed to map 'test'\n");

	/* with errno */
	sprintf(err, "failed to map 'test': %s\n",
	        strerror(EACCES));
	fstat_errno = EACCES;

	ck_assert_ptr_null(map_file("test", &size));
	ck_assert_uint_eq(size, 0);
	ck_assert(open_called && fstat_called && close_called);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test hat map_file() won't try to map a file with
 * a size of 0.
 */
START_TEST(map_file_empty_file)
{
	size_t size;

	*errbuf = '\0';
	fstat_returns = 0;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = 0;
	open_returns = 1;

	ck_assert_ptr_null(map_file("test", &size));
	ck_assert_uint_eq(size, 0);
	ck_assert(open_called && fstat_called && close_called);
	ck_assert_str_eq(errbuf, "failed to map 'test'\n");
}
END_TEST

/**
 * Test hat map_file() returns NULL and yields the proper
 * error message if open() fails.
 */
START_TEST(map_file_open_fails)
{
	size_t size;

	*errbuf = '\0';
	open_returns = -1;
	fstat_returns = 0;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = 100;

	ck_assert_ptr_null(map_file("test", &size));
	ck_assert_uint_eq(size, 0);
	ck_assert(open_called && !fstat_called && !close_called);
	ck_assert_str_eq(errbuf, "failed to map 'test'\n");
}
END_TEST

/**
 * Test that map_file() generates the proper error message
 * if asked to map a non-regular file.
 */
START_TEST(map_file_irregular_file)
{
	size_t size;
	char err[128];

	*errbuf = '\0';
	fstat_returns = 0;
	stat_returns = 0;
	stat_returns_buf.st_mode = S_IFDIR;
	stat_returns_buf.st_size = 100;
	open_returns = 1;

	sprintf(err, "failed to map 'test': %s\n", strerror(EACCES));
	ck_assert_ptr_null(map_file("test", &size));
	ck_assert_uint_eq(size, 0);
	ck_assert(open_called && fstat_called && close_called);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that the proper error message gets generated if read()
 * fails with errno set to something other than EINTR.
 */
START_TEST(map_file_read_fails)
{
	size_t size;
	char err[128];

	sprintf(err, "failed to map 'test': %s\n",
	        strerror(EBADF));

	*errbuf = '\0';
	open_returns  = 1;
	fstat_returns = 0;
	lseek_returns = 0;
	read_returns  = -1;
	read_errno    = EBADF;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = 1;

	ck_assert_ptr_null(map_file("test", &size));
	ck_assert_uint_eq(size, 0);
	ck_assert(open_called && fstat_called && read_called && close_called);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that the proper error message gets generated if read()
 * gets an EOF before the expected end of the file.
 */
START_TEST(map_file_read_premature_eof)
{
	size_t size;

	*errbuf = '\0';
	open_returns  = 1;
	fstat_returns = 0;
	lseek_returns = 0;
	read_returns  = 0;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = 1;

	ck_assert_ptr_null(map_file("test", &size));
	ck_assert_uint_eq(size, 0);
	ck_assert(open_called && fstat_called && read_called && close_called);
	ck_assert_str_eq(errbuf, "failed to map 'test'\n");
}
END_TEST

/**
 * Test that map_file() succeeds, using the built-in mmap().
 *
 * This test will cause read() to return -1 and set errno to
 * EINTR on the first call. The 2nd and 3rd calls will return
 * 1/2 of stat_returns_buf.st_size. This way, all the paths in
 * that loop get tested.
 */
START_TEST(test_map_file)
{
	size_t size;
	char *buf = NULL;

	*errbuf = '\0';
	open_returns  = 1;
	fstat_returns = 0;
	lseek_returns = 0;
	read_errno    = EINTR;
	read_returns  = sizeof(void *) >> 1;
	stat_returns_buf.st_mode = S_IFREG;
	stat_returns_buf.st_size = sizeof(void *);

	ck_assert_ptr_nonnull(buf = map_file("test", &size));
	ck_assert_uint_eq(size, (size_t)stat_returns_buf.st_size);
	ck_assert_int_eq(read_called, 3);
	ck_assert(open_called && fstat_called && close_called);
	ck_assert(!*errbuf);
	if (buf) unmap_file(buf, size);
}
END_TEST

Suite *file_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("File Mapping");
	t = tcase_create("map_file");
	tcase_add_checked_fixture(t, reset_stubs, NULL);
	tcase_add_test(t, map_file_invalid_args);
	tcase_add_test(t, map_file_cant_stat);
	tcase_add_test(t, map_file_empty_file);
	tcase_add_test(t, map_file_open_fails);
	tcase_add_test(t, map_file_irregular_file);
	tcase_add_test(t, map_file_read_fails);
	tcase_add_test(t, map_file_read_premature_eof);
	tcase_add_test(t, test_map_file);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

