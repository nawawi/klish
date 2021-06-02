db_LTLIBRARIES += kdb-expat.la
kdb_expat_la_SOURCES =
kdb_expat_la_LDFLAGS = $(AM_LDFLAGS) $(EXPAT_LDFLAGS) -avoid-version -module
kdb_expat_la_LIBS = $(EXPAT_LIBS)
kdb_expat_la_CFLAGS = $(AM_LDFLAGS) $(EXPAT_CFLAGS)
kdb_expat_la_LIBADD = libklish-helper-xml.la

kdb_expat_la_SOURCES += \
	dbs/expat/expat_api.c \
	dbs/expat/expat_plugin.c
