dnl
dnl Minimal Migration Manager - m4 Macros
dnl Copyright (C) 2016 Tim Hentenaar.
dnl
dnl This code is licenced under the Simplified BSD License.
dnl See the LICENSE file for details.
dnl
dnl SYNOPSIS
dnl
dnl AX_CHECK_LIBGIT2 - Check for a usable SQLite3 installation.
dnl
dnl DESCRIPTION
dnl
dnl This macro sets `have_libgit2` to either "yes" or "no" to indicate
dnl the presence of libgit2, and appends the necessary options
dnl to CPPFLAGS, LDFLAGS, and LIBS.
dnl
dnl `have_libgit2` is also substituted in the output.
dnl

AC_DEFUN([AX_CHECK_LIBGIT2],[
	have_libgit2=no
	AC_ARG_WITH([libgit2],
		[AS_HELP_STRING(
			[--with-libgit2=PATH],
			[enable git support via libgit2])],
		[with_libgit2=$withval],
		[with_libgit2=yes]
	)

	AS_IF([test "$with_libgit2" != "no"],[
		have_libgit2=yes
		save_cppflags=$CPPFLAGS
		save_ldflags=$LDFLAGS
		save_libs=$LIBS

		dnl Check for git2.h
		AS_IF([test "$with_libgit2" == "yes"],[with_libgit2=/usr])
		CPPFLAGS="$CPPFLAGS -DHAVE_GIT -I$with_libgit2/include"
		AC_CHECK_HEADER([git2.h], [], [have_libgit2=no])

		dnl Check for liblibgit2
		AS_IF([test "$have_libgit2" != "no"],[
			LDFLAGS="$LDFLAGS -L$with_libgit2/lib"
			AC_CHECK_LIB([git2], [git_libgit2_init], [], [have_libgit2=no])
		])

		AS_IF([test "$have_libgit2" == "no"],[
			CPPFLAGS=$save_cppflags
			LDFLAGS=$save_ldflags
			LIBS=$save_libs
			AC_MSG_WARN([git support disabled.])
		])
	])

	AC_SUBST([have_libgit2])
]) dnl AX_CHECK_LIBGIT2
