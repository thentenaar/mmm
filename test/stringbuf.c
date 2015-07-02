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
#include <CUnit/CUnit.h>
#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

#include "../src/stringbuf.h"
#include "../src/stringbuf.c"

/**
 * Test that sbuf_reset() should set offset to 0, and only
 * overwrite the first byte of the buffer with '\0'.
 */
static void sbuf_reset_no_scrub(void)
{
	sbuf[0] = 'x';
	sbuf[1] = 'x';
	offset = 1;

	sbuf_reset(0);
	CU_ASSERT_EQUAL(offset, 0);
	CU_ASSERT_EQUAL(sbuf[0], '\0');
	CU_ASSERT_EQUAL(sbuf[1], 'x');
}

/**
 * Test that sbuf_reset() should scrub the buffer only up until
 * offset.
 */
static void sbuf_reset_scrub_till_offset(void)
{
	sbuf[0] = 'x';
	sbuf[1] = 'x';
	sbuf[2] = 'x';
	offset = 2;

	sbuf_reset(1);
	CU_ASSERT_EQUAL(offset, 0);
	CU_ASSERT_EQUAL(sbuf[0], '\0');
	CU_ASSERT_EQUAL(sbuf[1], '\0');
	CU_ASSERT_EQUAL(sbuf[2], 'x');
}

/**
 * Test that sbuf_reset() should scrub the whole buffer.
 */
static void sbuf_reset_scrub_all(void)
{
	offset = 0;
	memset(sbuf, 'x', SBUFSIZ);
	sbuf_reset(1);
	CU_ASSERT_EQUAL(sbuf[SBUFSIZ-1], '\0');
	CU_ASSERT_EQUAL(sbuf[SBUFSIZ >> 1], '\0');
	CU_ASSERT_EQUAL(sbuf[0], '\0');
}

/**
 * Test that sbuf_get_buffer() returns a pointer to sbuf.
 */
static void test_sbuf_get_buffer(void)
{
	CU_ASSERT_EQUAL(sbuf_get_buffer(), sbuf);
}

/**
 * Test that sbuf_add_str() returns 1 if a NULL
 * argument for s is passed.
 */
static void sbuf_add_str_null_string(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(1, sbuf_add_str(NULL, 0, 0));
	CU_ASSERT_EQUAL(offset, 0);
}

/**
 * Test that sbuf_add_str() returns 1 if the string
 * in s is too long to fit in the buffer.
 */
static void sbuf_add_str_s_too_long(void)
{
	sbuf_reset(0);
	offset = SBUFSIZ - 7;
	CU_ASSERT_EQUAL(1, sbuf_add_str(".", 0, 0));
	CU_ASSERT_EQUAL(offset, SBUFSIZ - 7);

	offset = SBUFSIZ - 6;
	CU_ASSERT_EQUAL(1, sbuf_add_str(".", 0, 0));
	CU_ASSERT_EQUAL(offset, SBUFSIZ - 6);
}

/**
 * If no formatting and no string are specified,
 * sbuf_add_str() should be a no-op.
 */
static void sbuf_add_str_no_string_or_formatting(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_str("", 0, 0));
	CU_ASSERT_EQUAL(offset, 0);
}

/**
 * Test that sbuf_add_str() adds the specified string.
 */
static void sbuf_add_str_adds_string_no_formatting(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_str("xxx", 0, 0));
	CU_ASSERT_EQUAL(offset, 3);
	CU_ASSERT_STRING_EQUAL(sbuf, "xxx");
}

/**
 * Test that sbuf_add_str() only adds lparen if s
 * is an empty string, and the requisite format was specified.
 */
static void sbuf_add_str_lparen(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_str("", SBUF_LPAREN, 0));
	CU_ASSERT_EQUAL(offset, 1);
	CU_ASSERT_STRING_EQUAL("(", sbuf);
}

/**
 * Test that sbuf_add_str() only adds a leading space if s
 * is an empty string, and the requisite format was specified.
 */
static void sbuf_add_str_lspace(void)
{
	sbuf_reset(0);
	sbuf[offset++] = 'x';

	CU_ASSERT_EQUAL(0, sbuf_add_str("", SBUF_LSPACE, 0));
	CU_ASSERT_EQUAL(offset, 2);
	CU_ASSERT_STRING_EQUAL("x ", sbuf);
}

/**
 * Test that sbuf_add_str() only adds quotes if s
 * is an empty string, and the requisite format was specified.
 */
static void sbuf_add_str_quotes(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_str("", SBUF_QUOTE, 0));
	CU_ASSERT_EQUAL(offset, 2);
	CU_ASSERT_STRING_EQUAL("''", sbuf);
}

/**
 * Test that sbuf_add_str() only adds a comma if s
 * is an empty string, and the requisite format was specified.
 */
static void sbuf_add_str_comma(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_str("", SBUF_COMMA, 0));
	CU_ASSERT_EQUAL(offset, 1);
	CU_ASSERT_STRING_EQUAL(",", sbuf);
}

/**
 * Test that sbuf_add_str() only adds a equals if s
 * is an empty string, and the requisite format was specified.
 */
static void sbuf_add_str_equals(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_str("", SBUF_EQUALS, 0));
	CU_ASSERT_EQUAL(offset, 1);
	CU_ASSERT_STRING_EQUAL("=", sbuf);
}

/**
 * Test that sbuf_add_str() only adds a rparen if s
 * is an empty string, and the requisite format was specified.
 */
static void sbuf_add_str_rparen(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_str("", SBUF_RPAREN, 0));
	CU_ASSERT_EQUAL(offset, 1);
	CU_ASSERT_STRING_EQUAL(")", sbuf);
}

/**
 * Test that sbuf_add_str() only adds a semicolon if s
 * is an empty string, and the requisite format was specified.
 */
static void sbuf_add_str_scolon(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_str("", SBUF_SCOLON, 0));
	CU_ASSERT_EQUAL(offset, 1);
	CU_ASSERT_STRING_EQUAL(";", sbuf);
}

/**
 * Test that sbuf_add_str() only adds a trailing space if s
 * is an empty string, and the requisite format was specified.
 */
static void sbuf_add_str_tspace(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_str("", SBUF_TSPACE, 0));
	CU_ASSERT_EQUAL(offset, 1);
	CU_ASSERT_STRING_EQUAL(" ", sbuf);
}

/**
 * Test that sbuf_add_str() adds a lparen and comma with the
 * specified string in-between.
 */
static void sbuf_add_str_lparen_and_comma(void)
{
	int i;

	sbuf_reset(0);
	i = sbuf_add_str("x", SBUF_LPAREN | SBUF_COMMA, 0);
	CU_ASSERT_EQUAL(i, 0);
	CU_ASSERT_EQUAL(offset, 3);
	CU_ASSERT_STRING_EQUAL("(x,", sbuf);
}

/**
 * Test that sbuf_add_str() adds quotes around the specified
 * string, and a comma afterwards.
 */
static void sbuf_add_str_quote_and_comma(void)
{
	int i;

	sbuf_reset(0);
	i = sbuf_add_str("x", SBUF_QUOTE | SBUF_COMMA, 0);
	CU_ASSERT_EQUAL(i, 0);
	CU_ASSERT_EQUAL(offset, 4);
	CU_ASSERT_STRING_EQUAL("'x',", sbuf);
}

/**
 * Test that sbuf_add_str() adds quotes around the specified
 * string, a rparen and then a semicolon afterwards.
 */
static void sbuf_add_str_quote_rparen_and_scolon(void)
{
	unsigned int i = SBUF_QUOTE | SBUF_RPAREN | SBUF_SCOLON;

	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_str("x", i, 0));
	CU_ASSERT_EQUAL(offset, 5);
	CU_ASSERT_STRING_EQUAL("'x');", sbuf);
}

/**
 * Test that sbuf_add_unum() returns 1 if there's
 * insufficient space in the buffer for the number.
 */
static void sbuf_add_unum_num_too_long(void)
{
	sbuf_reset(0);
	offset = SBUFSIZ - 5;
	CU_ASSERT_EQUAL(1, sbuf_add_unum(ULONG_MAX, 0));
}

/**
 * Test that sbuf_add_unum() successfully adds numbers.
 */
static void test_sbuf_add_unum(void)
{
	char nbuf[50];

	sbuf_reset(0);
	sprintf(nbuf, "%lu", ULONG_MAX);
	CU_ASSERT_EQUAL(0, sbuf_add_unum(ULONG_MAX, 0));
	CU_ASSERT_STRING_EQUAL(sbuf, nbuf);
	CU_ASSERT_EQUAL(offset, strlen(nbuf));

	sbuf_reset(0);
	sprintf(nbuf, "%lu", ULONG_MAX >> 1);
	CU_ASSERT_EQUAL(0, sbuf_add_unum(ULONG_MAX >> 1, 0));
	CU_ASSERT_STRING_EQUAL(sbuf, nbuf);
	CU_ASSERT_EQUAL(offset, strlen(nbuf));

	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_unum(1, 0));
	CU_ASSERT_STRING_EQUAL(sbuf, "1");
	CU_ASSERT_EQUAL(offset, 1);

	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_unum(0, 0));
	CU_ASSERT_STRING_EQUAL(sbuf, "0");
	CU_ASSERT_EQUAL(offset, 1);
}

/**
 * Test that sbuf_add_snum() returns 1 if there's
 * insufficient space in the buffer for the number.
 */
static void sbuf_add_snum_num_too_long(void)
{
	sbuf_reset(0);
	offset = SBUFSIZ - 3;
	CU_ASSERT_EQUAL(1, sbuf_add_snum(LONG_MAX, 0));

	sbuf_reset(0);
	offset = SBUFSIZ - 3;
	CU_ASSERT_EQUAL(1, sbuf_add_snum(-LONG_MAX, 0));
}

/**
 * Test that sbuf_add_snum() successfully adds numbers.
 */
static void test_sbuf_add_snum(void)
{
	char nbuf[50];

	sbuf_reset(0);
	sprintf(nbuf, "%ld", LONG_MAX);
	CU_ASSERT_EQUAL(0, sbuf_add_snum(LONG_MAX, 0));
	CU_ASSERT_STRING_EQUAL(sbuf, nbuf);
	CU_ASSERT_EQUAL(offset, strlen(nbuf));

	sbuf_reset(0);
	sprintf(nbuf, "%ld", -LONG_MAX);
	CU_ASSERT_EQUAL(0, sbuf_add_snum(-LONG_MAX, 0));
	CU_ASSERT_STRING_EQUAL(sbuf, nbuf);
	CU_ASSERT_EQUAL(offset, strlen(nbuf));

	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_snum(-1, 0));
	CU_ASSERT_STRING_EQUAL(sbuf, "-1");
	CU_ASSERT_EQUAL(offset, 2);

	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_snum(0, 0));
	CU_ASSERT_STRING_EQUAL(sbuf, "0");
	CU_ASSERT_EQUAL(offset, 1);
}

/**
 * Test that sbuf_add_param_str() returns 1 if either of its
 * parameters is NULL.
 */
static void sbuf_add_param_str_null_params(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(1, sbuf_add_param_str(NULL, "test"));
	CU_ASSERT_EQUAL(offset, 0);

	sbuf_reset(0);
	CU_ASSERT_EQUAL(1, sbuf_add_param_str("test", NULL));
	CU_ASSERT_EQUAL(offset, 0);

	sbuf_reset(0);
	CU_ASSERT_EQUAL(1, sbuf_add_param_str(NULL, NULL));
	CU_ASSERT_EQUAL(offset, 0);
}

/**
 * Test that sbuf_add_param_str() with a empty value
 * is basically a no-op.
 */
static void sbuf_add_param_str_empty_value(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(1, sbuf_add_param_str("test", ""));
	CU_ASSERT_EQUAL(offset, 0);
}

/**
 * Test that sbuf_add_param_str() works as expected.
 */
static void test_sbuf_add_param_str(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_param_str("test", "value"));
	CU_ASSERT_STRING_EQUAL(sbuf, "test='value'");
	CU_ASSERT_EQUAL(offset, 12);
}

/**
 * Test that sbuf_add_param_num() returns 1 if param is NULL.
 */
static void sbuf_add_param_num_null_param(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(1, sbuf_add_param_num(NULL, 1));
	CU_ASSERT_EQUAL(offset, 0);
}

/**
 * Test that sbuf_add_param_num() works as expected.
 */
static void test_sbuf_add_param_num(void)
{
	sbuf_reset(0);
	CU_ASSERT_EQUAL(0, sbuf_add_param_num("test", 100));
	CU_ASSERT_STRING_EQUAL(sbuf, "test=100");
	CU_ASSERT_EQUAL(offset, 8);
}

static CU_TestInfo stringbuf_tests[] = {
	{
		"sbuf_reset() - without scrub",
		sbuf_reset_no_scrub
	},
	{
		"sbuf_reset() - scrub until offset",
		sbuf_reset_scrub_till_offset
	},
	{
		"sbuf_reset() - full scrub",
		sbuf_reset_scrub_all
	},
	{
		"sbuf_get_buffer() - works",
		test_sbuf_get_buffer
	},
	{
		"sbuf_add_str() - NULL string",
		sbuf_add_str_null_string
	},
	{
		"sbuf_add_str() - string too long",
		sbuf_add_str_s_too_long
	},
	{
		"sbuf_add_str() - empty string / no formatting",
		sbuf_add_str_no_string_or_formatting
	},
	{
		"sbuf_add_str() - adds string / no formatting",
		sbuf_add_str_adds_string_no_formatting
	},
	{
		"sbuf_add_str() - formatting: LPAREN",
		sbuf_add_str_lparen
	},
	{
		"sbuf_add_str() - formatting: LSPACE",
		sbuf_add_str_lspace
	},
	{
		"sbuf_add_str() - formatting: QUOTE",
		sbuf_add_str_quotes
	},
	{
		"sbuf_add_str() - formatting: COMMA",
		sbuf_add_str_comma
	},
	{
		"sbuf_add_str() - formatting: EQUALS",
		sbuf_add_str_equals
	},
	{
		"sbuf_add_str() - formatting: RPAREN",
		sbuf_add_str_rparen
	},
	{
		"sbuf_add_str() - formatting: SCOLON",
		sbuf_add_str_scolon
	},
	{
		"sbuf_add_str() - formatting: TSPACE",
		sbuf_add_str_tspace
	},
	{
		"sbuf_add_str() - formatting: LPAREN + COMMA",
		sbuf_add_str_lparen_and_comma
	},
	{
		"sbuf_add_str() - formatting: QUOTE + COMMA",
		sbuf_add_str_quote_and_comma
	},
	{
		"sbuf_add_str() - formatting: QUOTE + RPAREN + SCOLON",
		sbuf_add_str_quote_rparen_and_scolon
	},
	{
		"sbuf_add_unum() - number too long",
		sbuf_add_unum_num_too_long
	},
	{
		"sbuf_add_unum() - adds number",
		test_sbuf_add_unum
	},
	{
		"sbuf_add_snum() - number too long",
		sbuf_add_snum_num_too_long
	},
	{
		"sbuf_add_snum() - adds number",
		test_sbuf_add_snum
	},
	{
		"sbuf_add_param_str() - NULL params",
		sbuf_add_param_str_null_params
	},
	{
		"sbuf_add_param_str() - empty value",
		sbuf_add_param_str_empty_value
	},
	{
		"sbuf_add_param_str() - works",
		test_sbuf_add_param_str
	},
	{
		"sbuf_add_param_num() - NULL param",
		sbuf_add_param_num_null_param
	},
	{
		"sbuf_add_param_num() - works",
		test_sbuf_add_param_num
	},

	CU_TEST_INFO_NULL
};

void stringbuf_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("String Buffer", NULL, NULL);
	while (stringbuf_tests[i].pName) {
		CU_add_test(suite, stringbuf_tests[i].pName,
		            stringbuf_tests[i].pTestFunc);
		i++;
	}
}

