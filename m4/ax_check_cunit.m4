dnl
dnl Minimal Migration Manager - m4 Macros
dnl Copyright (C) 2016 Tim Hentenaar.
dnl
dnl This code is licenced under the Simplified BSD License.
dnl See the LICENSE file for details.
dnl
dnl SYNOPSIS
dnl
dnl AX_CHECK_CUNIT - Check for a usable CUnit installation.
dnl
dnl DESCRIPTION
dnl
dnl This macro sets `have_cunit` to either "yes" or "no" to indicate
dnl the presence of CUnit,appends the necessary options to
dnl TEST_CPPFLAGS, TEST_LDFLAGS, and TEST_LIBS, and substitutes them.
dnl
dnl The CUnit version number is parsed from the header file, and
dnl is included in TEST_CPPFLAGS as TEST_CU_VER. This will be
dnl set to 0 if it cannot be reckoned.
dnl
dnl `have_cunit` is also substituted in the output.
dnl

AC_DEFUN([AX_CHECK_CUNIT],[
	have_cunit=no
	AC_ARG_WITH([cunit],
		[AS_HELP_STRING(
			[--with-cunit=PATH],
			[enable support for CUnit])],
		[with_cunit=$withval],
		[with_cunit=yes]
	)

	AS_IF([test "$with_cunit" != "no"],[
		have_cunit=yes
		save_cppflags=$CPPFLAGS
		save_ldflags=$LDFLAGS
		save_libs=$LIBS

		dnl Check for CUnit.h
		AS_IF([test "$with_cunit" == "yes"],[with_cunit=$prefix])
		CPPFLAGS="$TEST_CPPFLAGS -I$with_cunit/include"
		AC_CHECK_HEADER([CUnit/CUnit.h], [], [have_cunit=no])

		dnl Check for libcunit
		AS_IF([test "$have_cunit" != "no"],[
			LDFLAGS="$LDFLAGS -L$with_cunit/lib"
			AC_CHECK_LIB([cunit], [CU_run_all_tests], [], [have_cunit=no])
		])

		AS_IF([test "$have_cunit" == "no"],[
			CPPFLAGS=$save_cppflags
			LDFLAGS=$save_ldflags
			LIBS=$save_libs
			TEST_CPPFLAGS="-DTEST_CU_VER=0"
			AC_MSG_WARN([CUnit support disabled.])
		],[
			CPPFLAGS=$save_cppflags
			LDFLAGS=$save_ldflags
			LIBS=$save_libs

			TEST_CPPFLAGS="$TEST_CPPFLAGS -I$with_cunit/include"
			TEST_LDFLAGS="$TEST_LDFLAGS -L$with_cunit/lib"
			TEST_LIBS="$TEST_LIBS -lcunit"

			dnl Try to guess the header path
			cunit_header="nxfile"
			AS_IF([test -f $with_cunit/include/CUnit/CUnit.h],[
				cunit_header=$with_cunit/include/CUnit/CUnit.h
			],[
				AS_IF([test -f /usr/include/CUnit/CUnit.h],[
					cunit_header=/usr/include/CUnit/CUnit.h
				])
			])

			dnl Get the CUnit version
			CUNIT_VER=""
			AS_IF([test -f $cunit_header],[
				CUNIT_VER=$(
					$GREP CU_VERSION $cunit_header | cut -d ' ' -f 3 |
					$SED -e 's/"//g' -e 's/\.//g' -e 's/-//g'
				)
			])
			AS_IF([test "x$CUNIT_VER" != "x"],[
				TEST_CPPFLAGS="$TEST_CPPFLAGS -DTEST_CU_VER=$CUNIT_VER"
			],[
				TEST_CPPFLAGS="$TEST_CPPFLAGS -DTEST_CU_VER=0"
			])
		])
	])

	AC_SUBST([TEST_CPPFLAGS])
	AC_SUBST([TEST_LDFLAGS])
	AC_SUBST([TEST_LIBS])
	AC_SUBST([have_cunit])
]) dnl AX_CHECK_CUNIT
