/**
 * Minimal Migration Manager - Configuration Parser Tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>
#include <check.h>

#include "tests.h"

/* from test_runner.c */
extern char errbuf[];

#include "../src/config.h"

/* {{{ DB stubs */
/**
 * This stuff allows us to track which callback is called from
 * handle_token(), and in which order. Also, we can easily
 * simulate the effect of not having any db or source callback
 * to call.
 */
static int db_called          = 0;
static int db_called_for_main = 0;
static int main_config_called = 0;

static void check_state_valid_for_cb(void);

static void main_config_proc(void)
{
	check_state_valid_for_cb();
	main_config_called++;
}

static void db_config_proc(void)
{
	check_state_valid_for_cb();
	db_called++;
}

static void source_config_proc(void)
{
	return;
}

static config_callback_t source_get_config_cb(const char *start,
                                              size_t end)
{
	(void)start;
	(void)end;
	return source_config_proc;
}

static config_callback_t db_get_config_cb(const char *start, size_t end)
{
	if (end == 4 && !memcmp(start, "main", end))
		db_called_for_main++;
	if (db_called) return NULL;
	return db_config_proc;
}
/* }}} */

#include "../src/config.c"

/**
 * Ensure that the state is valid when a callback
 * is called.
 */
static void check_state_valid_for_cb(void)
{
	/* Ensure we have a valid section, key, and value */
	ck_assert_ptr_nonnull(state.section);
	ck_assert_ptr_nonnull(state.key);
	ck_assert_ptr_nonnull(state.value);
	ck_assert_uint_ne(state.section_len, 0);
	ck_assert_uint_ne(state.key_len, 0);
	ck_assert_uint_ne(state.value_len, 0);
}

/**
 * Test that config_init() actually sets the main_config
 * address correctly.
 */
START_TEST(test_config_init)
{
	main_config_called = 0;
	main_config = NULL;
	config_init(main_config_proc);
	ck_assert(main_config == main_config_proc);
}
END_TEST

/**
 * Test that config_set_value() returns 0 if passed invalid
 * parameters, and that the supplied dest buffer is not
 * modified.
 */
START_TEST(config_set_value_params)
{
	size_t dest_len;

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "value";
	state.value_len   = 5;
	dest_len          = state.key_len + 1;

	/* !key_len */
	ck_assert_int_eq(config_set_value(0, errbuf, dest_len, "test", 0), 1);

	/* state.key_len (4) != key_len (3)  */
	ck_assert_int_eq(config_set_value(0, errbuf, dest_len, "test", 3), 1);

	/* !dest */
	ck_assert_int_eq(config_set_value(0, NULL, dest_len, "test", 4), 1);

	/* !expected_key */
	ck_assert_int_eq(config_set_value(0, errbuf, dest_len, NULL, 4), 1);

	/* !dest_size */
	ck_assert_int_eq(config_set_value(0, errbuf, 0, "test", 4), 1);

	/* !state.value_len */
	state.value_len = 0;
	ck_assert_int_eq(config_set_value(0, errbuf, dest_len, "test", 4), 1);

	/* !state.key_len */
	state.value_len = 5;
	state.key_len   = 0;
	ck_assert_int_eq(config_set_value(0, errbuf, dest_len, "test", 4), 1);

	/* !state.key */
	state.key_len = 4;
	state.key     = NULL;
	ck_assert_int_eq(config_set_value(0, errbuf, dest_len, "test", 4), 1);

	/* !state.value */
	state.key_len = 4;
	state.key     = state.value;
	state.value   = NULL;
	ck_assert_int_eq(config_set_value(0, errbuf, dest_len, "test", 4), 1);

	/* Ensure nothing wrote to errbuf */
	ck_assert(!*errbuf);
}
END_TEST

/**
 * Test that config_set_value() returns 0 if the
 * expected_key and state.key have the same length
 * but different content.
 */
START_TEST(config_set_value_wrong_key)
{
	size_t dest_len;

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "value";
	state.value_len   = 5;
	dest_len          = state.key_len + 1;

	/* "xxx" != "test" */
	ck_assert_int_eq(config_set_value(CONFIG_STRING, errbuf, dest_len, "xxxx", 4), 1);

	/* Ensure nothing wrote to errbuf */
	ck_assert(!*errbuf);
}
END_TEST

/**
 * Test that the correct error message is generated if dest is too
 * small to hold the contents of state.value.
 */
START_TEST(config_set_value_string_too_big)
{
	size_t dest_len;
	const char *err = "config: section.test: value too long\n";

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "value";
	state.value_len   = 5;
	dest_len          = state.value_len - 1;

	ck_assert_int_eq(config_set_value(CONFIG_STRING, errbuf, dest_len, "test", 4), 1);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that set_value_string() correctly copies state.value
 * to the destination.
 */
START_TEST(config_set_value_string)
{
	size_t dest_len;
	const char *value = "value";

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = value;
	state.value_len   = strlen(value);
	dest_len          = state.value_len + 1;

	/* strlen(value) == strlen(dest) */
	ck_assert_int_eq(config_set_value(CONFIG_STRING, errbuf, dest_len, "test", 4), 0);

	/* Ensure value was copied. */
	ck_assert_str_eq(errbuf, value);

	/* strlen(dest) > strlen(value) */
	memset(errbuf, 0, dest_len + 5);
	dest_len <<= 1;
	ck_assert_int_eq(config_set_value(CONFIG_STRING, errbuf, dest_len, "test", 4), 0);

	/* Ensure the value was copied. */
	ck_assert_str_eq(errbuf, value);
}
END_TEST

/**
 * Test that the correct error message is generated if value
 * doesn't represent a number.
 */
START_TEST(config_set_value_number_not_numeric)
{
	size_t dest_len;
	const char *err = "config: section.test: number expected\n";

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "value";
	state.value_len   = 5;
	dest_len          = state.value_len;

	ck_assert_int_eq(config_set_value(CONFIG_NUMBER, errbuf, dest_len, "test", 4), 1);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that the correct error message is generated if value
 * represents a number that is negative, or that cannot be
 * parsed as a base 10 integer by strtoul().
 */
START_TEST(config_set_value_number_not_unsigned)
{
	size_t dest_len;
	const char *err = "config: section.test: number expected\n";

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "-1234";
	state.value_len   = 5;
	dest_len          = sizeof(unsigned long);

	/* Check a negative number */
	ck_assert_int_eq(config_set_value(CONFIG_NUMBER, errbuf, dest_len, "test", 4), 1);
	ck_assert_str_eq(errbuf, err);

	/* Check a non-base-10 number too */
	*errbuf     = '\0';
	state.value = "0x234";
	ck_assert_int_eq(config_set_value(CONFIG_NUMBER, errbuf, dest_len, "test", 4), 1);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that the correct error message is generated if value
 * represents a number that is negative, or that cannot be
 * parsed as a base 10 integer by strtoul().
 */
START_TEST(config_set_value_number_out_of_range)
{
	size_t dest_len;
	char value[255];

	*errbuf = '\0';
	sprintf(value, "%lu1\n", ULONG_MAX);
	memset(&state, 0, sizeof state);
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = value;
	state.value_len   = strlen(value);
	dest_len          = sizeof(unsigned long);

	/* The number should be larger than ULONG_MAX. */
	ck_assert_int_eq(config_set_value(CONFIG_NUMBER, errbuf, dest_len, "test", 4), 1);

	/* Ensure the expected error message was generated. */
	sprintf(value, "config: section.test: %s\n", strerror(ERANGE));
	ck_assert_str_eq(errbuf, value);
}
END_TEST

/**
 * Test that the correct error message is generated if the
 * size of dest is larger than sizeof(unsigned long).
 */
START_TEST(config_set_value_number_bad_dest_size)
{
	const char *err = "config: section.test: bad dest size\n";

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "12345";
	state.value_len   = 5;

	/* > sizeof(unsigned long) */
	ck_assert_int_eq(config_set_value(CONFIG_NUMBER, errbuf, sizeof(unsigned long) << 1, "test", 4), 1);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that set_value_number() works correctly.
 */
START_TEST(config_set_value_number)
{
	size_t stval;
	unsigned ivalue;
	unsigned short svalue;
	unsigned long value;

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "12345";
	state.value_len   = 5;

	/* char */
	ck_assert_int_eq(config_set_value(CONFIG_NUMBER, errbuf, 1, "test", 4), 0);
	ck_assert(*errbuf == (char)12345);

	/* unsigned */
	ck_assert_int_eq(config_set_value(CONFIG_NUMBER, &ivalue, sizeof ivalue, "test", 4), 0);
	ck_assert_int_eq(ivalue, 12345);

	/* unsigned short */
	ck_assert_int_eq(config_set_value(CONFIG_NUMBER, &svalue, sizeof svalue, "test", 4), 0);
	ck_assert_int_eq(svalue, 12345);

	/* unsigned long */
	ck_assert_int_eq(config_set_value(CONFIG_NUMBER, &value, sizeof value, "test", 4), 0);
	ck_assert_uint_eq(value, 12345);

	/* size_t */
	ck_assert_int_eq(config_set_value(CONFIG_NUMBER, &stval, sizeof stval, "test", 4), 0);
	ck_assert_uint_eq(stval, 12345);
}
END_TEST

/**
 * Test that parse_config() returns 1 when given
 * invalid parameters.
 */
START_TEST(parse_config_invalid_params)
{
	ck_assert_int_eq(parse_config(NULL, 1), 1);
	ck_assert_int_eq(parse_config("xxx", 0), 1);
}
END_TEST

/**
 * Test that parse_config() returns 0 when given
 * an empty string.
 */
START_TEST(parse_config_empty_string)
{
	ck_assert_int_eq(parse_config("", 1), 0);
}
END_TEST

/**
 * Test that parse_config() returns 0 when it
 * encounters a null byte within a string.
 */
START_TEST(parse_config_null_within_string)
{
	ck_assert_int_eq(parse_config("\n\0\n", 3), 0);
}
END_TEST

/**
 * Test that parse_config() ignores lines
 * beginning with a comment.
 */
START_TEST(parse_config_ignores_comments)
{
	ck_assert_int_eq(parse_config("; Comment\n", 10), 0);
}
END_TEST

/**
 * Test that parse_config() fails on an invalid
 * identifier for a section name (i.e. not [a-z0-9_]).
 */
START_TEST(parse_config_invalid_section)
{
	const char *err =
	"config: [Line: 1, Char: 2] Invalid Section\n";
	char str[] = "[Atest]\n";
	int c;

	/* Check anything not [a-z0-9_], whitespace, or a comment. */
	memset(&state, 0, sizeof state);
	for (c = 1; c <= 0xff; c++) {
		if (isspace(c) || isdigit(c) || islower(c) || c == '_' || c == ';')
			continue;

		str[1] = (char)c;
		state.section = NULL;
		state.section_len = 0;

		ck_assert_int_eq(parse_config(str, strlen(str)), 1);
		ck_assert_ptr_null(state.section);
		ck_assert_uint_eq(state.section_len, 0);
		ck_assert_str_eq(errbuf, err);
	}

	/* Check if the name would run past the end of the string */
	str[1] = 'a';
	str[6] = 'x';
	str[7] = 'z';

	ck_assert_int_eq(parse_config(str, strlen(str)), 1);
	ck_assert_ptr_null(state.section);
	ck_assert_uint_eq(state.section_len, 0);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that the config parser uses the main callback
 * when it encounters the main section.
 */
START_TEST(parse_config_main_section_cb)
{
	const char *str = "[main]\n";

	/* Ensure the section was parsed correctly */
	memset(&state, 0, sizeof state);
	db_called = db_called_for_main = 0;
	main_config = main_config_proc;
	ck_assert_int_eq(parse_config(str, strlen(str)), 0);
	ck_assert_uint_eq(state.section_len, 4);
	ck_assert_mem_eq(state.section, "main", 4);

	/* Check for the callback */
	ck_assert(state.cb == main_config_proc);
	ck_assert(!db_called_for_main);

	/* If no main callback is set, check that db gets called. */
	memset(&state, 0, sizeof state);
	main_config = NULL;
	db_called = db_called_for_main = 0;
	ck_assert_int_eq(parse_config(str, strlen(str)), 0);

	/* Check for the callback */
	ck_assert(state.cb == db_config_proc);
	ck_assert(!!db_called_for_main);
}
END_TEST

/**
 * Test that the config parser first calls the db
 * lookup function, and then the source lookup
 * function when it encounters a section other
 * than main.
 */
START_TEST(parse_config_non_main_section_cb)
{
	const char *str = "[test]\n";

	memset(&state, 0, sizeof state);
	main_config = main_config_proc;
	db_called = 0;
	ck_assert_int_eq(parse_config(str, strlen(str)), 0);
	ck_assert_uint_eq(state.section_len, 4);
	ck_assert_mem_eq(state.section, "test", 4);

	/* Check for the callback */
	ck_assert(state.cb == db_config_proc);
	++db_called;

	/* Now, check the source callback */
	memset(&state, 0, sizeof state);
	ck_assert_int_eq(parse_config(str, strlen(str)), 0);
	ck_assert(state.cb == source_config_proc);
}
END_TEST

/**
 * Test that a key with a value of only a comment is
 * treated like an empty key, and that whitespace
 * between the = and the value is ignored.
 *
 * This also tests that a valid value is parsed.
 */
START_TEST(parse_config_valid_value)
{
	char str[] = "[main]\nkey=;value";
	char c;

	/* Check that a comment */
	*errbuf = '\0';
	main_config_called = 0;
	memset(&state, 0, sizeof state);

	config_init(main_config_proc);
	ck_assert_int_eq(parse_config(str, strlen(str)), 0);
	ck_assert_uint_eq(state.section_len, 4);
	ck_assert_mem_eq(state.section, "main", 4);
	ck_assert_uint_eq(state.key_len, 3);
	ck_assert_mem_eq(state.key, "key", 3);
	ck_assert_uint_eq(state.value_len, 0);
	ck_assert_ptr_null(state.value);
	ck_assert_int_eq(main_config_called, 0);

	/* ... and leading whitespace should be ignored. */
	for (c='\t'; c<'\r' + 2; c++) {
		if (c == '\n') c++;
		if (c > '\r') c = ' ';

		*errbuf = '\0';
		str[11] = c;
		memset(&state, 0, sizeof state);
		main_config_called = 0;

		ck_assert_int_eq(parse_config(str, strlen(str)), 0);
		ck_assert_uint_eq(state.section_len, 4);
		ck_assert_mem_eq(state.section, "main", 4);
		ck_assert_uint_eq(state.key_len, 0);
		ck_assert_ptr_null(state.key);
		ck_assert_uint_eq(state.value_len, 5);
		ck_assert_mem_eq(state.value, str + 12, 5);
		ck_assert_int_eq(main_config_called, 1);
		ck_assert(!*errbuf);
	}
}
END_TEST

/**
 * Test that we get an error for a key followed by a section
 * start token (i.e. no assignment op and thus no value.)
 */
START_TEST(parse_config_section_where_value_expected)
{
	char str[]      = "[main]\nkey[main]";
	const char *err = "config: [Line: 2, Char: 4] Value expected\n";

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	ck_assert_int_eq(parse_config(str, strlen(str)), 1);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that we get an error if the section
 * start token is missing.
 */
START_TEST(parse_config_missing_section_start)
{
	char str[]      = "main]\nkey=value";
	const char *err = "config: [Line: 1, Char: 1] Section expected\n";

check:
	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	ck_assert_int_eq(parse_config(str, strlen(str)), 1);
	ck_assert_uint_eq(state.section_len, 0);
	ck_assert_ptr_null(state.section);
	ck_assert_str_eq(errbuf, err);

	/* End section name without section name */
	switch (*str) {
	case 'm': /* end section name without a section name */
		memmove(str, str + 4, strlen(str) - 3);
		goto check;
	case ']': /* key=value without a section */
		memmove(str, str + 2, strlen(str) - 1);
		goto check;
	}
}
END_TEST

/**
 * Test that we get an error for an assignment operator without
 * a section.
 */
START_TEST(parse_config_assignment_without_section)
{
	char str[]      = "\n=value";
	const char *err = "config: [Line: 2, Char: 1] Section expected\n";

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	ck_assert_int_eq(parse_config(str, strlen(str)), 1);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that we get an error for an assignment operator missing
 * a key, that isn't the first key in the section.
 */
START_TEST(parse_config_assignment_without_key)
{
	char str[]      = "[main]\nkey=value\n=value2";
	const char *err = "config: [Line: 3, Char: 1] Key expected\n";

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	ck_assert_int_eq(parse_config(str, strlen(str)), 1);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/**
 * Test that we get an error for a key followed by a section
 * start token (i.e. no assignment op and thus no value.)
 */
START_TEST(parse_config_assignment_missing_op)
{
	char str[]       = "[main]\n_3y:";
	const char *err  = "config: [Line: 2, Char: 4] Value expected\n";

	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	ck_assert_int_eq(parse_config(str, strlen(str)), 1);
	ck_assert_str_eq(errbuf, err);
}
END_TEST

/* {{{ This is the default config copied from src/config_gen.c */
static const char *default_config_1 =
"[main]\n"
"history=3        ; Number of state transitions to keep.\n"
"source=file      ; Source to get migrations from.\n"
"driver=sqlite3   ; Database driver.\n\n"
";\n"
"; Database connection settings\n"
";\n"
"host=\n"
"port=\n"
"username=\n"
"password=\n"
"db=:memory:\n\n";

static const char *default_config_2 =
";\n"
"; Settings for the 'file' source\n"
";\n"
"[file]\n"
"; Path (relative or absolute) to the migration files.\n"
"migration_path=migrations\n\n"
";\n"
"; Settings for the 'git' source\n"
";\n"
"[git]\n"
"; Path (relative or absolute) to the git repository.\n"
"repo_path=\n"
"; Path relative to the repository where the migration files are.\n"
"migration_path=migrations\n";
/* }}} */

/**
 * Test that the parser parses a default config.
 */
START_TEST(parse_config_default_config)
{
	size_t off, len; char *str;

	/* Create a buffer big enough to hold the config */
	off = strlen(default_config_1);
	len = strlen(default_config_1) + strlen(default_config_2);
	str = malloc(len);
	ck_assert_ptr_nonnull(str);

	/* Concat the config parts */
	memcpy(str, default_config_1, off);
	memcpy(str + off, default_config_2, strlen(default_config_2));

	/* Make sure we can parse it */
	*errbuf = '\0';
	memset(&state, 0, sizeof state);
	ck_assert_int_eq(parse_config(str, len), 0);
	ck_assert(!*errbuf);
	free(str);
}
END_TEST

Suite *config_suite(void)
{
	Suite *s;
	TCase *t;

	s = suite_create("Configuration");
	t = tcase_create("config_init");
	tcase_add_test(t, test_config_init);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("config_set_value");
	tcase_add_test(t, config_set_value_params);
	tcase_add_test(t, config_set_value_wrong_key);
	tcase_add_test(t, config_set_value_string_too_big);
	tcase_add_test(t, config_set_value_string);
	tcase_add_test(t, config_set_value_number_not_numeric);
	tcase_add_test(t, config_set_value_number_not_unsigned);
	tcase_add_test(t, config_set_value_number_out_of_range);
	tcase_add_test(t, config_set_value_number_bad_dest_size);
	tcase_add_test(t, config_set_value_number);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	t = tcase_create("parse_config");
	tcase_add_test(t, parse_config_invalid_params);
	tcase_add_test(t, parse_config_empty_string);
	tcase_add_test(t, parse_config_null_within_string);
	tcase_add_test(t, parse_config_ignores_comments);
	tcase_add_test(t, parse_config_invalid_section);
	tcase_add_test(t, parse_config_main_section_cb);
	tcase_add_test(t, parse_config_non_main_section_cb);
	tcase_add_test(t, parse_config_valid_value);
	tcase_add_test(t, parse_config_section_where_value_expected);
	tcase_add_test(t, parse_config_missing_section_start);
	tcase_add_test(t, parse_config_assignment_without_section);
	tcase_add_test(t, parse_config_assignment_without_key);
	tcase_add_test(t, parse_config_assignment_missing_op);
	tcase_add_test(t, parse_config_default_config);
	tcase_set_timeout(t, 1);
	suite_add_tcase(s, t);

	return s;
}

