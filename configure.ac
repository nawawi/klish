#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
m4_define([MAJOR_VERSION], 1)
m4_define([MINOR_VERSION], 5)
m4_define([MICRO_VERSION], 3)

AC_PREREQ(2.59)
AC_INIT([klish],
        [MAJOR_VERSION.MINOR_VERSION.MICRO_VERSION],
        [serj.kalichev at gmail dot com])

AC_CONFIG_AUX_DIR(aux_scripts)

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
# Check for the PTHREAD library
################################
AC_ARG_WITH(pthread,
            [AS_HELP_STRING([--with-pthread=DIR],
                            [Use POSIX threads library distribution in DIR])
            ])

AC_CHECK_HEADERS(pthread.h,
                 [AC_CHECK_LIB(pthread, 
                              pthread_create, 
                              [AC_DEFINE([HAVE_LIBPTHREAD], [], [Have POSIX threads library])
                                if test "x$with_pthread" = "x"; then
                                    PTHREAD_LIBS="-lpthread"
                                else
                                    PTHREAD_CFLAGS="-I$with_pthread/include"
                                    PTHREAD_LIBS="-L$with_pthread/lib -lpthread"
                                fi
                                AC_SUBST(PTHREAD_LIBS)
                                AC_SUBST(PTHREAD_CFLAGS)
                              ])
                 ])

if test "x$PTHREAD_LIBS" = "x"; then
    AC_MSG_ERROR([Cannot find the POSIX threads library])
fi

################################
# Check for the RT library
################################
AC_ARG_WITH(rt,
            [AS_HELP_STRING([--with-rt=DIR],
                            [Use POSIX real time library distribution in DIR])])

AC_CHECK_LIB(rt, 
             clock_gettime, 
             [
                AC_DEFINE([HAVE_LIBRT], [], [Have POSIX real time library])
                if test "x$with_rt" = "x"; then
                    RT_LIBS="-lrt"
                else
                    RT_CFLAGS="-I$with_rt/include"
                    RT_LIBS="-L$with_rt/lib -lrt"
                fi
                AC_SUBST(RT_LIBS)
                AC_SUBST(RT_CFLAGS)
             ])

################################
# Check for the TCL library
################################
TCL_CONFIG=${libdir}/tclConfig.sh

AC_ARG_WITH(tcl,
            [AS_HELP_STRING([--with-tcl=DIR],
                            [Use TCL library distribution in DIR])],
if test -e $TCL_CONFIG; then
    source ${TCL_CONFIG} # get TCL_VERSION
    if test "${with_tcl}" = "yes"; then
        TCL_CFLAGS="${TCL_INCLUDE_SPEC}"
        TCL_LIBS="${TCL_LIB_SPEC}"
    else
        TCL_CFLAGS="-I${with_tcl}/include"
        TCL_LIBS="-L${with_tcl}/lib -ltcl"
    fi
    CPPFLAGS="$CPPFLAGS $TCL_CFLAGS"
    AC_CHECK_HEADERS(tcl.h,
        AC_CHECK_LIB(tcl${TCL_VERSION}, Tcl_CreateInterp, [
            AC_DEFINE([HAVE_LIBTCL], [], [Have TCL library])
            AC_SUBST(TCL_CFLAGS)
            AC_SUBST(TCL_LIBS)
            AC_SUBST(TCL_VERSION)
            have_tcl="yes"
        ])
    )
fi
)
AM_CONDITIONAL(TCL,test "x${have_tcl}" = "xyes")

################################
# Check for inclusion of lub_heap
################################
AC_ARG_ENABLE(lubheap,
    [AS_HELP_STRING([--enable-lubheap],
                    [Replace standard memory libaries with lubheap (EXPERIMENTAL) [default=no]])],
    [],
    [enable_lubheap=no])

AM_CONDITIONAL(LUBHEAP,test "x$enable_lubheap" = "xyes")

if test "x$enable_lubheap" = "xyes"; then
    AC_MSG_WARN([Replacing standard memory libraries with lubheap])

    LUBHEAP_LIBS="-llubheap"
    LUBHEAP_CFLAGS=""

    if test "x$LUBHEAP_LIBS" = "x"; then
        AC_MSG_ERROR([Cannot find the "Little Useful Bits Heap" library])
    fi
    AC_SUBST(LUBHEAP_LIBS)
    AC_SUBST(LUBHEAP_CFLAGS)
fi

################################
# Check for linkage with GPL libraries
################################
AC_ARG_ENABLE(gpl,
    [AS_HELP_STRING([--enable-gpl],
                    [Link with GPL licenced libraries [default=yes]])],
    [],
    [enable_gpl=yes])

if test "x$enable_gpl" = "xyes"; then
    ################################
    # Check for BFD library
    ################################
    AC_MSG_WARN([Linking with GPL licence libraries])
    AC_CHECK_HEADERS(bfd.h,
        [AC_CHECK_LIB(bfd, 
                      bfd_init,
                      [AC_DEFINE([HAVE_LIBBFD], 
                                 [], 
                                 [Have Binary File Decription library])
                       BFD_LIBS="-lbfd -liberty"
                       AC_CHECK_LIB(intl, 
                                    libintl_dgettext, 
                                    [AC_DEFINE([HAVE_LIBINTL], 
                                               [], 
                                               [Have native langauage support library])
                                     BFD_LIBS="$BFD_LIBS -lintl"
                                    ])
                       AC_SUBST(BFD_LIBS)
                      ])
        ])
else
    AC_MSG_WARN([Skipping BFD library checks because of GPL Licence])
fi

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
