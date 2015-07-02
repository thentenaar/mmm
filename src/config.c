/**
 * Minimal Migration Manager - Configuration Parser
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <errno.h>
#include <ctype.h>

#include "config.h"
#include "utils.h"

#ifndef IN_TESTS
#include "db.h"
#include "source.h"
#endif

/**
 * The configuration file should be structured like a typical
 * .ini-style file. With sections containing key-value pairs
 * like so:
 *
 * ; This is a comment
 * [section]
 * key = value
 * key2 = value2
 * key3 = value3 ; This is also a comment
 *
 * Strings are strings, and numbers are interpreted as unsigned.
 */

/**
 * Token identifiers.
 */
#define SECTION 1
#define KEY     2
#define VALUE   3

static const char *token_to_string[4] = {
	"Unknown",
	"Section",
	"Key",
	"Value"
};

/* Main configuration routine */
static config_callback_t main_config = NULL;

/**
 * Configuration parser state.
 */
static struct parse_state {
	unsigned int token;   /**< Next expected token */
	const char *section;  /**< Start of current section identifier */
	const char *key;      /**< Start of current key identifier */
	const char *value;    /**< Start of current value */
	size_t section_len;   /**< Section identifier length */
	size_t key_len;       /**< Key identifier length */
	size_t value_len;     /**< Value length */
	config_callback_t cb; /**< Section config callback */
} state;

/**
 * Identifiers are [a-z0-9_].
 *
 * \return index of the end of the identifier, len + 1 on error.
 */
static size_t read_identifier(const char *s, size_t pos, size_t len)
{
	while (islower(s[pos]) || isdigit(s[pos]) || s[pos] == '_') {
		if (++pos >= len) {
			pos = len + 1;
			break;
		}
	}

	return pos;
}

/**
 * Values are everything except comments and whitespace.
 *
 * \return index of the end of the value, up to \a len.
 */
static size_t read_value(const char *s, size_t pos, size_t len)
{
	while (s[pos] != ';' && !isspace(s[pos])) {
		if (++pos >= len) {
			pos = len;
			break;
		}
	}

	return pos;
}

/**
 * Interpret a token from the input.
 *
 * \param[in] start Starting byte of token.
 * \param[in] len   Number of bytes remaining in the data stream.
 * \param[in] pos   Current position within the data stream.
 * \return 0 on success, non-zero on error.
 */
static int handle_token(const char *start, size_t len, size_t *pos)
{
	size_t end;
	int retval = 0;

	/* Read the identifier / value */
	if (state.token == VALUE)
		end = read_value(start, 0, len);
	else end = read_identifier(start, 0, len);

	/* Make sure we have a valid identifier / value */
	if (!end || end > len)
		goto err;

	/* Update the parser state. */
	switch (state.token) {
	case SECTION:
		state.section = start;
		state.section_len = end;

		/* Is this for the 'main' section? */
		state.cb = NULL;
		if (end == 4 && !memcmp("main", start, 4)) {
			if (main_config)
				state.cb = main_config;
		}

		/* Pass it on through to the db/source layers */
		if (!state.cb)
			state.cb = db_get_config_cb(start, end);
		if (!state.cb)
			state.cb = source_get_config_cb(start, end);
		break;
	case KEY:
		state.key = start;
		state.key_len = end;
		break;
	case VALUE:
		state.value = start;
		state.value_len = end;

		/* Invoke the callback for this key-value pair. */
		if (state.cb) state.cb();

		state.key = NULL;
		state.key_len = 0;
		break;
	default:
		ERROR_1("invalid config token type %d", state.token);
		++retval;
		break;
	}

	state.token = 0;
	*pos += end;

ret:
	return retval;

err:
	++retval;
	goto ret;
}

/**
 * Analyze the next token on the line, perform
 * syntax checking, and mutate the parsing state.
 *
 * \param[in]     config Config data
 * \param[in,out] xpos   Current position within \a config.
 * \return 0 on success, non-zero on error.
 */
static int lex_line(const char *config, size_t *xpos)
{
	int retval = 0;
	size_t pos = *xpos;

	switch (config[pos]) {
	case '[': /* Start of a section name */
		if (state.key)
			goto value_expected;
		state.token = SECTION;
		pos++;
		break;
	case ']': /* End of section name */
		if (!state.section)
			goto section_expected;
		state.token = KEY;
		pos++;
		break;
	case '=': /* Assignment of a value to a key */
		if (!state.section)
			goto section_expected;

		if (!state.key)
			goto key_expected;

		/* Ignore empty keys */
		if (config[++pos] == '\n') {
			state.key = NULL;
			state.token = 0;
		} else state.token = VALUE;
		break;
	default:
		/* Values should follow KEYs */
		if (state.key && state.token != VALUE)
			goto value_expected;

		/* And above those should be a SECTION */
		if (!state.section)
			goto section_expected;

		/* Transition from VALUE to KEY */
		if (!isspace(config[pos]) && state.value && !state.key)
			state.token = KEY;
		break;
	}

ret:
	*xpos = pos;
	return retval;

err:
	++retval;
	goto ret;

section_expected:
	state.token = SECTION;
	goto err;

key_expected:
	state.token = KEY;
	goto err;

value_expected:
	state.token = VALUE;
	goto err;
}

/**
 * Parse the config, line-by-line.
 *
 * This approach doesn't modify config or duplicate
 * any part of it on the heap. The idea is to scan
 * the config file once, and then let the callback
 * decide what to do with it.
 *
 * \param[in] config Configuration file contents.
 * \param[in] len    Length of config.
 * \return 0 on success, non-zero on error.
 */
int parse_config(const char *config, size_t len)
{
	size_t pos = 0, line = 1, lpos = 0;
	int retval = 0;

	if (!config || !len)
		goto err;

	/* Clear the parse state */
	memset(&state, 0, sizeof(struct parse_state));
	do {
		/* Skip leading whitespace, and count lines. */
		while (pos < len && isspace(config[pos])) {
			if (config[pos] == '\n') {
				line++;
				lpos = pos + 1;
			}
			pos++;
		}

		/* Skip comments. */
		if (pos < len && config[pos] == ';') {
			while (pos < len && config[pos++] != '\n');
			continue;
		}

		/* Make sure we don't run past the end of config. */
		if (pos >= len || !config[pos])
			break;

		/* If we have a token, handle it. */
		if (state.token) {
			retval = handle_token(config + pos, len - pos, &pos);
			if (retval) goto invalid_identifier;
		}

		/* Lex the line for tokens. */
		retval = lex_line(config, &pos);
		if (retval) goto token_expected;
	} while (pos < len);

ret:
	return retval;

err:
	++retval;
	goto ret;

invalid_identifier:
	ERROR_3("config: [Line: %lu, Char: %lu] Invalid %s",
	        line, pos - lpos + 1, token_to_string[state.token & 3]);
	goto ret;

token_expected:
	ERROR_3("config: [Line: %lu, Char: %lu] %s expected",
	        line, pos - lpos + 1, token_to_string[state.token & 3]);
	goto ret;
}

/**
 * Set the main configuration callback.
 *
 * \param[in] main_config_cb Main config. callback
 */
void config_init(config_callback_t main_config_cb)
{
	main_config = main_config_cb;
}

/**
 * Copy a string value to \a dest, with length checking.
 *
 * \param[in] dest      Destination buffer
 * \param[in] dest_size Length of \a dest in bytes.
 * \return 0 on success, non-zero on error.
 */
static int set_value_string(void *dest, size_t dest_size)
{
	int retval = 0;

	if (state.value_len >= dest_size) {
		ERROR_4("config: %.*s.%.*s: value too long",
		        (int)state.section_len, state.section,
		        (int)state.key_len, state.key);
		++retval;
		goto ret;
	}

	memcpy(dest, state.value, state.value_len);
	*((char *)dest + state.value_len) = '\0';

ret:
	return retval;
}

/**
 * Parse an integer value and store it in \a dest.
 *
 * \param[in] dest      Destination memory area
 * \param[in] dest_size Length of \a dest in bytes.
 * \return 0 on success, non-zero on error.
 */
static int set_value_number(void *dest, size_t dest_size)
{
	size_t parsed_len = 0;
	unsigned long result;
	char *endptr = NULL;
	int retval = 0;

	/* Ensure the destination is large enough */
	if (!dest_size || dest_size > sizeof(unsigned long))
		goto bad_dest_size;

	/* At least make sure we have a digit */
	if (!isdigit(state.value[0]))
		goto number_expected;

	/* Get the value */
	errno = 0;
	result = strtoul(state.value, &endptr, 10);

	/* Check for the number being out of range. */
	if (result == ULONG_MAX && errno == ERANGE)
		goto result_out_of_range;

	/* Only update parsed_len if a conversion was performed. */
	if (endptr && errno != EINVAL)
		parsed_len = (size_t)(endptr - state.value);

	/* Ensure we got a number */
	if (parsed_len < state.value_len)
		goto number_expected;

	/* Store the value */
	memcpy(dest, &result, dest_size);

ret:
	return retval;

bad_dest_size:
	ERROR_4("config: %.*s.%.*s: bad dest size",
	        (int)state.section_len, state.section,
	        (int)state.key_len, state.key);
err:
	++retval;
	goto ret;

number_expected:
	ERROR_4("config: %.*s.%.*s: number expected",
	        (int)state.section_len, state.section,
	        (int)state.key_len, state.key);
	goto err;

result_out_of_range:
	ERROR_5("config: %.*s.%.*s: %s",
	        (int)state.section_len, state.section,
	        (int)state.key_len, state.key, strerror(ERANGE));
	goto err;
}

/**
 * Convenience function for setting a value from the config.
 *
 * If key matches expected_key:
 *
 * strings are copied to dest if they fit.
 * numbers are interpreted as unsigned integers and
 * copied into dest, according to dest_size.
 *
 * \param[in] type         Expected type of the value.
 * \param[in] dest         Destination memory area.
 * \param[in] dest_size    Size of the destination memory area.
 * \param[in] expected_key If key matches this, the value is set.
 * \param[in] key_len      Length of \a expected_key.
 * \return 0 on success, non-zero on error.
 */
int config_set_value(int type, void *dest, size_t dest_size,
                     const char *expected_key, size_t key_len)
{
	int retval = 0;

	/* Check the lengths / key-value pointers first */
	if (!key_len || !state.key_len || !state.key || !state.value)
		goto err;

	/* Now compare the key lengths */
	if (state.key_len != key_len)
		goto err;

	/* Validate the remaining params. */
	if (!dest || !expected_key || !dest_size || !state.value_len)
		goto err;

	/* Make sure the we've got the right key. */
	if (memcmp(expected_key, state.key, key_len))
		goto err;

	if (type == CONFIG_NUMBER)
		retval = set_value_number(dest, dest_size);
	else retval = set_value_string(dest, dest_size);

ret:
	return retval;

err:
	++retval;
	goto ret;
}
