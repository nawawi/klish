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

if test "x$use_roxml" != "xno"; then
    if test "x$use_libxml2" != "xno"; then
        AC_MSG_WARN([both roxml and libxml2 are selected. I choose roxml])
        use_libxml2=no
    fi
else
    if test "x$use_libxml2" = "xno"; then
        AC_MSG_WARN([neither roxml nor libxml2 are selected. I choose libxml2])
        use_libxml2=yes
    fi
fi

XML_LDFLAGS=""
XML_CFLAGS=""
XML_LIBS=""

case x$use_roxml in
  xno)
    AC_MSG_CHECKING([for libroxml support])
    AC_MSG_RESULT([no])
    ;;
  xyes)
    # we choose to NOT rely on pkg-config on this one. We may do it as
    # libroxml provides a .pc file but some environment (both cross-compile
    # or native environment) may lack this support. The good thing is that
    # it doesn't add much complexity to the configure.ac file (and we
    # may move these tests to another m4 file later). 

    # the header is installed in the standard path
    AC_CHECK_HEADER([roxml.h],
                    [roxml_h_found=yes],
                    [roxml_h_found=no],
                    [/* force include check */])
    if test "x$roxml_h_found" != "xyes"; then
        AC_MSG_ERROR([cannot find <roxml.h> header file])
    fi
    XML_CFLAGS=""

    # the library is installed in the standard path
    AC_CHECK_LIB([roxml],
                 [roxml_load_doc],
                 [roxml_lib_found=yes],
                 [roxml_lib_found=no],
                 [])
    if test "x$roxml_lib_found" != "xyes"; then
        AC_MSG_ERROR([cannot find roxml library])
    fi
    XML_LDFLAGS=""
    XML_LIBS="-lroxml"

    AC_DEFINE([HAVE_LIB_ROXML], 
              [], 
              [Define to 1 if you want to use libroxml to parse clish XML files])

    ;;

  *)
    # first, we check if we're not looking for an alternate include
    # directory -for example, if the user feeds the script with the
    # option --with-roxml=/usr/local
    # NOTE: we search for include/roxml.h and inc/roxml.h to defeat
    # the caching algorithm of the configure script. If someone knows
    # a better way, please do not hesitate
    roxml_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS -I${use_roxml}"
    AC_CHECK_HEADER([include/roxml.h],
                    [roxml_h_found=yes],
                    [roxml_h_found=no],
                    [/* force include check */])
    if test "x$roxml_h_found" = "xno"; then
        # the directory might be a source directory, so check
        # if the include file is to be found here
        AC_CHECK_HEADER([inc/roxml.h],
                        [roxml_h_found=yes],
                        [roxml_h_found=no],
                        [/* force include check */])
        if test "x$roxml_h_found" = "xno"; then
            AC_MSG_ERROR([cannot find <roxml.h> header file])
        fi
        XML_CFLAGS="-I${use_roxml}/inc"
        AC_MSG_NOTICE([header file <roxml.h> found in ${use_roxml}/inc])
    else
        XML_CFLAGS="-I${use_roxml}/include"
        AC_MSG_NOTICE([header file <roxml.h> found in ${use_roxml}/include])
    fi
    CFLAGS="$roxml_CFLAGS"

    # we're doing both previous checks, but we are trying to find a library
    # now, so the check themselves are a bit different
    # NOTE: we search for roxml_load_doc and roxml_close to defeat
    # the caching algorithm of the configure script. If someone knows
    # a better way, please do not hesitate.
    roxml_LDFLAGS="$LDFLAGS"
    LDFLAGS="$LDFLAGS -L${use_roxml}/lib"
    AC_CHECK_LIB([roxml],
                 [roxml_load_doc],
                 [roxml_lib_found=yes],
                 [roxml_lib_found=no],
                 [])
    LDFLAGS=$roxml_LDFLAGS
    if test "x$roxml_lib_found" = "xno"; then
        LDFLAGS="$LDFLAGS -L${use_roxml}"
        AC_CHECK_LIB([roxml],
                     [roxml_close],
                     [roxml_lib_found=yes],
                     [roxml_lib_found=no],
                     [])
        LDFLAGS=$roxml_LDFLAGS
        if test "x$roxml_lib_found" = "xno"; then
            AC_MSG_ERROR([cannot find roxml library])
        fi
        XML_LDFLAGS="-L${use_roxml}"
        XML_LIBS="-lroxml"
        AC_MSG_NOTICE([library libroxml found in ${use_roxml}])
    else
        XML_LDFLAGS="-L${use_roxml}/lib"
        XML_LIBS="-lroxml"
        AC_MSG_NOTICE([library libroxml found in ${use_roxml}/lib])
    fi

    AC_DEFINE([HAVE_LIB_ROXML], 
              [], 
              [Define to 1 if you want to use libroxml to parse clish XML files])
    ;;
esac

case x$use_libxml2 in
  xno)
    AC_MSG_CHECKING([for libxml2 support])
    AC_MSG_RESULT([no])
    ;;
  xyes)
    # I would love to avoid using pkg-config (which may not be available on
    # some compilation environment) but doing so really add a lot of complexity
    # to the system, as the headers don't lie in a standard directory (they
    # lie in a subdirectory of a standard include directory; not the same thing
    # for configure scripts). 
    XML_CFLAGS="`pkg-config libxml-2.0 --cflags`"
    XML_LDFLAGS="`pkg-config libxml-2.0 --libs-only-L`"
    XML_LIBS="`pkg-config libxml-2.0 --libs-only-l`"
    AC_CHECK_LIB([xml2],
                 [xmlNewDoc],
                 [libxml2_lib_found=yes],
                 [libxml2_lib_found=no],
                 [-lz])
    if test "x$libxml2_lib_found" != "xyes"; then
        AC_MSG_ERROR([cannot find libxml2 library])
    fi

    # the header file is installed in a subdirectory of one of the standard
    # include directory. This might prove to be a problem if the cross-compile
    # environment is not complete enough (i.e. if it misses pkg-config, or
    # if pkg-config returns wrong values). In most cases, the environment is
    # likely to be OK so we will never hit any issue. 
    xml2_CFLAGS="$CFLAGS"
    CFLAGS="$CFLAGS $XML_CFLAGS"
    AC_CHECK_HEADER([libxml/tree.h],
                    [libxml2_h_found=yes],
                    [libxml2_h_found=no],
                    [/* force include check */])
    CFLAGS="$xml2_CFLAGS"
    if test "x$libxml2_h_found" != "xyes"; then
        AC_MSG_ERROR([cannot find libxml2 headers])
    fi

    AC_DEFINE([HAVE_LIB_LIBXML2], 
              [], 
              [Define to 1 if you want to use libxml2 to parse clish XML files])
    ;;
  *)
    # this is probably broken. We consider that the user supplied path is
    # a non-standard path. But we're not going to check anything.
    AC_MSG_WARN([--with-libxml2=DIR is probably broken, but let's try])
    XML_LDFLAGS="-L${use_libxml2}/include/libxml2"
    XML_CFLAGS="-I${use_libxml2}/lib"
    XML_LIBS="-lxml2"
    AC_MSG_CHECKING([for libxml2 support])
    AC_MSG_RESULT([yes])
    AC_MSG_NOTICE([headers for libxml2 hopefully in ${use_libxml2}/include/libxml2])
    AC_MSG_NOTICE([library libxml2 hopefully in ${use_libxml2}/lib])
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
