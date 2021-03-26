db_LTLIBRARIES += kdb-libxml2.la
kdb_libxml2_la_SOURCES =
kdb_libxml2_la_LDFLAGS = $(AM_LDFLAGS) $(LIBXML2_LDFLAGS) -avoid-version -module
kdb_libxml2_la_LIBS = $(LIBXML2_LIBS)
kdb_libxml2_la_CFLAGS = $(AM_LDFLAGS) $(LIBXML2_CFLAGS)
kdb_libxml2_la_LIBADD = libklish-helper-xml.la

kdb_libxml2_la_SOURCES += \
	dbs/libxml2/libxml2_api.c \
	dbs/libxml2/libxml2_plugin.c
