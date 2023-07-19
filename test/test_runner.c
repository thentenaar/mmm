/**
 * Minimal Migration Manager - Test Runner
 *
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licensed under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <check.h>
#include "tests.h"

/* Global error buffer */
char errbuf[4096];

int main(void)
{
	int failed;

	SRunner *sr = srunner_create(NULL);
	srunner_add_suite(sr, config_suite());
	srunner_add_suite(sr, db_suite());
	srunner_add_suite(sr, source_suite());
	srunner_add_suite(sr, config_gen_suite());
	srunner_add_suite(sr, file_suite());
	srunner_add_suite(sr, utils_suite());
	srunner_add_suite(sr, stringbuf_suite());
	srunner_add_suite(sr, state_suite());
	srunner_add_suite(sr, source_file_suite());
	srunner_add_suite(sr, source_git_suite());
	srunner_add_suite(sr, db_sqlite3_suite());
	srunner_add_suite(sr, db_pgsql_suite());
	srunner_add_suite(sr, db_mysql_suite());
	srunner_add_suite(sr, migration_suite());
	srunner_add_suite(sr, commands_suite());

	srunner_run_all(sr, CK_ENV);
	failed = srunner_ntests_failed(sr);
	srunner_free(sr);
	return failed ? EXIT_FAILURE : EXIT_SUCCESS;
}

