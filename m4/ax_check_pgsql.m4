dnl
dnl Minimal Migration Manager - m4 Macros
dnl Copyright (C) 2016 Tim Hentenaar.
dnl
dnl This code is licenced under the Simplified BSD License.
dnl See the LICENSE file for details.
dnl
dnl SYNOPSIS
dnl
dnl AX_CHECK_PGSQL - Check for a usable PostgreSQL installation.
dnl
dnl DESCRIPTION
dnl
dnl This macro sets `have_pgsql` to either "yes" or "no" to indicate
dnl the presence of PostgreSQL, and appends the necessary options
dnl to CPPFLAGS, LDFLAGS, and LIBS.
dnl
dnl `have_pgsql` is also substituted in the output.
dnl

AC_DEFUN([AX_CHECK_PGSQL],[
	have_pgsql=no
	AC_ARG_WITH([pgsql],
		[AS_HELP_STRING(
			[--with-pgsql=PATH],
			[enable support for PostgreSQL])],
		[with_pgsql=$withval],
		[with_pgsql=yes]
	)

	dnl Look for pg_config
	pg_config=no
	AS_IF([test "$with_pgsql" == "yes"],[with_pgsql=$bindir])
	AS_IF([test "$with_pgsql" != "no"],[
		AC_PATH_PROG([pg_config], [pg_config], [no], [$with_pgsql:$PATH])
	])

	dnl Check for libpq-fe.h and libpq
	AS_IF([test "$pg_config" != "no"],[
		have_pgsql=yes

		dnl Check for libpq-fe.h
		save_ldflags=$LDFLAGS
		save_cppflags=$CPPFLAGS
		CPPFLAGS="$CPPFLAGS -DHAVE_PGSQL -I$($pg_config --includedir)"
		AC_CHECK_HEADER([libpq-fe.h], [], [have_pgsql=no])

		dnl Check for libpq
		AS_IF([test "$have_pgsql" != "no"],[
			AS_IF([test "$have_pgsql" != "no" ],[
				LDFLAGS="$LDFLAGS -L$($pg_config --libdir)"
				AC_CHECK_LIB([pq], [PQconnectdb], [], [have_pgsql=no])
			])
		])

		AS_IF([test "$have_pgsql" == "no"],[
			LDFLAGS=$save_ldflags
			CPPFLAGS=$save_cppflags
			AC_MSG_WARN([PostgreSQL support disabled.])
		])
	])

	AC_SUBST([have_pgsql])
]) dnl AX_CHECK_PGSQL
