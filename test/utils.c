/**
 * Minimal Migration Manager - Utils Tests
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

#include "../src/utils.h"
#include "../src/utils.c"

/* Test samples */
static char *test    = "Test";
static char *tset    = "Tset";
static char *test_1  = "1-test";
static char *tset_1  = "1-tset";
static char *test_99 = "99-test";
static char *test_1s = "1-test.sql";
static char *one_sql = "1.sql";
static char *one_xxx = "1-xxx.sql";

/**
 * Test that calling bubblesort() with an array containing
 * one string, or NULL, works.
 */
static void bubblesort_one_string(void)
{
	char *a[1];

	a[0] = test;
	bubblesort(a, 1);
	bubblesort(NULL, 0);
	CU_ASSERT_EQUAL(a[0], test);

	a[0] = test_1;
	bubblesort(a, 1);
	CU_ASSERT_EQUAL(a[0], test_1);

	a[0] = NULL;
	bubblesort(a, 1);
	CU_ASSERT_PTR_NULL(a[0]);
}

/**
 * Test that bubblesort() correctly sorts an array of two
 * strings.
 */
static void bubblesort_two_strings(void)
{
	char *a[2];

	/* The array is already sorted - no designation */
	a[0] = test;
	a[1] = tset;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], test);
	CU_ASSERT_EQUAL(a[1], tset);

	/* The array is in revers order - no designation */
	a[0] = tset;
	a[1] = test;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], test);
	CU_ASSERT_EQUAL(a[1], tset);

	/* The array is already sorted - with designations */
	a[0] = test_1;
	a[1] = tset_1;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], test_1);
	CU_ASSERT_EQUAL(a[1], tset_1);

	/* The array is in reverse order - with designation */
	a[0] = tset_1;
	a[1] = test_1;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], test_1);
	CU_ASSERT_EQUAL(a[1], tset_1);

	/* The array is already sorted - mixed designations */
	a[0] = test_1;
	a[1] = tset;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], test_1);
	CU_ASSERT_EQUAL(a[1], tset);

	/* The array is in reverse order - mixed designations */
	a[0] = tset;
	a[1] = test_1;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], test_1);
	CU_ASSERT_EQUAL(a[1], tset);

	/* The array is already sorted - differing designations */
	a[0] = test_1;
	a[1] = test_99;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], test_1);
	CU_ASSERT_EQUAL(a[1], test_99);

	/* The array is in reverse order - differing designations */
	a[0] = test_99;
	a[1] = test_1;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], test_1);
	CU_ASSERT_EQUAL(a[1], test_99);
}

/**
 * Test that if bubblesort() gets two strings, with equal designations,
 * that if one of the two only contains the deisgnation and .sql, that
 * it is pushed farther towards the beginning of the array.
 */
static void bubblesort_sql(void)
{
	char *a[2];

	a[0] = test_1s;
	a[1] = one_sql;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], one_sql);
	CU_ASSERT_EQUAL(a[1], test_1s);

	a[0] = one_xxx;
	a[1] = one_sql;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], one_sql);
	CU_ASSERT_EQUAL(a[1], one_xxx);

	a[0] = one_sql;
	a[1] = one_xxx;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], one_sql);
	CU_ASSERT_EQUAL(a[1], one_xxx);
}

/**
 * Test that bubblesort() can handle designations that would result
 * in strtoul() erroring out with ERANGE.
 */
static void bubblesort_erange(void)
{
	char *a[2];
	char s[50];

	/* This should cause strtoul() to set errno to ERANGE */
	sprintf(s, "0%lu9-aaa.sql", ULONG_MAX);

	a[0] = s;
	a[1] = one_sql;
	bubblesort(a, 2);
	CU_ASSERT_EQUAL(a[0], one_sql);
	CU_ASSERT_EQUAL(a[1], s);
}

static CU_TestInfo utils_tests[] = {
	{
		"bubblesort() - 1 string",
		bubblesort_one_string
	},

	{
		"bubblesort() - 2 strings",
		bubblesort_two_strings
	},

	{
		"bubblesort() - .sql extension",
		bubblesort_sql
	},

	{
		"bubblesort() - ERANGE",
		bubblesort_erange
	},

	CU_TEST_INFO_NULL
};

void utils_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Utils", NULL, NULL);
	while (utils_tests[i].pName) {
		CU_add_test(suite, utils_tests[i].pName,
		            utils_tests[i].pTestFunc);
		i++;
	}
}

