/**
 * Minimal Migration Manager - Test Runner
 *
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licensed under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>

#include <CUnit/CUnit.h>
#include "tests.h"

/* Global error buffer */
char errbuf[4096];

/**
 * This will be non-zero if any tests failed when
 * not run in curses mode.
 */
static int had_failures = 0;

/**
 * This function is called after each test completes.
 */
static void test_complete_cb(const CU_pTest UNUSED(test),
                             const CU_pSuite UNUSED(suite),
                             const CU_pFailureRecord failure)
{
	printf("%c", failure ? 'F' : '.');

	/* Reset errno for the next suite */
	errno = -1;
}

/**
 * Print each assertion failure for the same test as another
 * failure.
 *
 * This takes advantage of the fact that elements in the
 * linked list for the same test are contiguous.
 */
static CU_pFailureRecord print_failure(CU_pFailureRecord f)
{
	CU_pFailureRecord tmp;

	if (!f) return f;
	puts("\n\n"
	     "======================================================");
	printf("FAIL: %s: %s\n", f->pSuite->pName, f->pTest->pName);
	puts("------------------------------------------------------");

	for (tmp=f;tmp;tmp=tmp->pNext) {
		if (tmp->pTest != f->pTest) {
			tmp = tmp->pPrev;
			break;
		}

		printf("%s:%u\n    %s\n", tmp->strFileName,
		       tmp->uiLineNumber, tmp->strCondition);
	}
	puts("------------------------------------------------------");
	return tmp ? tmp->pNext : tmp;
}

/**
 * This function is called after all tests complete.
 */
static void all_tests_done_cb(CU_pFailureRecord failure)
{
	unsigned int n_tests    = CU_get_number_of_tests_run();
	unsigned int n_failures = CU_get_number_of_failures();

	while (failure) {
		failure = print_failure(failure);
		had_failures++;
	}

	if (!had_failures) printf("\n");
#if TEST_CU_VER < 212
	printf("\nRan %u test%s\n",
	       n_tests, (n_tests > 1 ? "s" : ""));
#else
	printf("\nRan %u test%s in %.5fs\n",
	       n_tests, (n_tests > 1 ? "s" : ""),
	       CU_get_elapsed_time());
#endif

	if (n_failures)
		printf("\nFAILED (failures=%u)\n", n_failures);
	else puts("\nOK");
}

static void run_tests(void)
{
	CU_set_test_complete_handler(test_complete_cb);
	CU_set_all_test_complete_handler(all_tests_done_cb);
	CU_run_all_tests();
}

int main(int UNUSED(argc), char * UNUSED(argv[]))
{
	if (CU_initialize_registry()) {
		fprintf(stderr, "Error initializing test registry\n");
		return 0;
	}

	/* Set errno for the first suite */
	errno = -1;

	/* Add test suites */
	config_add_suite();
	db_add_suite();
	source_add_suite();
	config_gen_add_suite();
	file_add_suite();
	utils_add_suite();
	stringbuf_add_suite();
	state_add_suite();
	source_file_add_suite();
	source_git_add_suite();
	db_sqlite3_add_suite();
	db_pgsql_add_suite();
	db_mysql_add_suite();
	migration_add_suite();
	commands_add_suite();

	/* Run them */
	run_tests();
	CU_cleanup_registry();
	if (had_failures) return 1;
	return 0;
}

