/**
 * Minimal Migration Manager - Common String Buffer
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>

#include "stringbuf.h"

/* Size of the buffer */
#define SBUFSIZ 2048

/* Largest exponent of 10 for an unsigned long */
#define EXP10 ((sizeof(unsigned long) << 1) +\
               (sizeof(unsigned long) >> 1))

static char sbuf[SBUFSIZ];

/* Current offset into the string-side of the buffer */
static size_t offset = 0;

/**
 * Get the log10 of a number as an integer.
 *
 * \param[in] num Number
 * \return log10(num) as an integer
 */
static size_t ilog10(unsigned long num)
{
	size_t i;
	unsigned long p = 1;

	for (i = 0; i < EXP10; i++) {
		if (num < p)
			break;
		p *= 10;
	}

	return !i ? 1 : i;
}

/**
 * Add pre-string formatting
 */
static void sbuf_add_formatting_pre(long flags)
{
	/* Add a space or quote */
	if (offset && (flags & SBUF_LSPACE))
		sbuf[offset++] = ' ';
	if (flags & SBUF_LPAREN) sbuf[offset++] = '(';
	if (flags & SBUF_QUOTE) sbuf[offset++] = '\'';
}

/**
 * Add post-string formatting.
 */
static void sbuf_add_formatting_post(long flags)
{
	/* Add a quote, comma, etc. */
	if (flags & SBUF_QUOTE) sbuf[offset++] = '\'';
	if (flags & SBUF_COMMA) sbuf[offset++] = ',';
	if (flags & SBUF_EQUALS) sbuf[offset++] = '=';
	if (flags & SBUF_RPAREN) sbuf[offset++] = ')';
	if (flags & SBUF_SCOLON) sbuf[offset++] = ';';
	if (flags & SBUF_TSPACE) sbuf[offset++] = ' ';
	sbuf[offset] = '\0';
}

/**
 * Reset the buffer
 *
 * \param[in] scrub If non-zero, scrub the buffer.
 */
void sbuf_reset(int scrub)
{
	if (scrub)
		memset(sbuf, 0, offset ? offset : SBUFSIZ);
	offset = 0;
	*sbuf = '\0';
}

/**
 * Get a const-qualified pointer to the buffer.
 */
const char *sbuf_get_buffer(void)
{
	return sbuf;
}

/**
 * Add a string to the buffer.
 *
 * \param[in] s     String to add
 * \param[in] flags Formatting flags
 * \param[in] pos   Position at which to add the string (0 = current)
 * \return 0 on success, 1 on failure.
 */
int sbuf_add_str(const char *s, long flags, size_t pos)
{
	size_t s_len = 0;
	int retval = 0;

	if (!s) goto err;
	s_len = strlen(s);

	/* With an empty string, and no formatting, just return. */
	if (!s_len && !flags)
		goto ret;

	/* Make sure we have enough room in the buffer */
	if (!pos) pos = offset;
	if (pos + s_len + 6 >= SBUFSIZ)
		goto err;

	/* Copy the string */
	offset = pos;
	sbuf_add_formatting_pre(flags);
	if (s_len) {
		memcpy(sbuf + offset, s, s_len);
		offset += s_len;
	}
	sbuf_add_formatting_post(flags);

ret:
	return retval;

err:
	++retval;
	goto ret;
}

/**
 * Add an unsigned integer to the buffer.
 *
 * \param[in] num Number to add
 * \param[in] flags Formatting flags
 * \return 0 on success, 1 on failure.
 */
int sbuf_add_unum(unsigned long num, long flags)
{
	size_t i, j;
	int retval = 0;

	/* Ensure this number will fit */
	i = ilog10(num);
	if (offset + i >= SBUFSIZ - 1) {
		++retval;
		goto ret;
	}

	/* Convert num to a string of base 10 numbers. */
	sbuf_add_formatting_pre(flags);
	j = offset + i;
	sbuf[j] = '\0';
	do {
		sbuf[offset + --i] = (char)((num % 10) + '0');
		num /= 10;
	} while (i > 0 && num > 0);
	offset = j;
	sbuf_add_formatting_post(flags);

ret:
	return retval;
}

/**
 * Add a signed integer to the buffer.
 *
 * \param[in] num   Number to add
 * \param[in] flags Formatting flags
 * \return 0 on success, 1 on failure.
 */
int sbuf_add_snum(long num, long flags)
{
	unsigned long s, n;
	int retval = 0;

	/* Get the sign bit of num, and extend it throughout s. */
	n = (unsigned long)num;
	s = -(n >> ((sizeof(unsigned long) << 3) - 1));

	/* If s is negative, add a '-' to the buffer */
	if (s) {
		if (offset + 3 >= SBUFSIZ) {
			++retval;
			goto ret;
		}

		sbuf[offset++] = '-';
	}

	/* Add the absolute value of num to the buffer */
	retval = sbuf_add_unum((s ^ n) - s, flags);

ret:
	return retval;
}

/**
 * Add a parameter string to the string buffer.
 *
 * This will add:
 *     param='value'
 *
 * to the buffer, adding a leading space between parameters.
 *
 * \param[in] param  Parameter name
 * \param[in] value  Value to set
 * \return 0 on success, 1 on error.
 */
int sbuf_add_param_str(const char *param, const char *value)
{
	size_t p_len = 0, v_len = 0;
	int retval = 0;

	if (!param || !value)
		goto err;

	/* See if we have enough room in the buffer */
	p_len = strlen(param);
	v_len = strlen(value);

	if (!v_len || offset + p_len + v_len + 5 > SBUFSIZ)
		goto err;

	/* Copy the param and value */
	if (sbuf_add_str(param, SBUF_LSPACE | SBUF_EQUALS, 0))
		goto err;

	if (sbuf_add_str(value, SBUF_QUOTE, 0))
		++retval;

ret:
	return retval;

err:
	++retval;
	goto ret;
}

/**
 * Add a numeric parameter to the string buffer.
 *
 * This will add:
 *     param=value
 *
 * to the buffer, adding a leading space between parameters.
 *
 * \param[in] param  Parameter name
 * \param[in] value  Numeric value
 * \return 0 on success, 1 on failure.
 */
int sbuf_add_param_num(const char *param, unsigned long value)
{
	int retval = 0;

	/* Copy the param and value */
	if (sbuf_add_str(param, SBUF_LSPACE | SBUF_EQUALS, 0))
		++retval;

	if (!retval && sbuf_add_unum(value, 0))
		++retval;

	return retval;
}
