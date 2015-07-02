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
#include <CUnit/CUnit.h>
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

static config_callback_t source_get_config_cb(const char * UNUSED(start),
                                              size_t UNUSED(end))
{
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
	CU_ASSERT_PTR_NOT_NULL(state.section);
	CU_ASSERT_PTR_NOT_NULL(state.key);
	CU_ASSERT_PTR_NOT_NULL(state.value);
	CU_ASSERT_NOT_EQUAL(state.section_len, 0);
	CU_ASSERT_NOT_EQUAL(state.key_len, 0);
	CU_ASSERT_NOT_EQUAL(state.value_len, 0);
}

/**
 * Test that config_init() actually sets the main_config
 * address correctly.
 */
static void test_config_init(void)
{
	main_config_called = 0;
	main_config = NULL;
	config_init(main_config_proc);
	CU_ASSERT_EQUAL(main_config, main_config_proc);
}

/**
 * Test that the config parser uses the main callback
 * when it encounters the main section.
 */
static void test_main_section_callback(void)
{
	const char *str = "[main]\n";

	memset(&state, 0, sizeof(state));
	db_called = db_called_for_main = 0;
	main_config = main_config_proc;
	CU_ASSERT_EQUAL(0, parse_config(str, strlen(str)));

	/* Ensure the section was parsed correctly */
	CU_ASSERT_EQUAL(state.section_len, 4);
	CU_ASSERT_FALSE(memcmp(state.section, "main", 4));

	/* Check for the callback */
	CU_ASSERT_EQUAL(state.cb, main_config_proc);
	CU_ASSERT_FALSE(db_called_for_main);

	/* If no main callback is set, check that db gets called. */
	memset(&state, 0 , sizeof(state));
	main_config = NULL;
	db_called = db_called_for_main = 0;
	CU_ASSERT_EQUAL(0, parse_config(str, strlen(str)));

	/* Ensure the section was parsed correctly */
	CU_ASSERT_EQUAL(state.section_len, 4);
	CU_ASSERT_FALSE(memcmp(state.section, "main", 4));

	/* Check for the callback */
	CU_ASSERT_EQUAL(state.cb, db_config_proc);
	CU_ASSERT_TRUE(db_called_for_main);
}

/**
 * Test that the config parser first calls the db
 * lookup function, and then the source lookup
 * function when it encounters a section other
 * than main.
 */
static void test_non_main_callbacks(void)
{
	const char *str = "[test]\n";

	memset(&state, 0, sizeof(state));
	main_config = main_config_proc;
	db_called = 0;
	CU_ASSERT_EQUAL(0, parse_config(str, strlen(str)));

	/* Ensure the section was parsed correctly */
	CU_ASSERT_EQUAL(state.section_len, 4);
	CU_ASSERT_FALSE(memcmp(state.section, "test", 4));

	/* Check for the callback */
	CU_ASSERT_EQUAL(state.cb, db_config_proc);
	++db_called;

	/* Now, check the source callback */
	memset(&state, 0, sizeof(state));
	parse_config(str, strlen(str));
	CU_ASSERT_EQUAL(state.cb, source_config_proc);
}

/**
 * Test that config_set_value() returns 0 if passed invalid
 * parameters, and that the supplied dest buffer is not
 * modified.
 */
static void config_set_value_params(void)
{
	int i;
	size_t dest_len;

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "value";
	state.value_len = 5;
	dest_len = state.key_len + 1;

	/* !key_len */
	i = config_set_value(0, errbuf, dest_len, "test", 0);
	CU_ASSERT_EQUAL(i, 1);

	/* state.key_len (4) != key_len (3)  */
	i = config_set_value(0, errbuf, dest_len, "test", 3);
	CU_ASSERT_EQUAL(i, 1);

	/* !dest */
	i = config_set_value(0, NULL, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* !expected_key */
	i = config_set_value(0, errbuf, dest_len, NULL, 4);
	CU_ASSERT_EQUAL(i, 1);

	/* !dest_size */
	i = config_set_value(0, errbuf, 0, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* !state.value_len */
	state.value_len = 0;
	i = config_set_value(0, errbuf, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* !state.key_len */
	state.value_len = 5;
	state.key_len   = 0;
	i = config_set_value(0, errbuf, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* !state.key */
	state.key_len = 4;
	state.key = NULL;
	i = config_set_value(0, errbuf, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* !state.value */
	state.key_len = 4;
	state.key     = state.value;
	state.value   = NULL;
	i = config_set_value(0, errbuf, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* Ensure nothing wrote to errbuf */
	CU_ASSERT_EQUAL(errbuf[0], '\0');
}

/**
 * Test that config_set_value() returns 0 if the
 * expected_key and state.key have the same length
 * but different content.
 */
static void config_set_value_wrong_key(void)
{
	int i;
	size_t dest_len;

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "value";
	state.value_len   = 5;
	dest_len = state.key_len + 1;

	/* "xxx" != "test" */
	i = config_set_value(CONFIG_STRING, errbuf, dest_len, "xxxx", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* Ensure nothing wrote to errbuf */
	CU_ASSERT_EQUAL(errbuf[0], '\0');
}

/**
 * Test that the correct error message is generated if dest is too
 * small to hold the contents of state.value.
 */
static void set_value_string_value_too_big(void)
{
	int i;
	size_t dest_len;
	const char *err = "set_value_string: config: section.test: value "
	                  "too long\n";

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "value";
	state.value_len   = 5;
	dest_len = state.value_len - 1;

	i = config_set_value(CONFIG_STRING, errbuf, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* Ensure the expected error message was generated. */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that set_value_string() correctly copies state.value
 * to the destination.
 */
static void test_set_value_string(void)
{
	int i;
	size_t dest_len;
	const char *value = "value";

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = value;
	state.value_len   = strlen(value);
	dest_len = state.value_len + 1;

	/* strlen(value) == strlen(dest) */
	i = config_set_value(CONFIG_STRING, errbuf, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 0);

	/* Ensure value was copied. */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(value));
	CU_ASSERT_STRING_EQUAL(errbuf, value);

	/* strlen(dest) > strlen(value) */
	memset(errbuf, 0, dest_len + 5);
	dest_len <<= 1;
	i = config_set_value(CONFIG_STRING, errbuf, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 0);

	/* Ensure the value was copied. */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(value));
	CU_ASSERT_STRING_EQUAL(errbuf, value);
}

/**
 * Test that the correct error message is generated if value
 * doesn't represent a number.
 */
static void test_set_value_number_not_numeric(void)
{
	int i;
	size_t dest_len;
	const char *err = "set_value_number: config: section.test: number "
	                  "expected\n";

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "value";
	state.value_len   = 5;
	dest_len = state.value_len;

	i = config_set_value(CONFIG_NUMBER, errbuf, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* Ensure the expected error message was generated. */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that the correct error message is generated if value
 * represents a number that is negative, or that cannot be
 * parsed as a base 10 integer by strtoul().
 */
static void set_value_number_not_unsigned(void)
{
	int i;
	size_t dest_len;
	const char *err = "set_value_number: config: section.test: "
	                  "number expected\n";

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "-1234";
	state.value_len   = 5;
	dest_len = sizeof(unsigned long);

	/* Check a negative number */
	i = config_set_value(CONFIG_NUMBER, errbuf, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* Ensure the expected error message was generated. */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);

	/* Check a non-base-10 number too */
	errbuf[0] = '\0';
	state.value = "0x234";
	i = config_set_value(CONFIG_NUMBER, errbuf, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* Ensure the expected error message was generated. */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that the correct error message is generated if value
 * represents a number that is negative, or that cannot be
 * parsed as a base 10 integer by strtoul().
 */
static void set_value_number_out_of_range(void)
{
	int i;
	size_t dest_len;
	char value[255];

	errbuf[0] = '\0';
	sprintf(value, "%lu1\n", ULONG_MAX);
	memset(&state, 0, sizeof(state));
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = value;
	state.value_len   = strlen(value);
	dest_len = sizeof(unsigned long);

	/* The number should be larger than ULONG_MAX. */
	i = config_set_value(CONFIG_NUMBER, errbuf, dest_len, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* Ensure the expected error message was generated. */
	sprintf(value, "set_value_number: config: section.test: %s\n",
	        strerror(ERANGE));
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(value));
	CU_ASSERT_STRING_EQUAL(errbuf, value);
}

/**
 * Test that the correct error message is generated if the
 * size of dest is larger than sizeof(unsigned long).
 */
static void set_value_number_bad_dest_size(void)
{
	int i;
	const char *err = "set_value_number: config: section.test: bad "
	                  "dest size\n";

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "12345";
	state.value_len   = 5;

	/* > sizeof(unsigned long) */
	i = sizeof(unsigned long) << 1;
	i = config_set_value(CONFIG_NUMBER, errbuf, (size_t)i, "test", 4);
	CU_ASSERT_EQUAL(i, 1);

	/* Ensure the expected error message was generated. */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that set_value_number() works correctly.
 */
static void test_set_value_number(void)
{
	int i;
	unsigned long value;
	unsigned int  ivalue;
	unsigned short svalue;
	size_t stval;

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	state.section     = "section";
	state.section_len = 7;
	state.key_len     = 4;
	state.key         = "test";
	state.value       = "12345";
	state.value_len   = 5;

	/* == sizeof(char) */
	i = config_set_value(CONFIG_NUMBER, errbuf, sizeof(char),
	                     "test", 4);
	CU_ASSERT_EQUAL(i, 0);
	CU_ASSERT_EQUAL(errbuf[0], (char)12345);

	/* == sizeof(unsigned int) */
	i = config_set_value(CONFIG_NUMBER, errbuf, sizeof(ivalue),
	                     "test", 4);
	CU_ASSERT_EQUAL(i, 0);
	memcpy(&ivalue, errbuf, sizeof(unsigned int));
	CU_ASSERT_EQUAL(ivalue, 12345);

	/* == sizeof(unsigned short) */
	i = config_set_value(CONFIG_NUMBER, errbuf, sizeof(svalue),
	                     "test", 4);
	CU_ASSERT_EQUAL(i, 0);
	memcpy(&svalue, errbuf, sizeof(unsigned short));
	CU_ASSERT_EQUAL(svalue, 12345);

	/* == sizeof(size_t) */
	i = config_set_value(CONFIG_NUMBER, errbuf, sizeof(stval),
	                     "test", 4);
	CU_ASSERT_EQUAL(i, 0);
	memcpy(&stval, errbuf, sizeof(size_t));
	CU_ASSERT_EQUAL(stval, 12345);

	/* == sizeof(unsigned long) */
	i = config_set_value(CONFIG_NUMBER, errbuf, sizeof(value),
	                     "test", 4);
	CU_ASSERT_EQUAL(i, 0);
	memcpy(&value, errbuf, sizeof(unsigned long));
	CU_ASSERT_EQUAL(value, 12345);
}

/**
 * Ensure that handle_token() returns a non-zero
 * value if the state's token type is invalid.
 */
static void handle_token_invalid_token_type(void)
{
	size_t s = 0;
	const char *err = "handle_token: invalid config token type 4\n";
	state.token = 4;
	errbuf[0] = '\0';
	CU_ASSERT_EQUAL(1, handle_token("test=", 5, &s));
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that parse_config() returns 1 when given
 * invalid parameters.
 */
static void parse_config_invalid_params(void)
{
	const char *str = "xxx";
	CU_ASSERT_EQUAL(1, parse_config(NULL, 1));
	CU_ASSERT_EQUAL(1, parse_config(str, 0));
}

/**
 * Test that parse_config() returns 0 when given
 * an empty string.
 */
static void parse_config_parses_empty_string(void)
{
	const char *str = "";
	CU_ASSERT_EQUAL(0, parse_config(str, 1));
}

/**
 * Test that parse_config() returns 0 when it
 * encounters a null byte within a string.
 */
static void parse_config_null_within_string(void)
{
	const char *str = "\n\0\n";
	CU_ASSERT_EQUAL(0, parse_config(str, 3));
}

/**
 * Test that parse_config() ignores lines
 * beginning with a comment.
 */
static void parse_config_ignores_comments(void)
{
	const char *str = "; Comment\n";
	CU_ASSERT_EQUAL(0, parse_config(str, strlen(str)));
}

/**
 * Test that parse_config() fails on an invalid
 * identifier for a section name (i.e. not [a-z0-9_]).
 */
static void parse_config_invalid_section(void)
{
	const char *err =
	"parse_config: config: [Line: 1, Char: 2] Invalid Section\n";
	char str[] = "[Atest]\n";
	int c;

	/* Check anything not [a-z0-9_], whitespace, or a comment. */
	memset(&state, 0, sizeof(state));
	for (c = 1; c <= 0xff; c++) {
		if (isspace(c) || isdigit(c) || islower(c))
			continue;
		if (c == '_' || c == ';')
			continue;

		str[1] = (char)c;
		state.section = NULL;
		state.section_len = 0;

		CU_ASSERT_EQUAL(1, parse_config(str, strlen(str)));
		CU_ASSERT_PTR_NULL(state.section);
		CU_ASSERT_EQUAL(state.section_len, 0);

		/* Ensure the expected error message was generated. */
		CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
		CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
		CU_ASSERT_STRING_EQUAL(errbuf, err);
	}

	/* Check if the name would run past the end of the string */
	str[1] = 'a';
	str[6] = 'x';
	str[7] = 'z';

	CU_ASSERT_EQUAL(1, parse_config(str, strlen(str)));
	CU_ASSERT_PTR_NULL(state.section);
	CU_ASSERT_EQUAL(state.section_len, 0);

	/* Ensure the expected error message was generated. */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Ensure that keys get correctly parsed, and that
 * keys with empty values are ignored (NULL).
 */
static void handle_token_ignores_empty_keys(void)
{
	const char *data = "[main]\nkey=\n";
	CU_ASSERT_EQUAL(0, parse_config(data, strlen(data)));
	CU_ASSERT_EQUAL(state.key_len, 3);
	CU_ASSERT_PTR_NULL(state.key);
	CU_ASSERT_EQUAL(state.token, 0);
	CU_ASSERT_FALSE(main_config_called);
}

/**
 * Test that a key with a value of only a comment is
 * treated like an empty key, and that whitespace
 * between the = and the value is ignored.
 *
 * This also tests that a valid value is parsed.
 */
static void parse_config_valid_value(void)
{
	char str[] = "[main]\nkey=;value";
	char c;

	/* Check that a comment */
	errbuf[0] = '\0';
	main_config_called = 0;
	memset(&state, 0, sizeof(state));
	CU_ASSERT_EQUAL(0, parse_config(str, strlen(str)));
	CU_ASSERT_EQUAL(state.section, str+1);
	CU_ASSERT_EQUAL(state.section_len, 4);
	CU_ASSERT_EQUAL(state.key, str+7);
	CU_ASSERT_EQUAL(state.key_len, 3);
	CU_ASSERT_PTR_NULL(state.value);
	CU_ASSERT_EQUAL(state.value_len, 0);
	CU_ASSERT_EQUAL(main_config_called, 0);

	/* ... and leading whitespace should be ignored. */
	for (c='\t';c<'\r' + 2;c++) {
		if (c == '\n') c++;
		if (c > '\r') c = ' ';

		str[11] = c;
		errbuf[0] = '\0';
		memset(&state, 0, sizeof(state));
		main_config_called = 0;
		CU_ASSERT_EQUAL(0, parse_config(str, strlen(str)));
		CU_ASSERT_EQUAL(state.section, str+1);
		CU_ASSERT_EQUAL(state.section_len, 4);
		CU_ASSERT_EQUAL(state.key_len, 0);
		CU_ASSERT_PTR_NULL(state.key);
		CU_ASSERT_EQUAL(state.value, str+12);
		CU_ASSERT_EQUAL(state.value_len, 5);
		CU_ASSERT_EQUAL(main_config_called, 1);

		/* No error message should be generated. */
		CU_ASSERT_EQUAL_FATAL(errbuf[0], '\0');
	}
}

/**
 * Test that we get an error for a key followed by a section
 * start token (i.e. no assignment op and thus no value.)
 */
static void parse_config_section_where_value_expected(void)
{
	char str[] = "[main]\nkey[main]";
	char *err  =
	"parse_config: config: [Line: 2, Char: 4] Value expected\n";

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	CU_ASSERT_EQUAL(1, parse_config(str, strlen(str)));

	/* Check for the error message */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that we get an error if the section
 * start token is missing.
 */
static void parse_config_missing_section_start(void)
{
	char str[] = "main]\nkey=value";
	char *err  =
	"parse_config: config: [Line: 1, Char: 1] Section expected\n";

check:
	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	CU_ASSERT_EQUAL(1, parse_config(str, strlen(str)));
	CU_ASSERT_PTR_NULL(state.section);
	CU_ASSERT_EQUAL(state.section_len, 0);

	/* Check for the error message */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);

	/* End section name without section name */
	if (!memcmp(str, "main]", 5)) {
		memmove(str, &str[4], strlen(str) - 3);
		goto check;
	}

	/* key=value without section */
	if (str[0] == ']') {
		memmove(str, &str[2], strlen(str) - 1);
		goto check;
	}
}

/**
 * Test that we get an error for an assignment operator without
 * a section.
 */
static void parse_config_assignment_without_section(void)
{
	char str[] = "\n=value";
	char *err  =
	"parse_config: config: [Line: 2, Char: 1] Section expected\n";

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	CU_ASSERT_EQUAL(1, parse_config(str, strlen(str)));

	/* Check for the error message */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that we get an error for an assignment operator missing
 * a key, that isn't the first key in the section.
 */
static void parse_config_assignment_without_key(void)
{
	char str[] = "[main]\nkey=value\n=value2";
	char *err  =
	"parse_config: config: [Line: 3, Char: 1] Key expected\n";

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	CU_ASSERT_EQUAL(1, parse_config(str, strlen(str)));

	/* Check for the error message */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

/**
 * Test that we get an error for a key followed by a section
 * start token (i.e. no assignment op and thus no value.)
 */
static void parse_config_key_without_assignment_op(void)
{
	char str[] = "[main]\n_3y:";
	char *err  =
	"parse_config: config: [Line: 2, Char: 4] Value expected\n";

	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	CU_ASSERT_EQUAL(1, parse_config(str, strlen(str)));

	/* Check for the error message */
	CU_ASSERT_NOT_EQUAL_FATAL(errbuf[0], '\0');
	CU_ASSERT_EQUAL_FATAL(strlen(errbuf), strlen(err));
	CU_ASSERT_STRING_EQUAL(errbuf, err);
}

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
static void parses_default_config(void)
{
	size_t off, len; char *str;

	/* Create a buffer big enough to hold the config */
	off = strlen(default_config_1);
	len = strlen(default_config_1) + strlen(default_config_2);
	str = malloc(len);
	CU_ASSERT_PTR_NOT_NULL_FATAL(str);

	/* Concat the config parts */
	memcpy(str, default_config_1, off);
	memcpy(str + off, default_config_2, strlen(default_config_2));

	/* Make sure we can parse it */
	errbuf[0] = '\0';
	memset(&state, 0, sizeof(state));
	CU_ASSERT_EQUAL(0, parse_config(str, len));
	free(str);

	/* Check for the error message */
	CU_ASSERT_EQUAL_FATAL(errbuf[0], '\0');
}

static CU_TestInfo config_tests[] = {
	{
		"config_init works",
		test_config_init
	},
	{
		"config_set_value - invalid params",
		config_set_value_params
	},
	{
		"config_set_value - wrong key",
		config_set_value_wrong_key
	},
	{
		"set_value_string - value too big",
		set_value_string_value_too_big
	},
	{
		"set_value_string works",
		test_set_value_string
	},
	{
		"set_value_number - value not numeric",
		test_set_value_number_not_numeric
	},
	{
		"set_value_number - value not unsigned",
		set_value_number_not_unsigned
	},
	{
		"set_value_number - value out of range",
		set_value_number_out_of_range
	},
	{
		"set_value_number - bad storage size",
		set_value_number_bad_dest_size
	},
	{
		"set_value_number works",
		test_set_value_number
	},
	{
		"handle_token - invalid token type",
		handle_token_invalid_token_type
	},
	{
		"parse_config - invalid params",
		parse_config_invalid_params
	},
	{
		"parsing empty string",
		parse_config_parses_empty_string
	},
	{
		"null within string",
		parse_config_null_within_string
	},
	{
		"ignores comments",
		parse_config_ignores_comments
	},
	{ /* XXX: expected to be slow */
		"invalid section name",
		parse_config_invalid_section
	},
	{
		"main section callback",
		test_main_section_callback
	},
	{
		"non-main callbacks",
		test_non_main_callbacks
	},
	{
		"ignores empty keys",
		handle_token_ignores_empty_keys
	},
	{
		"parses valid values",
		parse_config_valid_value
	},
	{
		"error if section where value expected",
		parse_config_section_where_value_expected
	},
	{
		"error if missing section start token",
		parse_config_missing_section_start
	},
	{
		"error on assignment without section",
		parse_config_assignment_without_section
	},
	{
		"error on assignment without key",
		parse_config_assignment_without_key
	},
	{
		"error on key without assignment op",
		parse_config_key_without_assignment_op
	},
	{
		"parses default config",
		parses_default_config
	},

	CU_TEST_INFO_NULL
};

void config_add_suite(void)
{
	size_t i = 0;
	CU_pSuite suite;

	suite = CU_add_suite("Config Parser", NULL, NULL);
	while (config_tests[i].pName) {
		CU_add_test(suite, config_tests[i].pName,
		            config_tests[i].pTestFunc);
		i++;
	}
}

