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

#if defined(HAVE_SYS_MMAN_H) && defined(_POSIX_MAPPED_FILES) && _POSIX_MAPPED_FILES != -1
#include <sys/mman.h>
#endif
#endif /* IN_TESTS */

#include "utils.h"
#include "file.h"

#if !defined(HAVE_SYS_MMAN_H) || !defined(_POSIX_MAPPED_FILES) || _POSIX_MAPPED_FILES == -1
#define MAP_PRIVATE 0
#define PROT_READ   0
#define MAP_FAILED  ((void *)-1)

static void *mmap(void *addr, size_t length, int prot, int flags, int fd,
                  off_t offset)
{
	char *buf = NULL;
	size_t bytes_read = 0;
	ssize_t br;
	struct stat sbuf;

	(void)addr;
	(void)prot;
	(void)flags;
	(void)offset;

	if (lseek(fd, 0, SEEK_SET) < 0) {
		errno = EBADF;
		goto err;
	}

	if (fstat(fd, &sbuf) || !S_ISREG(sbuf.st_mode)) {
		errno = EACCES;
		goto err;
	}

	if (!(buf = malloc(length))) {
		errno = ENOMEM;
		goto err;
	}

	do {
		/* Read what we can */
		if ((br = read(fd, buf + bytes_read, length - bytes_read)) <= 0) {
			if (errno == EINTR)
				continue;
			goto err;
		}

		bytes_read += (size_t)br;
	} while (bytes_read < length);

	return buf;

err:
	if (buf) free(buf);
	return MAP_FAILED;
}

static int munmap(void *addr, size_t length)
{
	(void)length;
	if (addr != MAP_FAILED)
		free(addr);
	return 0;
}
#endif /* !HAVE_SYS_MMAN_H || !_POSIX_MAPPED_FILES */

/**
 * Map a file into memory
 *
 * \param[in]  path   Path to the file to be mapped.
 * \param[out] size   Size of the file (in bytes.)
 * \return A pointer to the file buffer. On error, NULL will
 *         be returned, and size will be set to 0.
 */
char *map_file(const char *path, size_t *size)
{
	int fd = -1;
	struct stat sbuf;
	char *retval = NULL;

	sbuf.st_size = 0;
	if (!path || !size)
		goto ret;

	/* Open the file */
	errno = 0;
	fd = open(path, O_RDONLY);
	if (fd < 0 || fstat(fd, &sbuf) || !sbuf.st_size)
		goto err;

	/* Map the file */
	retval = mmap(NULL, (size_t)sbuf.st_size, MAP_PRIVATE, PROT_READ, fd, 0);
	if (retval == MAP_FAILED)
		goto err;

ret:
	if (size) *size = (size_t)sbuf.st_size;
	if (fd > -1) close(fd);
	return retval;

err:
	if (errno) {
		error("failed to map '%s': %s", path, strerror(errno));
	} else error("failed to map '%s'", path);
	if (fd > -1) close(fd);
	if (size) *size = 0;
	return NULL;
}

/**
 * Unmap a previously mapped file
 *
 * \param[in] mem Base address of the file mapping
 * \param[in] len Length of the file
 */
void unmap_file(char *mem, size_t len)
{
	if (mem && len)
		munmap(mem, len);
}

