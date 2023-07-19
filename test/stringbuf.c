/**
 * Minimal Migration Manager - String Buffer Tests
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

#include "../src/stringbuf.h"
#include "../src/stringbuf.c"

/**
 * Test that sbuf_reset() should set offset to 0, and only
 * overwrite the first byte of the buffer with '\0'.
 */
START_TEST(sbuf_reset_no_scrub)
{
	sbuf[0] = 'x';
	sbuf[1] = 'x';
	offset = 1;

	sbuf_reset(0);
	ck_assert_uint_eq(offset, 0);
	ck_assert(!*sbuf && sbuf[1] == 'x');
}
END_TEST

/**
 * Test that sbuf_reset() should scrub the buffer only up until
 * offset.
 */
START_TEST(sbuf_reset_scrub_till_offset)
{
	sbuf[0] = 'x';
	sbuf[1] = 'x';
	sbuf[2] = 'x';
	offset = 2;

	sbuf_reset(1);
	ck_assert_uint_eq(offset, 0);
	ck_assert(!*sbuf && !sbuf[1] && sbuf[2] == 'x');
}
END_TEST

/**
 * Test that sbuf_reset() should scrub the whole buffer.
 */
START_TEST(sbuf_reset_scrub_all)
{
	offset = 0;
	memset(sbuf, 'x', SBUFSIZ);
	sbuf_reset(1);
	ck_assert(!sbuf[SBUFSIZ - 1] && !sbuf[SBUFSIZ >> 1] && !*sbuf);
}
END_TEST

/**
 * Test that sbuf_get_buffer() returns a pointer to sbuf.
 */
START_TEST(test_sbuf_get_buffer)
{
	ck_assert_ptr_eq(sbuf_get_buffer(), sbuf);
}
END_TEST

/**
 * Test that sbuf_add_str() returns 1 if a NULL
 * argument for s is passed.
 */
START_TEST(sbuf_add_str_null_string)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str(NULL, 0, 0), 1);
	ck_assert_uint_eq(offset, 0);
}
END_TEST

/**
 * Test that sbuf_add_str() returns 1 if the string
 * in s is too long to fit in the buffer.
 */
START_TEST(sbuf_add_str_too_long)
{
	sbuf_reset(0);

	offset = SBUFSIZ - 7;
	ck_assert_int_eq(sbuf_add_str(".", 0, 0), 1);
	ck_assert_uint_eq(offset, SBUFSIZ - 7);

	offset = SBUFSIZ - 6;
	ck_assert_int_eq(sbuf_add_str(".", 0, 0), 1);
	ck_assert_uint_eq(offset, SBUFSIZ - 6);
}
END_TEST

/**
 * If no formatting and no string are specified,
 * sbuf_add_str() should be a no-op.
 */
START_TEST(sbuf_add_str_no_string_or_formatting)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str("", 0, 0), 0);
	ck_assert_uint_eq(offset, 0);
}
END_TEST

/**
 * Test that sbuf_add_str() adds the specified string.
 */
START_TEST(sbuf_add_str_string_no_formatting)
{
	sbuf_reset(0);

	ck_assert_int_eq(sbuf_add_str("xxx", 0, 0), 0);
	ck_assert_uint_eq(offset, 3);
	ck_assert_str_eq(sbuf, "xxx");
}
END_TEST

/**
 * Test that sbuf_add_str() adds lparen if the requisite format was
 * specified.
 */
START_TEST(sbuf_add_str_lparen)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str("", SBUF_LPAREN, 0), 0);
	ck_assert_uint_eq(offset, 1);
	ck_assert_str_eq(sbuf, "(");
}
END_TEST

/**
 * Test that sbuf_add_str() adds a leading space if the requisite
 * format was specified.
 */
START_TEST(sbuf_add_str_lspace)
{
	sbuf_reset(0);
	sbuf[offset++] = 'x';

	ck_assert_int_eq(sbuf_add_str("", SBUF_LSPACE, 0), 0);
	ck_assert_uint_eq(offset, 2);
	ck_assert_str_eq(sbuf, "x ");
}
END_TEST

/**
 * Test that sbuf_add_str() adds a trailing space if the requisite
 * format was specified.
 */
START_TEST(sbuf_add_str_tspace)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str("", SBUF_TSPACE, 0), 0);
	ck_assert_uint_eq(offset, 1);
	ck_assert_str_eq(sbuf, " ");
}
END_TEST

/**
 * Test that sbuf_add_str() adds quotes if the requisite format
 * was specified.
 */
START_TEST(sbuf_add_str_quotes)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str("", SBUF_QUOTE, 0), 0);
	ck_assert_uint_eq(offset, 2);
	ck_assert_str_eq(sbuf, "''");
}
END_TEST

/**
 * Test that sbuf_add_str() adds a comma if the requisite format
 * was specified.
 */
START_TEST(sbuf_add_str_comma)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str("", SBUF_COMMA, 0), 0);
	ck_assert_uint_eq(offset, 1);
	ck_assert_str_eq(sbuf, ",");
}
END_TEST

/**
 * Test that sbuf_add_str() adds equals if the requisite format
 * was specified.
 */
START_TEST(sbuf_add_str_equals)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str("", SBUF_EQUALS, 0), 0);
	ck_assert_uint_eq(offset, 1);
	ck_assert_str_eq(sbuf, "=");
}
END_TEST

/**
 * Test that sbuf_add_str() adds rparen if the reqisite format was
 * specified.
 */
START_TEST(sbuf_add_str_rparen)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str("", SBUF_RPAREN, 0), 0);
	ck_assert_uint_eq(offset, 1);
	ck_assert_str_eq(sbuf, ")");
}
END_TEST

/**
 * Test that sbuf_add_str() adds a semicolon if the requisite format
 * was specified.
 */
START_TEST(sbuf_add_str_scolon)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str("", SBUF_SCOLON, 0), 0);
	ck_assert_uint_eq(offset, 1);
	ck_assert_str_eq(sbuf, ";");
}
END_TEST

/**
 * Test that sbuf_add_str() adds a lparen and comma with the
 * specified string in-between.
 */
START_TEST(sbuf_add_str_lparen_comma)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str("x", SBUF_LPAREN | SBUF_COMMA, 0), 0);
	ck_assert_uint_eq(offset, 3);
	ck_assert_str_eq(sbuf, "(x,");
}
END_TEST

/**
 * Test that sbuf_add_str() adds quotes around the specified
 * string, and a comma afterwards.
 */
START_TEST(sbuf_add_str_quote_comma)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str("x", SBUF_QUOTE | SBUF_COMMA, 0), 0);
	ck_assert_uint_eq(offset, 4);
	ck_assert_str_eq(sbuf, "'x',");
}
END_TEST

/**
 * Test that sbuf_add_str() adds quotes around the specified
 * string, a rparen and then a semicolon afterwards.
 */
START_TEST(sbuf_add_str_quote_rparen_scolon)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_str("x", SBUF_QUOTE | SBUF_RPAREN | SBUF_SCOLON, 0), 0);
	ck_assert_uint_eq(offset, 5);
	ck_assert_str_eq(sbuf, "'x');");
}
END_TEST

/**
 * Test that sbuf_add_unum() returns 1 if there's
 * insufficient space in the buffer for the number.
 */
START_TEST(sbuf_add_unum_too_long)
{
	sbuf_reset(0);
	offset = SBUFSIZ - 5;
	ck_assert_int_eq(sbuf_add_unum(ULONG_MAX, 0), 1);
}
END_TEST

/**
 * Test that sbuf_add_unum() successfully adds numbers.
 */
START_TEST(test_sbuf_add_unum)
{
	char nbuf[50];

	sbuf_reset(0);
	sprintf(nbuf, "%lu", ULONG_MAX);
	ck_assert_int_eq(sbuf_add_unum(ULONG_MAX, 0), 0);
	ck_assert_uint_eq(offset, strlen(nbuf));
	ck_assert_str_eq(sbuf, nbuf);

	sbuf_reset(0);
	sprintf(nbuf, "%lu", ULONG_MAX >> 1);
	ck_assert_int_eq(sbuf_add_unum(ULONG_MAX >> 1, 0), 0);
	ck_assert_uint_eq(offset, strlen(nbuf));
	ck_assert_str_eq(sbuf, nbuf);

	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_unum(1, 0), 0);
	ck_assert_uint_eq(offset, 1);
	ck_assert_str_eq(sbuf, "1");

	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_unum(0, 0), 0);
	ck_assert_uint_eq(offset, 1);
	ck_assert_str_eq(sbuf, "0");
}
END_TEST

/**
 * Test that sbuf_add_snum() returns 1 if there's
 * insufficient space in the buffer for the number.
 */
START_TEST(sbuf_add_snum_too_long)
{
	sbuf_reset(0);
	offset = SBUFSIZ - 3;
	ck_assert_int_eq(sbuf_add_snum(LONG_MAX, 0), 1);

	sbuf_reset(0);
	offset = SBUFSIZ - 3;
	ck_assert_int_eq(sbuf_add_snum(-LONG_MAX, 0), 1);
}
END_TEST

/**
 * Test that sbuf_add_snum() successfully adds numbers.
 */
START_TEST(test_sbuf_add_snum)
{
	char nbuf[50];

	sbuf_reset(0);
	sprintf(nbuf, "%ld", LONG_MAX);
	ck_assert_int_eq(sbuf_add_snum(LONG_MAX, 0), 0);
	ck_assert_uint_eq(offset, strlen(nbuf));
	ck_assert_str_eq(sbuf, nbuf);

	sbuf_reset(0);
	sprintf(nbuf, "%ld", -LONG_MAX);
	ck_assert_int_eq(sbuf_add_snum(-LONG_MAX, 0), 0);
	ck_assert_uint_eq(offset, strlen(nbuf));
	ck_assert_str_eq(sbuf, nbuf);

	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_snum(-1, 0), 0);
	ck_assert_uint_eq(offset, 2);
	ck_assert_str_eq(sbuf, "-1");

	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_snum(0, 0), 0);
	ck_assert_uint_eq(offset, 1);
	ck_assert_str_eq(sbuf, "0");
}
END_TEST

/**
 * Test that sbuf_add_param_str() returns 1 if either of its
 * parameters is NULL.
 */
START_TEST(sbuf_add_param_str_null_params)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_param_str(NULL, "test"), 1);
	ck_assert_uint_eq(offset, 0);

	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_param_str("test", NULL), 1);
	ck_assert_uint_eq(offset, 0);

	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_param_str(NULL, NULL), 1);
	ck_assert_uint_eq(offset, 0);
}
END_TEST

/**
 * Test that sbuf_add_param_str() with a empty value
 * is basically a no-op.
 */
START_TEST(sbuf_add_param_str_empty_value)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_param_str("test", ""), 1);
	ck_assert_uint_eq(offset, 0);
}
END_TEST

/**
 * Test that sbuf_add_param_str() works as expected.
 */
START_TEST(test_sbuf_add_param_str)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_param_str("test", "value"), 0);
	ck_assert_uint_eq(offset, 12);
	ck_assert_str_eq(sbuf, "test='value'");
}
END_TEST

/**
 * Test that sbuf_add_param_num() returns 1 if param is NULL.
 */
START_TEST(sbuf_add_param_num_null_params)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_param_num(NULL, 1), 1);
	ck_assert_uint_eq(offset, 0);
}
END_TEST

/**
 * Test that sbuf_add_param_num() works.
 */
START_TEST(test_sbuf_add_param_num)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_param_num("test", 42), 0);
	ck_assert_uint_eq(offset, 7);
	ck_assert_str_eq(sbuf, "test=42");
}
END_TEST

/**
 * Test that sbuf_add_param_snum() returns 1 if param is NULL.
 */
START_TEST(sbuf_add_param_snum_null_params)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_param_snum(NULL, -1), 1);
	ck_assert_uint_eq(offset, 0);
}
END_TEST

/**
 * Test that sbuf_add_param_snum() works.
 */
START_TEST(test_sbuf_add_param_snum)
{
	sbuf_reset(0);
	ck_assert_int_eq(sbuf_add_param_snum("test", -42), 0);
	ck_assert_uint_eq(offset, 8);
	ck_assert_str_eq(sbuf, "test=-42");
}
END_TEST

Suite *stringbuf_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("String Buffer");
	t = tcase_create("sbuf_reset");
	tcase_add_test(t, sbuf_reset_no_scrub);
	tcase_add_test(t, sbuf_reset_scrub_till_offset);
	tcase_add_test(t, sbuf_reset_scrub_all);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("sbuf_get_buffer");
	tcase_add_test(t, test_sbuf_get_buffer);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("sbuf_add_str");
	tcase_add_test(t, sbuf_add_str_null_string);
	tcase_add_test(t, sbuf_add_str_too_long);
	tcase_add_test(t, sbuf_add_str_no_string_or_formatting);
	tcase_add_test(t, sbuf_add_str_string_no_formatting);
	tcase_add_test(t, sbuf_add_str_lparen);
	tcase_add_test(t, sbuf_add_str_lspace);
	tcase_add_test(t, sbuf_add_str_quotes);
	tcase_add_test(t, sbuf_add_str_comma);
	tcase_add_test(t, sbuf_add_str_equals);
	tcase_add_test(t, sbuf_add_str_rparen);
	tcase_add_test(t, sbuf_add_str_scolon);
	tcase_add_test(t, sbuf_add_str_tspace);
	tcase_add_test(t, sbuf_add_str_lparen_comma);
	tcase_add_test(t, sbuf_add_str_quote_comma);
	tcase_add_test(t, sbuf_add_str_quote_rparen_scolon);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("sbuf_add_unum");
	tcase_add_test(t, sbuf_add_unum_too_long);
	tcase_add_test(t, test_sbuf_add_unum);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("sbuf_add_snum");
	tcase_add_test(t, sbuf_add_snum_too_long);
	tcase_add_test(t, test_sbuf_add_snum);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("sbuf_add_param_str");
	tcase_add_test(t, sbuf_add_param_str_null_params);
	tcase_add_test(t, sbuf_add_param_str_empty_value);
	tcase_add_test(t, test_sbuf_add_param_str);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("sbuf_add_param_num");
	tcase_add_test(t, sbuf_add_param_num_null_params);
	tcase_add_test(t, test_sbuf_add_param_num);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("sbuf_add_param_snum");
	tcase_add_test(t, sbuf_add_param_snum_null_params);
	tcase_add_test(t, test_sbuf_add_param_snum);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

