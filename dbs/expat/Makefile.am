lib_LTLIBRARIES += libklish-db-expat.la
libklish_db_expat_la_SOURCES =
libklish_db_expat_la_LDFLAGS = $(AM_LDFLAGS) @EXPAT_LDFLAGS@
libklish_db_expat_la_CFLAGS = $(AM_LDFLAGS) @EXPAT_CFLAGS@
libklish_db_expat_la_LIBADD = libklish-helper-xml.la @EXPAT_LIBS@

libklish_db_expat_la_SOURCES += \
	dbs/expat/expat_api.c \
	dbs/expat/expat_plugin.c
