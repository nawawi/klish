lib_LTLIBRARIES += libklish-helper-xml.la
libklish_helper_xml_la_SOURCES = \
	klish/kdb/xml-common/load.c
libklish_helper_xml_la_CFLAGS = $(AM_CFLAGS) -fPIC
libklish_helper_xml_la_LDFLAGS = -static

dbdir = ${pkglibdir}/db
db_LTLIBRARIES =

EXTRA_DIST += \
	klish/kdb/libxml2/Makefile.am \
	klish/kdb/roxml/Makefile.am \
	klish/kdb/expat/Makefile.am

if WITH_LIBXML2
include $(top_srcdir)/klish/kdb/libxml2/Makefile.am
endif
if WITH_ROXML
include $(top_srcdir)/klish/kdb/roxml/Makefile.am
endif
if WITH_EXPAT
include $(top_srcdir)/klish/kdb/expat/Makefile.am
endif
