/**
 * Minimal Migration Manager - File-mapping Functions
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */

#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <limits.h>

#ifndef IN_TESTS
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#endif /* IN_TESTS */

#include "utils.h"
#include "file.h"

/**
 * Trying to read() more than SSIZE_MAX is undefined.
 * It's also quite unlikely that we would ever want
 * a buffer that big.
 */
#define FILEBUFSIZ (64 * 1024)
#if FILEBUFSIZ >= SSIZE_MAX
#error "File buffer cannot be SSIZE_MAX or larger"
#endif

static char filebuf[FILEBUFSIZ];
static int file_mapped = 0;
static size_t file_size = 0;

/**
 * Read a file into the static file buffer.
 *
 * \param[in] fd   File descriptor to read from.
 * \param[in] size Length of data to read.
 * \return 0 on success, non-zero on error.
 */
static int read_file(int fd, size_t size)
{
	int retval = 0;
	size_t bytes_read = 0;
	ssize_t br;

	/* Ensure we're at the start of the file */
	if (lseek(fd, 0, SEEK_SET) < 0)
		goto err;

	do {
		/* Read what we can */
		errno = 0;
		br = read(fd, filebuf + bytes_read, size - bytes_read);
		if (br < 0) {
			if (errno == EINTR)
				continue;
			goto err;
		}

		if (br == 0) goto err;
		bytes_read += (size_t)br;
	} while (bytes_read < size);

ret:
	return retval;

err:
	++retval;
	goto ret;
}

/**
 * Read a file into our buffer.
 *
 * This will fail for any file which has a size too large
 * to hold in a off_t. The files this program uses shouldn't
 * be that large anyhow.
 *
 * \param[in]  path Path to the file to be mapped.
 * \param[out] size Size of the file (in bytes.)
 * \return A pointer to the file buffer. On error, NULL will
 *         be returned, and size will be set to 0.
 */
char *map_file(const char *path, size_t *size)
{
	int fd = -1;
	struct stat sbuf;
	char *retval = NULL;

	if (!path || !size) goto ret;

	/* Ensure another file isn't still mapped. */
	if (file_mapped) {
		ERROR_1("failed to map '%s': "
		        "another file is already mapped", path);
		goto ret;
	}

	/* Open the file */
	errno = 0;
	fd = open(path, O_RDONLY);
	if (fd < 0) goto err;

	/* We don't map 0-length files */
	if (fstat(fd, &sbuf) || !sbuf.st_size)
		goto err;

	/* Don't map non-regular files either */
	if (!S_ISREG(sbuf.st_mode)) {
		ERROR_1("'%s' is not a regular file", path);
		goto ret;
	}

	/* Ensure the file will fit in our buffer */
	if ((size_t)sbuf.st_size >= FILEBUFSIZ) {
		ERROR_2("bad size for '%s' (%ld bytes)", path,
		        sbuf.st_size);
		goto ret;
	}

	/* Read the file */
	++file_mapped;
	file_size = (size_t)sbuf.st_size;
	if (read_file(fd, (size_t)sbuf.st_size))
		goto err;
	retval = filebuf;

ret:
	if (size) *size = file_size;
	if (fd > -1) close(fd);
	return retval;

err:
	if (errno) {
		ERROR_2("failed to map '%s': %s", path, strerror(errno));
	} else ERROR_1("failed to map '%s'", path);
	if (file_mapped) file_mapped = 0;
	file_size = 0;
	goto ret;
}

/**
 * Unmap the previously mapped file.
 */
void unmap_file(void)
{
	if (!file_mapped)
		return;

	memset(filebuf, 0, file_size);
	file_mapped = 0;
	file_size = 0;
}
