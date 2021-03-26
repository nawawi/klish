dbdir = ${pkglibdir}/db
db_LTLIBRARIES =

EXTRA_DIST += \
	db/libxml2/Makefile.am \
	db/roxml/Makefile.am \
	db/expat/Makefile.am

if WITH_LIBXML2
include $(top_srcdir)/db/libxml2/Makefile.am
endif
if WITH_ROXML
include $(top_srcdir)/db/roxml/Makefile.am
endif
if WITH_EXPAT
include $(top_srcdir)/db/expat/Makefile.am
endif
