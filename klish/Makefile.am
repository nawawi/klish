lib_LTLIBRARIES += libklish.la
libklish_la_SOURCES =
libklish_la_LDFLAGS = $(AM_LDFLAGS) $(VERSION_INFO)

#if TESTC
#libklish_la_CFLAGS += -DTESTC
#endif

nobase_include_HEADERS += \
	klish/ktp.h \
	klish/ktp_session.h \
	klish/kscheme.h \
	klish/kview.h \
	klish/kcommand.h \
	klish/kparam.h

EXTRA_DIST += \
	klish/ktp/Makefile.am \
	klish/kscheme/Makefile.am

include $(top_srcdir)/klish/ktp/Makefile.am
include $(top_srcdir)/klish/kscheme/Makefile.am

#if TESTC
#include $(top_srcdir)/klish/testc_module/Makefile.am
#endif
