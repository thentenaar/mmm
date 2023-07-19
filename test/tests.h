/**
 * Minimal Migration Manager - Test Suites
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef TESTS_H
#define TESTS_H

#include <check.h>

#ifndef IN_TESTS
#define IN_TESTS
#endif

Suite *config_suite(void);
Suite *db_suite(void);
Suite *source_suite(void);
Suite *config_gen_suite(void);
Suite *file_suite(void);
Suite *utils_suite(void);
Suite *stringbuf_suite(void);
Suite *state_suite(void);
Suite *source_file_suite(void);
Suite *source_git_suite(void);
Suite *db_sqlite3_suite(void);
Suite *db_pgsql_suite(void);
Suite *db_mysql_suite(void);
Suite *migration_suite(void);
Suite *commands_suite(void);

#endif /* TESTS_H */

