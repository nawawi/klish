lib_LTLIBRARIES += libklish.la
libklish_la_SOURCES =
libklish_la_LDFLAGS = $(AM_LDFLAGS) $(VERSION_INFO)

#if TESTC
#libklish_la_CFLAGS += -DTESTC
#endif

nobase_include_HEADERS += \
	klish/ktp.h

EXTRA_DIST += \
	klish/ktp/Makefile.am

include $(top_srcdir)/klish/ktp/Makefile.am

#if TESTC
#include $(top_srcdir)/klish/testc_module/Makefile.am
#endif
