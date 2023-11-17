lib_LTLIBRARIES += libklish-db-roxml.la
libklish_db_roxml_la_SOURCES =
libklish_db_roxml_la_LDFLAGS = $(AM_LDFLAGS) @ROXML_LDFLAGS@
libklish_db_roxml_la_CFLAGS = $(AM_LDFLAGS) @ROXML_CFLAGS@
libklish_db_roxml_la_LIBADD = libklish-helper-xml.la @ROXML_LIBS@

libklish_db_roxml_la_SOURCES += \
	dbs/roxml/roxml_api.c \
	dbs/roxml/roxml_plugin.c
