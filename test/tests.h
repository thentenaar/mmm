/**
 * Minimal Migration Manager - Test Suites
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef TESTS_H
#define TESTS_H

#ifndef IN_TESTS
#define IN_TESTS
#endif

/* {{{ GCC: UNUSED() macro */
#if defined(__GNUC__) || defined(__clang__)
#define UNUSED(x) x __attribute__((__unused__))
#else
#define UNUSED(x) x
#endif /* }}} */

/* Suite adding functions */
void config_add_suite(void);
void db_add_suite(void);
void source_add_suite(void);
void config_gen_add_suite(void);
void file_add_suite(void);
void utils_add_suite(void);
void stringbuf_add_suite(void);
void state_add_suite(void);
void source_file_add_suite(void);
void source_git_add_suite(void);
void db_sqlite3_add_suite(void);
void db_pgsql_add_suite(void);
void db_mysql_add_suite(void);
void migration_add_suite(void);
void commands_add_suite(void);

#endif /* TESTS_H */

