#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([MAJOR_VERSION], 1)
m4_define([MINOR_VERSION], 6)
m4_define([MICRO_VERSION], 0)

AC_PREREQ(2.59)
AC_INIT([klish],
        [MAJOR_VERSION.MINOR_VERSION.MICRO_VERSION],
        [serj.kalichev at gmail dot com])

AC_CONFIG_AUX_DIR(aux_scripts)

# Check for system extensions (_POSIX_THREAD_SEMANTICS for Solaris)
AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CC
AC_LIBTOOL_WIN32_DLL
AC_PROG_LIBTOOL

AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE(subdir-objects)
AM_PROG_CC_C_O

# needed to handle 64-bit architecture
AC_CHECK_SIZEOF(int)
AC_CHECK_SIZEOF(long)
AC_CHECK_SIZEOF(size_t)

#------------------------------------------------------------------------
# Handle the --prefix=... option
#------------------------------------------------------------------------

if test "${prefix}" = "NONE"; then
    prefix=/usr/local
fi
if test "${exec_prefix}" = "NONE"; then
    exec_prefix=$prefix
fi
# libdir must be a fully qualified path (not ${exec_prefix}/lib)
eval libdir="$libdir"

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
# Check for the roxml library
################################
AC_ARG_WITH(roxml,
            [AS_HELP_STRING([--with-roxml=DIR],
                            [Use roxml as the XML parser implementation [default=no]])],
            [use_roxml=$withval],
            [use_roxml=no])

AC_ARG_WITH(libxml2,
            [AS_HELP_STRING([--with-libxml2=DIR],
                            [Use libxml2 as the XML parser implementation [default=yes]])],
            [use_libxml2=$withval],
            [use_libxml2=yes])

if test "x$use_roxml" = "xyes"; then
	if test "x$use_libxml2" = "xyes"; then
		AC_MSG_WARN([both roxml and libxml2 are selected. I choose roxml])
		use_libxml2=no
	fi
fi

XML_LDFLAGS=""
XML_CFLAGS=""
XML_LIBS=""

AC_MSG_CHECKING([for libroxml support])
case x$use_roxml in
  xno)
    AC_MSG_RESULT([no])
    ;;
  xyes)
    # the library is installed in the standard path
    XML_LDFLAGS="`pkg-config libroxml --libs-only-L`"
    XML_CFLAGS="`pkg-config libroxml --cflags`"
    XML_LIBS="`pkg-config libroxml --libs-only-l`"
    AC_MSG_RESULT([yes])
    AC_DEFINE([HAVE_LIB_ROXML], 
              [], 
              [Define to 1 if you want to use libroxml to parse clish XML files])
    ;;
  *)
    # this is very likely to be a source directory, so
    # let's consider that it's a valid one
    XML_LDFLAGS="-L${use_roxml}"
    XML_CFLAGS="-I${use_roxml}/inc"
    XML_LIBS="-lroxml"
    AC_MSG_RESULT([yes (lib in ${use_roxml}, includes in ${use_roxml}/inc)])
    AC_DEFINE([HAVE_LIB_ROXML], 
              [], 
              [Define to 1 if you want to use libroxml to parse clish XML files])
    ;;
esac

AC_MSG_CHECKING([for libxml2 support])
case x$use_libxml2 in
  xno)
    AC_MSG_RESULT([no])
    ;;
  xyes)
    # the library is installed in the standard path
    XML_LDFLAGS="`pkg-config libxml-2.0 --libs-only-L`"
    XML_CFLAGS="`pkg-config libxml-2.0 --cflags`"
    XML_LIBS="`pkg-config libxml-2.0 --libs-only-l`"
    AC_MSG_RESULT([yes])
    AC_DEFINE([HAVE_LIB_LIBXML2], 
              [], 
              [Define to 1 if you want to use libxml2 to parse clish XML files])
    ;;
  *)
    # this is very likely to be a source directory, so
    # let's consider that it's a valid one
    XML_LDFLAGS="-L${use_libxml2}"
    XML_CFLAGS="-I${use_libxml2}"
    XML_LIBS="-lxml2"
    AC_MSG_RESULT([yes (lib and includes in ${use_libxml2}])
    AC_DEFINE([HAVE_LIB_LIBXML2], 
              [], 
              [Define to 1 if you want to use libxml2 to parse clish XML files])
    ;;
esac

AC_SUBST(XML_LIBS)
AC_SUBST(XML_LDFLAGS)
AC_SUBST(XML_CFLAGS)

################################
# Check for the LUB library
################################
LUB_LIBS="-llub"
LUB_CFLAGS=""

if test "x$LUB_LIBS" = "x"; then
    AC_MSG_ERROR([Cannot find the "Little Useful Bits" library])
fi
AC_SUBST(LUB_LIBS)
AC_SUBST(LUB_CFLAGS)

################################
# Check for the KONF library
################################
KONF_LIBS="-lkonf"
KONF_CFLAGS=""

if test "x$KONF_LIBS" = "x"; then
    AC_MSG_ERROR([Cannot find the "CLI config" library])
fi
AC_SUBST(KONF_LIBS)
AC_SUBST(KONF_CFLAGS)

################################
# Check for the TINYRL library
################################
TINYRL_LIBS="-ltinyrl"
TINYRL_CFLAGS=""

if test "x$TINYRL_LIBS" = "x"; then
    AC_MSG_ERROR([Cannot find the "Tiny Readline" library])
fi
AC_SUBST(TINYRL_LIBS)
AC_SUBST(TINYRL_CFLAGS)

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
# Check for getopt_long()
################################
AC_CHECK_HEADERS(getopt.h, [],
    AC_MSG_WARN([getopt.h not found: only short parameters can be used on command line]))

################################
# Check for locale.h
################################
AC_CHECK_HEADERS(locale.h, [],
    AC_MSG_WARN([locale.h not found: the locales is not supported]))

################################
# Check for CODESET within nl_langinfo
################################
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

AC_CONFIG_FILES(Makefile)
AC_OUTPUT
