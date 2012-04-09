#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([MAJOR_VERSION], 1)
m4_define([MINOR_VERSION], 5)
m4_define([MICRO_VERSION], 4)

AC_PREREQ(2.59)
AC_INIT([klish],
        [MAJOR_VERSION.MINOR_VERSION.MICRO_VERSION],
        [serj.kalichev at gmail dot com])

AC_CONFIG_AUX_DIR(aux_scripts)

# Check for system extensions (_POSIX_THREAD_SEMANTICS for Solaris)
AC_USE_SYSTEM_EXTENSIONS

# Checks for programs.
AC_PROG_CC
AC_PROG_CXX
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
# Check for the TINYXML library
################################
TINYXML_LIBS="-ltinyxml"
TINYXML_CXXFLAGS="-std=gnu++98"

if test "x$TINYXML_LIBS" = "x"; then
    AC_MSG_ERROR([Cannot find the "Tiny XML" library])
fi
AC_SUBST(TINYXML_LIBS)
AC_SUBST(TINYXML_CXXFLAGS)

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
