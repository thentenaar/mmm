dnl
dnl Minimal Migration Manager - m4 Macros
dnl Copyright (C) 2016 Tim Hentenaar.
dnl
dnl This code is licenced under the Simplified BSD License.
dnl See the LICENSE file for details.
dnl
dnl SYNOPSIS
dnl
dnl AX_CHECK_MYSQL - Check for a usable MariaDB / MySQL installation.
dnl
dnl DESCRIPTION
dnl
dnl This macro sets `have_mysql` to either "yes" or "no" to indicate
dnl the presence of MySQL or MariaDB in the system; with MariaDB
dnl being the first choice. If found, -DHAVE_MYSQL will be added
dnl to CPPFLAGS, and LIBS will be updated accordingly.
dnl
dnl `have_mysql` is also substituted in the output.
dnl

AC_DEFUN([AX_CHECK_MYSQL],[
	have_mysql=no
	AC_ARG_WITH([mysql],
		[AS_HELP_STRING(
			[--with-mysql=PATH],
			[enable support for MariaDB / MySQL])],
		[with_mysql=$withval],
		[with_mysql=yes]
	)

	dnl Look for mariadb-config / mysql-config
	mysql_config=no
	AS_IF([test "$with_mysql" == "yes"],[with_mysql=$bindir])
	AS_IF([test "$with_mysql" != "no"],[
		AC_PATH_PROG([mariadb_config],
			[mariadb_config], [no], [$with_mysql:$PATH]
		)
		AS_IF([test "$mariadb_config" == "no"],[
			AC_PATH_PROG([mysql_config],
				[mysql_config], [no], [$with_mysql:$PATH]
			)
		], [ mysql_config=${mariadb_config} ])
	])

	dnl Get the LIBS and CFLAGS, check the header
	AS_IF([test "$mysql_config" != "no"],[
		have_mysql=yes
		save_cppflags=$CPPFLAGS
		save_LIBS=$LIBS
		MYSQL_CFLAGS=$(${mysql_config} --cflags)
		MYSQL_LIBS=$(${mysql_config} --libs)
		CPPFLAGS="$CPPFLAGS -DHAVE_MYSQL $MYSQL_CFLAGS"
		LIBS="$LIBS $MYSQL_LIBS"
		AC_CHECK_HEADER([mysql.h], [], [have_mysql=no])

		AS_IF([test "$have_mysql" == "no"],[
			CPPFLAGS=$save_cppflags
			LIBS=$save_libs
			AC_MSG_WARN([MariaDB / MySQL support disabled.])
		])
	])

	AC_SUBST([have_mysql])
]) dnl AX_CHECK_MYSQL
