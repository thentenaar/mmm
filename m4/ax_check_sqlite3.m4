dnl
dnl Minimal Migration Manager - m4 Macros
dnl Copyright (C) 2016 Tim Hentenaar.
dnl
dnl This code is licenced under the Simplified BSD License.
dnl See the LICENSE file for details.
dnl
dnl SYNOPSIS
dnl
dnl AX_CHECK_SQLITE3 - Check for a usable SQLite3 installation.
dnl
dnl DESCRIPTION
dnl
dnl This macro sets `have_sqlite3` to either "yes" or "no" to indicate
dnl the presence of SQLite3, and appends the necessary options
dnl to CPPFLAGS, LDFLAGS, and LIBS.
dnl
dnl `have_sqlite3` is also substituted in the output.
dnl

AC_DEFUN([AX_CHECK_SQLITE3],[
	have_sqlite3=no
	AC_ARG_WITH([sqlite3],
		[AS_HELP_STRING(
			[--with-sqlite3=PATH],
			[enable support for SQLite3])],
		[with_sqlite3=$withval],
		[with_sqlite3=yes]
	)

	AS_IF([test "$with_sqlite3" != "no"],[
		have_sqlite3=yes
		save_cppflags=$CPPFLAGS
		save_ldflags=$LDFLAGS
		save_libs=$LIBS

		dnl Check for sqlite3.h
		AS_IF([test "$with_sqlite3" == "yes"],[with_sqlite3=$prefix])
		CPPFLAGS="$CPPFLAGS -DHAVE_SQLITE3 -I$with_sqlite3/include"
		AC_CHECK_HEADER([sqlite3.h], [], [have_sqlite3=no])

		dnl Check for libsqlite3
		AS_IF([test "$have_sqlite3" != "no"],[
			LDFLAGS="$LDFLAGS -L$with_sqlite3/lib"
			AC_CHECK_LIB([sqlite3], [sqlite3_open], [], [have_sqlite3=no])
		])

		AS_IF([test "$have_sqlite3" == "no"],[
			CPPFLAGS=$save_cppflags
			LDFLAGS=$save_ldflags
			LIBS=$save_libs
			AC_MSG_WARN([SQLite3 support disabled.])
		])
	])

	AC_SUBST([have_sqlite3])
]) dnl AX_CHECK_SQLITE3
