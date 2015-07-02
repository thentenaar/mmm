/**
 * Minimal Migration Manager - Utility Macros / Functions
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include "utils.h"

/**
 * Comparitor for bubblesort().
 *
 * \param[in] a String a
 * \param[in] b String b
 * \return -1 if a < b, 0 if a == b, 1 if a > b.
 *
 * This function is designed to compare two strings
 * which begin with an unsigned numerical designation.
 * If the function lacks a numeric designation, or if
 * the initial portion of the string doesn't convert
 * to an unsigned long, this implementation will fall
 * back to using strcoll() as a comparitor.
 */
static int bscmp(char *a, char *b)
{
	unsigned long x, y;
	char *end1, *end2;
	int retval = 0;

	/* Compare the numerical designations. */
	errno = 0;
	x = strtoul(a, &end1, 0);

	/* If the conversion failed, fallback to strcoll(). */
	if (!end1 || end1 == a || (x == ULONG_MAX && errno == ERANGE))
		goto use_strcoll;

	/* Convert b. */
	errno = 0;
	y = strtoul(b, &end2, 0);

	/* Check again for falling back to strcoll(). */
	if (!end2 || end2 == b || (y == ULONG_MAX && errno == ERANGE))
		goto use_strcoll;

	/**
	 * If the designations are equal, and there's
	 * more to our two strings (as there should be
	 * in the cases where this is used) compare
	 * the remainder of the string with strcoll(),
	 * after positively biasing a name that only
	 * has the designation and ".sql" towards the
	 * beginning of the array.
	 */
	if (x == y && *end1 && *end2) {
		if (strlen(end1) == 4 && !memcmp(end1, ".sql", 4) &&
		    (strlen(end2) != 4 || memcmp(end2, ".sql", 4))) {
			retval = -1;
			goto ret;
		}

		if (strlen(end2) == 4 && !memcmp(end2, ".sql", 4) &&
		    (strlen(end1) != 4 || memcmp(end1, ".sql", 4))) {
			retval = 1;
			goto ret;
		}

		a = end1 + 1;
		b = end2 + 1;
		goto use_strcoll;
	}

	/* Otherwise return -1, 0, or 1 */
	retval = (x < y) ? -1 : (x == y) ? 0 : 1;

ret:
	return retval;

use_strcoll:
	retval = strcoll(a, b);
	goto ret;
}

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
void bubblesort(char *a[], size_t size)
{
	char *tmp;
	size_t i = size - 1, j;

	if (!a || size < 2) goto ret;
	do {
		for (j = i - 1; j < size - 1; j++) {
			if (bscmp(a[j], a[j + 1]) > 0) {
				tmp = a[j + 1];
				a[j + 1] = a[j];
				a[j] = tmp;
			}
		}
	} while (--i);

ret:
	return;
}
