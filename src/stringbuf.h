/**
 * \file stringbuf.h
 *
 * Minimal Migration Manager - Common String Buffer
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef STRINGBUF_H
#define STRINGBUF_H

/**
 * \defgroup cbff SBUF Formatting Flags
 * @{
 */

/**
 * \def SBUF_LSPACE
 *
 * Add a leading space before the string.
 */
#define SBUF_LSPACE (1 << 0)

/**
 * \def SBUF_TSPACE
 *
 * Add a trailing space after the string.
 */
#define SBUF_TSPACE (1 << 1)

/**
 * \def SBUF_EQUALS
 *
 * Add an '=' after the string.
 */
#define SBUF_EQUALS (1 << 2)

/**
 * \def SBUF_QUOTE
 *
 * Add a single-quote before and after the string.
 */
#define SBUF_QUOTE (1 << 3)

/**
 * \def SBUF_COMMA
 *
 * Add a comma after the string.
 */
#define SBUF_COMMA (1 << 4)

/**
 * \def SBUF_LPAREN
 *
 * Add a left-parenthesis before the string.
 */
#define SBUF_LPAREN (1 << 5)

/**
 * \def SBUF_RPAREN
 *
 * Add a right-parenthesis after the string.
 */
#define SBUF_RPAREN (1 << 6)

/**
 * \def SBUF_SCOLON
 *
 * Add a semicolon after the string.
 */
#define SBUF_SCOLON (1 << 7)
/** @} */

/**
 * Reset the buffer
 *
 * \param[in] scrub If non-zero, scrub the buffer.
 */
void sbuf_reset(int scrub);

/**
 * Get a const-qualified pointer to the buffer.
 */
const char *sbuf_get_buffer(void);

/**
 * Add a string to the buffer.
 *
 * \param[in] s     String to add
 * \param[in] flags Formatting flags
 * \param[in] pos   Position at which to add the string (0 = current)
 * \return 0 on success, 1 on failure.
 */
int sbuf_add_str(const char *s, long flags, size_t pos);

/**
 * Add an unsigned integer to the buffer.
 *
 * \param[in] num Number to add
 * \param[in] flags Formatting flags
 * \return 0 on success, 1 on failure.
 */
int sbuf_add_unum(unsigned long num, long flags);

/**
 * Add a signed integer to the buffer.
 *
 * \param[in] num   Number to add
 * \param[in] flags Formatting flags
 * \return 0 on success, 1 on failure.
 */
int sbuf_add_snum(long num, long flags);

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
int sbuf_add_param_str(const char *param, const char *value);

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
int sbuf_add_param_num(const char *param, unsigned long value);

/**
 * Add a signed numeric parameter to the string buffer.
 *
 * This will add:
 *     param=value
 *
 * to the buffer, adding a leading space between parameters.
 *
 * \param[in] param  Parameter name
 * \param[in] value  Signed numeric value
 * \return 0 on success, 1 on failure.
 */
int sbuf_add_param_snum(const char *param, long value);

#endif /* STRINGBUF_H */
