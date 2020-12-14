## Process this file with automake to generate Makefile.in
AUTOMAKE_OPTIONS = foreign nostdinc
ACLOCAL_AMFLAGS = -I m4

AM_CPPFLAGS = -I. -I$(top_srcdir)
AM_LD = $(CC)
VERSION_INFO = -version-info $(SONAME_CURRENT):$(SONAME_REVISION):$(SONAME_AGE)

if DEBUG
DEBUG_CFLAGS = -DDEBUG
endif

if TESTC
TESTC_CFLAGS = -DTESTC
endif

AM_CFLAGS = -Wall $(DEBUG_CFLAGS) $(TESTC_CFLAGS)

bin_PROGRAMS =
lib_LTLIBRARIES =
lib_LIBRARIES =
nobase_include_HEADERS =

EXTRA_DIST =
	klish/Makefile.am \
	bin/Makefile.am

#	bin/module.am \
#	clish/module.am \
#	tinyrl/module.am \
#	konf/module.am \
#	plugins/module.am \
#	libc/module.am \
#	doc/module.am \
#	contrib \
#	xml-examples \
#	debian \
#	clish.xsd \
#	LICENCE \
#	README \
#	CHANGES \
#	ISSUES

include $(top_srcdir)/klish/Makefile.am
include $(top_srcdir)/bin/Makefile.am

#include $(top_srcdir)/tinyrl/module.am
#include $(top_srcdir)/konf/module.am
#include $(top_srcdir)/clish/module.am
#include $(top_srcdir)/bin/module.am
#include $(top_srcdir)/plugins/module.am
#include $(top_srcdir)/libc/module.am
#include $(top_srcdir)/doc/module.am
