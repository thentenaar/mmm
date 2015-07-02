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
#include <limits.h>

/* {{{ GCC: UNUSED() macro */
/**
 * \def UNUSED(x)
 *
 * Mark a function parameter or variable unused.
 *
 * This macro only has effect on GCC or clang, where
 * the 'unused' attribute to suppress 'unused parameter'
 * warnings is valid.
 */
#if defined(__GNUC__) || defined(__clang__)
#define UNUSED(x) x __attribute__((__unused__))
#else
#define UNUSED(x) x
#endif /* }}} */

/* {{{ GCC < 3: __func__ */
/**
 * \def __func__
 *
 * C99 includes this but some compilers had
 * __FUNCTION__ before that. So, if we're
 * being compiled by a compiler without C99
 * support, let's hope that __FUNCTION__ exists.
 */
#if !defined(__STDC_VERSION__) || __STDC_VERSION__ < 199901L
#if !defined(__GNUC__) || __GNUC__ < 3
#define __func__ __FUNCTION__
#endif /* !__GNUC__ || __GNUC__ < 3   */
#endif /* !__STDC_VERSION__ < 199901L }}} */

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

/* {{{ ERROR() macros */
#ifndef IN_TESTS
/**
 * \def ERROR
 * \def ERROR_1
 * \def ERROR_2
 * \def ERROR_3
 * \def ERROR_4
 * \def ERROR_5
 *
 * Simple error message macros.
 */
#define ERROR(msg) do {\
	fprintf(stderr, "%s: " msg "\n", __func__);\
} while (0);

#define ERROR_1(fmt, arg1) do {\
	fprintf(stderr, "%s: " fmt "\n", __func__, (arg1));\
} while (0);

#define ERROR_2(fmt, arg1, arg2) do {\
	fprintf(stderr, "%s: " fmt "\n", __func__, (arg1), (arg2));\
} while (0);

#define ERROR_3(fmt, arg1, arg2, arg3) do {\
	fprintf(stderr, "%s: " fmt "\n", __func__, (arg1),\
	        (arg2), (arg3));\
} while (0);

#define ERROR_4(fmt, arg1, arg2, arg3, arg4) do {\
	fprintf(stderr, "%s: " fmt "\n", __func__, (arg1),\
	        (arg2), (arg3), (arg4));\
} while (0);

#define ERROR_5(fmt, arg1, arg2, arg3, arg4, arg5) do {\
	fprintf(stderr, "%s: " fmt "\n", __func__, (arg1),\
	        (arg2), (arg3), (arg4), (arg5));\
} while (0);
#else /* IN_TESTS */
/**
 * During testing, errors should be written to the static buffer
 * defined in test/test_runner.c instead of going to stderr.
 */

#define ERROR(msg) do {\
	sprintf(errbuf, "%s: " msg "\n", __func__);\
} while (0);

#define ERROR_1(fmt, arg1) do {\
	sprintf(errbuf, "%s: " fmt "\n", __func__, (arg1));\
} while (0);

#define ERROR_2(fmt, arg1, arg2) do {\
	sprintf(errbuf, "%s: " fmt "\n", __func__, (arg1), (arg2));\
} while (0);

#define ERROR_3(fmt, arg1, arg2, arg3) do {\
	sprintf(errbuf, "%s: " fmt "\n", __func__, (arg1),\
	        (arg2), (arg3));\
} while (0);

#define ERROR_4(fmt, arg1, arg2, arg3, arg4) do {\
	sprintf(errbuf, "%s: " fmt "\n", __func__, (arg1),\
	        (arg2), (arg3), (arg4));\
} while (0);

#define ERROR_5(fmt, arg1, arg2, arg3, arg4, arg5) do {\
	sprintf(errbuf, "%s: " fmt "\n", __func__, (arg1),\
	        (arg2), (arg3), (arg4), (arg5));\
} while (0);
#endif /* IN_TESTS }}} */

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
