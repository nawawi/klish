#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([MAJOR_VERSION], 3)
m4_define([MINOR_VERSION], 0)
m4_define([MICRO_VERSION], 0)

AC_PREREQ(2.59)
AC_INIT([klish],
        [MAJOR_VERSION.MINOR_VERSION.MICRO_VERSION],
        [serj.kalichev at gmail dot com])

AC_CONFIG_AUX_DIR(aux_scripts)
AC_CONFIG_MACRO_DIR([m4])

# Values for SONAME. See -version-info for details.
AC_SUBST(SONAME_CURRENT, 3)
AC_SUBST(SONAME_REVISION, 0)
AC_SUBST(SONAME_AGE, 0)

# Check for system extensions (_POSIX_THREAD_SEMANTICS for Solaris)
AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(subdir-objects)
AM_PROG_CC_C_O

# Dir for libc replacements
AC_CONFIG_LIBOBJ_DIR([libc])

# needed to handle 64-bit architecture
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(size_t)


#########################################
# See if linker supports version scripts
#########################################
# Check if LD supports linker scripts,
# and define automake conditional HAVE_LD_VERSION_SCRIPT if so.
AC_ARG_ENABLE([ld-version-script],
              AS_HELP_STRING([--enable-ld-version-script],
                             [enable linker version script (default is enabled when possible)]),
                             [have_ld_version_script=$enableval], [])
if test -z "$have_ld_version_script"; then
    AC_MSG_CHECKING([if LD -Wl,--version-script works])
    save_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS -Wl,--version-script=conftest.map"
    cat > conftest.map <<EOF
VERS_1 {
    global: sym;
};

VERS_2 {
    global: sym;
} VERS_1;
EOF
    AC_LINK_IFELSE([AC_LANG_SOURCE([int main() { return 0; }])],
                   [have_ld_version_script=yes], [have_ld_version_script=no])
    rm -f conftest.map
    LDFLAGS="$save_LDFLAGS"
    AC_MSG_RESULT($have_ld_version_script)
fi
AM_CONDITIONAL(HAVE_LD_VERSION_SCRIPT, test "$have_ld_version_script" = "yes")


################################
# Deal with debugging options
################################
AC_ARG_ENABLE(debug,
              [AS_HELP_STRING([--enable-debug],
                              [Turn on debugging including asserts [default=no]])],
              [],
              [enable_debug=no])
AM_CONDITIONAL(DEBUG,test x$enable_debug = xyes)


################################
# Compile in testc tests
################################
AC_ARG_ENABLE(testc,
              [AS_HELP_STRING([--enable-testc],
                              [Enable testc tests compiling [default=no]])],
              [],
              [enable_testc=no])
AM_CONDITIONAL(TESTC,test x$enable_testc = xyes)


################################
# Search for network functions (like connect())
################################
AC_SEARCH_LIBS([socket], [socket])


################################
# Check for regex.h
################################
AC_CHECK_HEADERS(regex.h, [],
    AC_MSG_ERROR([regex.h not found: regular expressions are not supported]))


################################
# Internal getopt()
################################
AC_ARG_WITH(internal-getopt,
            [AS_HELP_STRING([--with-internal-getopt],
            [Use internal implementation of getopt [default=no]])],
            [],
            [with_internal_getopt=no])

if test x$with_internal_getopt != xno; then
    AC_DEFINE([WITH_INTERNAL_GETOPT], [1], [Use internal getopt() implementation])
    AC_LIBOBJ([getopt])
    AC_MSG_WARN([Use internal implementation of getopt() and getopt_long()])
else
    AC_CHECK_HEADERS(getopt.h, [found_getopt_h=yes],
        AC_MSG_WARN([getopt.h not found: only short parameters can be used on command line]))
fi

AC_MSG_CHECKING([for getopt_long()])
if test x$with_internal_getopt = xyes -o x$found_getopt_h = xyes; then
    AC_DEFINE([HAVE_GETOPT_LONG], [1], [getopt_long() function])
    AC_MSG_RESULT([yes])
else
    AC_MSG_RESULT([no])
fi


################################
# Check for locale.h
################################
AC_CHECK_HEADERS(locale.h, [],
    AC_MSG_WARN([locale.h not found: the locales is not supported]))


################################
# Check for CODESET within nl_langinfo
################################
AC_DEFUN([AM_LANGINFO_CODESET],
[
  AC_CACHE_CHECK([for nl_langinfo and CODESET], [am_cv_langinfo_codeset],
    [AC_TRY_LINK([#include <langinfo.h>],
      [char* cs = nl_langinfo(CODESET); return !cs;],
      [am_cv_langinfo_codeset=yes],
      [am_cv_langinfo_codeset=no])
    ])
  if test $am_cv_langinfo_codeset = yes; then
    AC_DEFINE([HAVE_LANGINFO_CODESET], [1],
      [Define if you have <langinfo.h> and nl_langinfo(CODESET).])
  fi
])
AM_LANGINFO_CODESET


################################
# Check for pwd.h and grp.h
################################
AC_CHECK_HEADERS(pwd.h, [],
    AC_MSG_WARN([pwd.h not found: the pwd operations is not supported]))
AC_CHECK_HEADERS(grp.h, [],
    AC_MSG_WARN([grp.h not found: the grp operations is not supported]))


################################
# Check for chroot
################################
AC_CHECK_FUNCS(chroot, [],
    AC_MSG_WARN([chroot() not found: the choot is not supported]))


################################
# Check for dlopen
################################
CLISH_PLUGIN_BUILTIN_LIST=
CLISH_PLUGIN_BUILTIN_DEFS=
CLISH_PLUGIN_BUILTIN_LIBS=

AC_DEFUN([AC_PLUGIN_BUILTIN],
[
  CLISH_PLUGIN_BUILTIN_LIBS="$CLISH_PLUGIN_BUILTIN_LIBS clish_plugin_$1.la"
  CLISH_PLUGIN_BUILTIN_DEFS="$CLISH_PLUGIN_BUILTIN_DEFS CLISH_PLUGIN_INIT($1);"
  CLISH_PLUGIN_BUILTIN_LIST="$CLISH_PLUGIN_BUILTIN_LIST { \"$1\", clish_plugin_$1_init },"
])

AC_CHECK_HEADERS(dlfcn.h, [
        AC_SEARCH_LIBS([dlopen], [dl dld], [], [
          AC_MSG_ERROR([unable to find the dlopen() function])
        ])
    ],[
          AC_MSG_WARN([dlfcn.h not found: the dl operations is not supported])
          AC_PLUGIN_BUILTIN([clish])
          if test x$use_lua != xno; then
            AC_PLUGIN_BUILTIN([lua])
          fi
    ])

AC_SUBST([CLISH_PLUGIN_BUILTIN_LIST])
AC_SUBST([CLISH_PLUGIN_BUILTIN_DEFS])
AC_SUBST([CLISH_PLUGIN_BUILTIN_LIBS])
#AC_CONFIG_FILES([clish/plugin_builtin.c])


################################
# Variables to install plugins
################################
DBS_SUBDIR=dbs
AC_SUBST(DBS_SUBDIR)
PLUGINS_SUBDIR=plugins
AC_SUBST(PLUGINS_SUBDIR)


################################
# Check for mandatory faux library
################################
AC_ARG_WITH(faux,
            [AS_HELP_STRING([--with-faux=DIR],
                            [Search DIR directory for faux library files [default=yes]])],
            [use_faux=$withval],
            [use_faux=yes])

AS_IF([test x$use_faux != xyes],
  [
    CPPFLAGS="-I${use_faux} ${CPPFLAGS}"
    LDFLAGS="-L${use_faux}/.libs ${LDFLAGS}"
  ]
)

AC_CHECK_HEADERS([faux/faux.h],
	[],
	[AC_MSG_ERROR([cannot find <faux/faux.h> header file])]
	)
AC_SEARCH_LIBS([faux_zmalloc], [faux],
	[],
	[AC_MSG_ERROR([cannot find working faux library])]
	)


################################
# Check for the libxml2 library
################################
AC_ARG_WITH(libxml2,
	[AS_HELP_STRING([--with-libxml2=DIR],
		[Use libxml2 as the XML parser implementation [default=no]])],
	[use_libxml2=$withval],
	[use_libxml2=no])

AM_CONDITIONAL(WITH_LIBXML2,test x$use_libxml2 != xno)

case x$use_libxml2 in
xno)
	/bin/true
	;;
xyes)
	# I would love to avoid using pkg-config (which may not be available on
	# some compilation environment) but doing so really add a lot of
	# complexity to the system, as the headers don't lie in a standard
	# directory (they lie in a subdirectory of a standard include directory;
	# not the same thing for configure scripts).
	LIBXML2_CFLAGS="`pkg-config libxml-2.0 --cflags`"
	LIBXML2_LDFLAGS="`pkg-config libxml-2.0 --libs-only-L`"
	LIBXML2_LIBS="`pkg-config libxml-2.0 --libs-only-l`"
	AC_CHECK_LIB([xml2],
		[xmlNewDoc],
		[],
		[AC_MSG_ERROR([cannot find libxml2 headers])],
		[])
	# the header file is installed in a subdirectory of one of the standard
	# include directory. This might prove to be a problem if the cross-
	# compile environment is not complete enough (i.e. if it misses
	# pkg-config, or if pkg-config returns wrong values). In most cases, the
	# environment is likely to be OK so we will never hit any issue.
	saved_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS $LIBXML2_CFLAGS"
	AC_CHECK_HEADER([libxml/tree.h],
		[],
		[AC_MSG_ERROR([cannot find libxml2 headers])],
		[/* */])
	CFLAGS="$saved_CFLAGS"
	AC_DEFINE([HAVE_LIB_LIBXML2],
		[],
		[libxml2-based XML backend])
	;;
*)
	# this is probably broken. We consider that the user supplied path is
	# a non-standard path. But we're not going to check anything.
	AC_MSG_WARN([--with-libxml2=DIR is probably broken, just trying])
	LIBXML2_LDFLAGS="-L${use_libxml2}/lib"
	LIBXML2_CFLAGS="-I${use_libxml2}/include/libxml2"
	LIBXML2_LIBS="-lxml2"
	AC_MSG_CHECKING([for libxml2 support])
	AC_MSG_RESULT([yes])
	AC_MSG_NOTICE([headers for libxml2 hopefully in ${use_libxml2}/include/libxml2])
	AC_MSG_NOTICE([library libxml2 hopefully in ${use_libxml2}/lib])
	AC_DEFINE([HAVE_LIB_LIBXML2],
		[],
		[libxml2-based XML backend])
	;;
esac

AC_SUBST(LIBXML2_LIBS)
AC_SUBST(LIBXML2_LDFLAGS)
AC_SUBST(LIBXML2_CFLAGS)


################################
# Check for the roxml library
################################
AC_ARG_WITH(roxml,
	[AS_HELP_STRING([--with-roxml=DIR],
		[Use roxml as the XML parser implementation [default=no]])],
	[use_roxml=$withval],
	[use_roxml=no])

AM_CONDITIONAL(WITH_ROXML,test x$use_roxml != xno)

case x$use_roxml in
xno)
	/bin/true
	;;
xyes)
	# we choose to NOT rely on pkg-config on this one. We may do it as
	# roxml provides a .pc file but some environment (both cross-compile
	# or native environment) may lack this support. The good thing is that
	# it doesn't add much complexity to the configure.ac file (and we
	# may move these tests to another m4 file later).
	# The header is installed in the standard path
	AC_CHECK_HEADER([roxml.h],
		[],
		[AC_MSG_ERROR([cannot find <roxml.h> header file])],
		[/* */])
	# The library is installed in the standard path
	AC_CHECK_LIB([roxml],
		[roxml_load_doc],
		[],
		[AC_MSG_ERROR([cannot find roxml library])],
		[])
	ROXML_CFLAGS=""
	ROXML_LDFLAGS=""
	ROXML_LIBS="-lroxml"
	AC_DEFINE([HAVE_LIB_ROXML],
		[],
		[roxml-based XML backend])
	;;
*)
	# first, we check if we're not looking for an alternate include
	# directory -for example, if the user feeds the script with the
	# option --with-roxml=/usr/local
	# NOTE: we search for include/roxml.h and inc/roxml.h to defeat
	# the caching algorithm of the configure script. If someone knows
	# a better way, please do not hesitate
	saved_CFLAGS="$CFLAGS"
	CFLAGS="$CFLAGS -I${use_roxml}"
	AC_CHECK_HEADER([include/roxml.h],
		[roxml_h_found=yes],
		[roxml_h_found=no],
		[/* */])
	if test "x$roxml_h_found" = "xno"; then
		# the directory might be a source directory, so check
		# if the include file is to be found here
		AC_CHECK_HEADER([inc/roxml.h],
			[roxml_h_found=yes],
			[roxml_h_found=no],
			[/* */])
		if test "x$roxml_h_found" = "xno"; then
			AC_MSG_ERROR([cannot find <roxml.h> header file])
		fi
		ROXML_CFLAGS="-I${use_roxml}/inc"
		AC_MSG_NOTICE([header file <roxml.h> found in ${use_roxml}/inc])
	else
		ROXML_CFLAGS="-I${use_roxml}/include"
		AC_MSG_NOTICE([header file <roxml.h> found in ${use_roxml}/include])
	fi
	CFLAGS="$saved_CFLAGS"
	# we're doing both previous checks, but we are trying to find a library
	# now, so the check themselves are a bit different
	# NOTE: we search for roxml_load_doc and roxml_close to defeat
	# the caching algorithm of the configure script. If someone knows
	# a better way, please do not hesitate.
	saved_LDFLAGS="$LDFLAGS"
	LDFLAGS="$LDFLAGS -L${use_roxml}/lib"
	AC_CHECK_LIB([roxml],
		[roxml_load_doc],
		[roxml_lib_found=yes],
		[roxml_lib_found=no],
		[])
	LDFLAGS="$saved_LDFLAGS"
	if test "x$roxml_lib_found" = "xno"; then
		LDFLAGS="$LDFLAGS -L${use_roxml}"
		AC_CHECK_LIB([roxml],
			[roxml_close],
			[],
			[AC_MSG_ERROR([cannot find roxml library])],
			[])
		LDFLAGS=$saved_LDFLAGS
		ROXML_LDFLAGS="-L${use_roxml}"
		ROXML_LIBS="-lroxml"
		AC_MSG_NOTICE([library roxml found in ${use_roxml}])
	else
		ROXML_LDFLAGS="-L${use_roxml}/lib"
		ROXML_LIBS="-lroxml"
		AC_MSG_NOTICE([library roxml found in ${use_roxml}/lib])
	fi
	AC_DEFINE([HAVE_LIB_ROXML],
		[],
		[roxml-based XML backend])
	;;
esac

AC_SUBST(ROXML_LIBS)
AC_SUBST(ROXML_LDFLAGS)
AC_SUBST(ROXML_CFLAGS)


################################
# Check for the expat library
################################
AC_ARG_WITH(expat,
            [AS_HELP_STRING([--with-expat=DIR],
                            [Use expat as the XML parser implementation [default=no]])],
            [use_expat=$withval],
            [use_expat=no])

AM_CONDITIONAL(WITH_EXPAT,test x$use_expat != xno)

case x$use_expat in
xno)
	/bin/true
	;;
xyes)
	# we choose to NOT rely on pkg-config on this one. Instead, we
	# check for the library and the header file - that should be
	# enough.
	AC_CHECK_HEADER([expat.h],
		[expat_h_found=yes],
		[expat_h_found=no],
		[/* */])
	if test "x$expat_h_found" != "xyes"; then
		AC_CHECK_HEADER([bsdxml.h],
			[expat_h_found=yes],
			[expat_h_found=no],
			[/* */])
		if test "x$expat_h_found" != "xyes"; then
			AC_MSG_ERROR([cannot find <expat.h> header file])
		fi
	fi
	EXPAT_CFLAGS=""
	AC_CHECK_LIB([expat],
		[XML_ParserCreate],
		[expat_lib_found=yes],
		[expat_lib_found=no],
		[])
	if test "x$expat_lib_found" != "xyes"; then
		AC_CHECK_LIB([bsdxml],
			[XML_ParserCreate],
			[],
			[AC_MSG_ERROR([cannot find expat library])],
			[])
		EXPAT_LIBS="-lbsdxml"
		AC_DEFINE([HAVE_LIB_BSDXML],
			[],
			[libbsdxml-based XML backend])
	else
		EXPAT_LIBS="-lexpat"
	fi
	EXPAT_LDFLAGS=""
	AC_DEFINE([HAVE_LIB_EXPAT],
		[],
		[expat-based XML backend])
	;;
*)
	# this is probably broken. We consider that the user supplied path is
	# a non-standard path. But we're not going to check anything.
	AC_MSG_WARN([--with-expat=DIR is probably broken, just trying])
	EXPAT_LDFLAGS="-L${use_expat}/lib"
	EXPAT_CFLAGS="-I${use_expat}/include"
	EXPAT_LIBS="-lexpat"
	AC_MSG_CHECKING([for expat support])
	AC_MSG_RESULT([yes])
	AC_MSG_NOTICE([headers for expat hopefully in ${use_expat}/include])
	AC_MSG_NOTICE([library expat hopefully in ${use_expat}/lib])
	AC_DEFINE([HAVE_LIB_EXPAT],
		[],
		[expat-based XML backend])
	;;
esac

AC_SUBST(EXPAT_LIBS)
AC_SUBST(EXPAT_LDFLAGS)
AC_SUBST(EXPAT_CFLAGS)


################################
# Check for Lua support
################################
AC_ARG_WITH(lua,
	[AS_HELP_STRING([--with-lua=DIR],
		[Build Lua ACTION plugin [default=no]])],
	[use_lua=$withval],
	[use_lua=no])
AM_CONDITIONAL(WITH_LUA,test x$use_lua != xno)

if test x$use_lua != xno; then
	if test x$use_lua != xyes; then
		CPPFLAGS="${CPPFLAGS} -I$use_lua/include"
		LDFLAGS="${LDFLAGS} -L$use_lua/lib"
	fi
	if test x$LUA_VERSION = x; then
		AX_PROG_LUA([5.1])
	fi
	AX_LUA_HEADERS()
	AX_LUA_LIBS()
fi


AC_CONFIG_FILES([Makefile])
AC_OUTPUT
