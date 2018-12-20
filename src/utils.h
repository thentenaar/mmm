/**
 * \file utils.h
 *
 * Minimal Migration Manager - Utility Macros / Functions
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#ifndef UTILS_H
#define UTILS_H

#include <stdio.h>
#include <stdarg.h>
#include <limits.h>

/* {{{ SIZE_MAX */
/**
 * \def SIZE_MAX
 *
 * Maximum value of a \a size_t since this constant
 * isn't guarenteed to be available. SSIZE_MAX is
 * required to be provided by limits.h, however.
 */
#ifndef SIZE_MAX
#if SSIZE_MAX == LONG_MAX
#define SIZE_MAX ULONG_MAX
#else
#define SIZE_MAX (SSIZE_MAX << 1)
#endif /* SSIZE_MAX == LONG_MAX */
#endif /* }}} */

/**
 * \brief Log an error message
 * \param[in] fmt Format string
 */
void error(const char *fmt, ...);

/* {{{ PRINT() macros */
#ifndef IN_TESTS
/**
 * \def PRINT
 * \def PRINT_1
 *
 * Simple output printing macros.
 */
#define PRINT(msg) do { printf((msg)); } while (0);
#define PRINT_1(fmt, arg1) do { printf((fmt), (arg1)); } while (0);
#else /* IN_TESTS */
/**
 * During testing, output should be written to the static buffer
 * defined in test/test_runner.c instead of going to stdout.
 */
#define PRINT(msg) do { sprintf(errbuf, (msg)); } while (0);
#define PRINT_1(fmt, arg1) do {\
	sprintf(errbuf, (fmt), (arg1));\
} while (0);
#endif /* IN_TESTS }}} */

/**
 * Bubblesort an array of strings, assuming that each string
 * begins with a numeric designation.
 *
 * \param[in] a    Array of strings
 * \param[in] size Number of elements in the array
 *
 * If the strings don't begin with a numeric designation, they
 * will be sorted via strcoll().
 *
 * This runs in O(n^2) worst case time and  O(n) best case time,
 * with O(1) space complexity.
 */
void bubblesort(char *a[], size_t size);

#endif /* UTILS_H */
