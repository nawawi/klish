dbdir = ${pkglibdir}/db
db_LTLIBRARIES =

EXTRA_DIST += \
	dbs/libxml2/Makefile.am \
	dbs/roxml/Makefile.am \
	dbs/expat/Makefile.am

if WITH_LIBXML2
include $(top_srcdir)/dbs/libxml2/Makefile.am
endif
if WITH_ROXML
include $(top_srcdir)/dbs/roxml/Makefile.am
endif
if WITH_EXPAT
include $(top_srcdir)/dbs/expat/Makefile.am
endif
