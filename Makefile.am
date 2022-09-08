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

AM_CFLAGS = -Wall -Wextra $(DEBUG_CFLAGS) $(TESTC_CFLAGS)
AM_LDFLAGS = -z relro -z now -z defs

bin_PROGRAMS =
lib_LTLIBRARIES =
lib_LIBRARIES =
nobase_include_HEADERS =
noinst_HEADERS =

EXTRA_DIST =
	bin/Makefile.am \
	dbs/Makefile.am \
	docs/Makefile.am \
	examples/Makefile.am \
	klish/Makefile.am \
	tinyrl/Makefile.am \
	plugins/Makefile.am \
	klish.xsd \
	LICENCE \
	README.md

include $(top_srcdir)/bin/Makefile.am
include $(top_srcdir)/dbs/Makefile.am
include $(top_srcdir)/docs/Makefile.am
include $(top_srcdir)/examples/Makefile.am
include $(top_srcdir)/klish/Makefile.am
include $(top_srcdir)/tinyrl/Makefile.am
include $(top_srcdir)/plugins/Makefile.am

define CONTROL
PACKAGE: klish
Version: $(PACKAGE_VERSION)
Architecture: amd64
Depends: faux
Maintainer: pkun
Description: The klish is a framework for implementing a CISCO-like CLI on a UNIX systems. It is configurable by XML files
Homepage: http://klish.libcode.org
endef

export CONTROL
deb: all
	mkdir -p deb/DEBIAN
	echo "$$CONTROL" > deb/DEBIAN/control
	DESTDIR=$(shell pwd)/deb make install
	dpkg-deb --build deb/ klish_$(PACKAGE_VERSION).deb
