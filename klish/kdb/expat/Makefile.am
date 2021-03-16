lib_LTLIBRARIES += libkdb-expat.la
libkdb_expat_la_SOURCES =
libkdb_expat_la_LDFLAGS = $(AM_LDFLAGS) $(EXPAT_LDFLAGS) $(VERSION_INFO)
libkdb_expat_la_LIBS = $(EXPAT_LIBS)
libkdb_expat_la_CFLAGS = $(AM_LDFLAGS) $(EXPAT_CFLAGS)

libkdb_expat_la_SOURCES += \
	klish/kdb/xml-common/load.c \
	klish/kdb/expat/expat_api.c
