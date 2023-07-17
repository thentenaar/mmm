dnl
dnl
dnl Minimal Migration Manager
dnl Copyright (C) 2016 Tim Hentenaar.
dnl
dnl This code is licenced under the Simplified BSD License.
dnl See the LICENSE file for details.
dnl

AC_PREREQ([2.63])
AC_INIT([mmm], [1.0], [http://github.com/thentenaar/mmm])
AC_CONFIG_AUX_DIR([.])
AC_LANG([C])

dnl Include local macros
m4_include([m4/ax_append_compile_flags.m4])
m4_include([m4/ax_append_flag.m4])
m4_include([m4/ax_check_compile_flag.m4])
m4_include([m4/ax_require_defined.m4])
m4_include([m4/ax_strict_cflags.m4])
m4_include([m4/ax_set_prefix.m4])
m4_include([m4/ax_check_dep.m4])

dnl Check for standard programs
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_SED
AC_PROG_MKDIR_P
AC_PROG_INSTALL

dnl Check for headers
AC_HEADER_DIRENT
AC_CHECK_HEADERS([errno.h limits.h fcntl.h unistd.h sys/stat.h sys/types.h sys/mman.h])

dnl Check compiler characteristics
AC_C_CONST
AC_TYPE_SIZE_T

dnl Check for Databases
AX_SET_PREFIX
AX_CHECK_DEP([sqlite3], [sqlite3.h], [sqlite3], [sqlite3_open], [nonfatal])
AX_CHECK_DEP([pgsql], [libpq-fe.h], [pq], [PQconnectdb], [nonfatal])
AX_CHECK_DEP([mysql], [mysql.h], [mysql], [mysql_library_init], [nonfatal])

dnl Ensure at least one supported database is available
AS_IF([test "$have_sqlite3" != "yes"], [
AS_IF([test "$have_pgsql"   != "yes"], [
AS_IF([test "$have_mysql"   != "yes"], [
	AC_MSG_ERROR("No database drivers would be built.")
])
])
])

dnl Check for libgit2
AX_CHECK_DEP([libgit2], [git2.h], [git2], [git_libgit2_init], [nonfatal])

dnl Check for check, gcovr, and indent
AX_CHECK_DEP([check], [check.h], [check], [srunner_run_all], [nonfatal])
AC_PATH_PROG([GCOVR],[gcovr])
AC_PATH_PROG([INDENT],[indent])

dnl Use strict CFLAGS and SUSv2 compliant interfaces
CFLAGS="-O2 -D_XOPEN_SOURCE=500 -ansi"
AX_STRICT_CFLAGS
CFLAGS="$CFLAGS -Werror"

AC_CONFIG_FILES([Makefile test/Makefile])
AC_OUTPUT
