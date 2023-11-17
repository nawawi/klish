lib_LTLIBRARIES += libklish-db-libxml2.la
libklish_db_libxml2_la_SOURCES =
libklish_db_libxml2_la_LDFLAGS = $(AM_LDFLAGS) @LIBXML2_LDFLAGS@
libklish_db_libxml2_la_CFLAGS = $(AM_LDFLAGS) @LIBXML2_CFLAGS@
libklish_db_libxml2_la_LIBADD = libklish-helper-xml.la @LIBXML2_LIBS@

libklish_db_libxml2_la_SOURCES += \
	dbs/libxml2/libxml2_api.c \
	dbs/libxml2/libxml2_plugin.c
