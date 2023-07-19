/**
 * Minimal Migration Manager - POSIX stubs for tests
 * Copyright (C) 2015 Tim Hentenaar.
 *
 * This code is licenced under the Simplified BSD License.
 * See the LICENSE file for details.
 */
#ifndef TEST_POSIX_STUBS_H
#define TEST_POSIX_STUBS_H

#include <limits.h>
#include <errno.h>

/* {{{ POSIX types, macros, and structs */
typedef unsigned int mode_t;
typedef long ssize_t;
typedef long off_t;
typedef void DIR;

#ifndef NULL
#define NULL ((void *)0)
#endif

/* open: flags */
#define O_RDONLY 0x01
#define O_WRONLY 0x02
#define O_CREAT  0x04
#define O_EXCL   0x08
#define O_TRUNC  0x10

/* stat: File mode testing */
#define S_IFREG 01000
#define S_IFDIR 04000

#define S_ISREG(m) ((m) & S_IFREG)
#define S_ISDIR(m) ((m) & S_IFDIR)

/* stat: Permissions bits for mode */
#define S_IXUSR 0100
#define S_IXGRP 0010
#define S_IXOTH 0001
#define S_IRWXU 0700
#define S_IRWXG 0070
#define S_IRWXO 0007

/* lseek: whence values */
#define SEEK_SET 0

/**
 * Less-than-minimal representation of a stat
 * structure.
 */
struct stat {
	off_t st_size;
	mode_t st_mode;
};

struct dirent {
	char d_name[50];
	struct dirent *next;
};
/* }}} */

/* {{{ Function called counters */
static int stat_called   = 0;
static int fstat_called  = 0;
static int open_called   = 0;
static int close_called  = 0;
static int write_called  = 0;
static int read_called   = 0;
static int mkdir_called  = 0;
static int unlink_called = 0;
static int lseek_called   = 0;
static int opendir_called = 0;
static int readdir_called = 0;
static int closedir_called = 0;
/* }}} */

/* {{{ Function failure counters */
static int stat_fails_at    = 0;
static int fstat_fails_at   = 0;
static int open_fails_at    = 0;
static int close_fails_at   = 0;
static int write_fails_at   = 0;
static int read_fails_at    = 0;
static int mkdir_fails_at   = 0;
static int unlink_fails_at  = 0;
static int lseek_fails_at   = 0;
static int opendir_fails_at = 0;
static int readdir_fails_at = 0;
static int closedir_fails_at = 0;
/* }}} */

/* {{{ Stub return values */
static struct stat stat_returns_buf;
static int stat_returns      = 0;
static int fstat_returns     = 0;
static int open_returns      = 0;
static int close_returns     = 0;
static ssize_t write_returns = 0;
static ssize_t read_returns  = 0;
static int mkdir_returns     = 0;
static int unlink_returns    = 0;
static int lseek_returns     = 0;
static void *opendir_returns = NULL;
static struct dirent *readdir_returns = NULL;
static int closedir_returns  = 0;
/* }}} */

/* {{{ Stub errno values */
static int stat_errno    = -1;
static int fstat_errno    = -1;
static int open_errno    = -1;
static int close_errno   = -1;
static int write_errno   = -1;
static int read_errno    = -1;
static int mkdir_errno   = -1;
static int unlink_errno  = -1;
static int lseek_errno   = -1;
static int opendir_errno = -1;
static int readdir_errno = -1;
static int closedir_errno = -1;
/* }}} */

/* {{{ reset_stubs: Reset the state variables for the stubs */
static void reset_stubs(void)
{
	/* Return values */
	stat_returns   = 0; open_returns  = 0; close_returns = 0;
	write_returns  = 0; read_returns  = 0; mkdir_returns = 0;
	unlink_returns = 0; lseek_returns = 0; fstat_returns = 0;
	opendir_returns = NULL;
	readdir_returns = NULL;
	closedir_returns = 0;

	/* Called couters, errno, etc. */
	stat_called    = 0; stat_fails_at    = 0; stat_errno    = -1;
	fstat_called   = 0; fstat_fails_at   = 0; fstat_errno   = -1;
	open_called    = 0; open_fails_at    = 0; open_errno    = -1;
	close_called   = 0; close_fails_at   = 0; close_errno   = -1;
	write_called   = 0; write_fails_at   = 0; write_errno   = -1;
	read_called    = 0; read_fails_at    = 0; read_errno    = -1;
	mkdir_called   = 0; mkdir_fails_at   = 0; mkdir_errno   = -1;
	unlink_called  = 0; unlink_fails_at  = 0; unlink_errno  = -1;
	lseek_called   = 0; lseek_fails_at   = 0; lseek_errno   = -1;
	opendir_called = 0; opendir_fails_at = 0; opendir_errno = -1;
	readdir_called = 0; readdir_fails_at = 0; readdir_errno = -1;
	closedir_called = 0; closedir_fails_at = 0; closedir_errno = -1;

	memset(&stat_returns_buf, 0, sizeof(struct stat));
} /* }}} */

/* {{{ Ultra-light stub macro
 *
 * This macro is the actual implementation of most of these
 * stubs. It incrememnts the 'called' counter, returns the
 * specified result and/or sets errno and returns -1.
 *
 * It also asserts that errno == 0 before setting errno. This
 * will catch any place where errno is expected to be set by
 * the call, but not initialized to 0 beforehand.
 */
#define DO_STUB(func) do {\
	func ## _called ++;\
	if (!-- func ## _fails_at)\
		return -1;\
	if (func ## _errno >= 0) {\
		ck_assert_int_eq(errno, 0);\
		errno = func ## _errno ;\
		func ## _errno = -1;\
		return -1;\
	}\
	return func ## _returns ;\
} while (0); /* }}} */

/* {{{ GCC: Disable warnings
 *
 * The parameters in the functions below are intentionally unused,
 * and not all functions may be used in the translation unit including
 * this file. Thus, we want GCC to see this as a system header and
 * not complain about unused functions and the like.
 */
#if defined(__GNUC__) && __GNUC__ >= 3
#pragma GCC system_header
#endif /* GCC >= 3 }}} */

/* {{{ stat */
static int stat(const char *pathname, struct stat *buf)
{
	if (buf) memcpy(buf, &stat_returns_buf, sizeof(struct stat));
	DO_STUB(stat);
}
/* }}} */

/* {{{ fstat */
static int fstat(int fd, struct stat *buf)
{
	if (buf) memcpy(buf, &stat_returns_buf, sizeof(struct stat));
	DO_STUB(fstat);
}
/* }}} */

/* {{{ open */
static int open(const char *pathname, int flags, ...)
{
	DO_STUB(open);
}
/* }}} */

/* {{{ close */
static int close(int fd)
{
	DO_STUB(close);
}
/* }}} */

/* {{{ write */
static ssize_t write(int fd, const void *buf, size_t count)
{
	DO_STUB(write);
}
/* }}} */

/* {{{ read */
static ssize_t read(int fd, void *buf, size_t count)
{
	DO_STUB(read);
}
/* }}} */

/* {{{ mkdir */
static int mkdir(const char *pathname, mode_t mode)
{
	DO_STUB(mkdir);
}
/* }}} */

/* {{{ unlink */
static int unlink(const char *pathname)
{
	DO_STUB(unlink);
}
/* }}} */

/* {{{ lseek */
static off_t lseek(int fd, off_t offset, int whence)
{
	DO_STUB(lseek);
}
/* }}} */

/* {{{ opendir */
static DIR *opendir(const char *name)
{
	opendir_called++;
	if (!--opendir_fails_at) return NULL;
	if (opendir_errno >= 0) {
		errno = opendir_errno;
		opendir_errno = -1;
		return NULL;
	}

	return opendir_returns;
}
/* }}} */

/* {{{ readdir */
static struct dirent *readdir(DIR *dirp)
{
	struct dirent *d = readdir_returns;
	if (d) readdir_returns = d->next;

	readdir_called++;
	if (!--readdir_fails_at) return NULL;
	if (readdir_errno >= 0) {
		errno = readdir_errno;
		readdir_errno = -1;
		d = NULL;
	}

	return d;
}
/* }}} */

/* {{{ closedir */
static int closedir(DIR *dirp)
{
	DO_STUB(closedir);
}
/* }}} */

#endif /* TEST_POSIX_STUBS_H */

