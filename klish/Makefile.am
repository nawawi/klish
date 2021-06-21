lib_LTLIBRARIES += libklish.la
libklish_la_SOURCES =
libklish_la_LDFLAGS = $(AM_LDFLAGS) $(VERSION_INFO)

# Add RPATH for kdb
libklish_la_LDFLAGS += '-Wl,-rpath,$(pkglibdir)/@DBS_SUBDIR@:$$ORIGIN/../lib/@PACKAGE@/@DBS_SUBDIR@'

# Add RPATH for plugins
libklish_la_LDFLAGS += '-Wl,-rpath,$(pkglibdir)/@PLUGINS_SUBDIR@:$$ORIGIN/../lib/@PACKAGE@/@PLUGINS_SUBDIR@'

#if TESTC
#libklish_la_CFLAGS += -DTESTC
#endif

# Common
nobase_include_HEADERS += \
	klish/khelper.h

# KTP
nobase_include_HEADERS += \
	klish/ktp.h \
	klish/ktp_session.h

# Scheme
nobase_include_HEADERS += \
	klish/kscheme.h \
	klish/kentry.h \
	klish/kview.h \
	klish/kcommand.h \
	klish/kparam.h \
	klish/kplugin.h \
	klish/kaction.h \
	klish/kptype.h \
	klish/ksym.h \
	klish/knspace.h \
	klish/kdb.h

# iScheme
nobase_include_HEADERS += \
	klish/ischeme.h \
	klish/iview.h \
	klish/icommand.h \
	klish/iparam.h \
	klish/iplugin.h \
	klish/iaction.h \
	klish/iptype.h \
	klish/inspace.h \
	klish/ientry.h

# Session
nobase_include_HEADERS += \
	klish/kudata.h \
	klish/kustore.h \
	klish/kcontext.h \
	klish/kpath.h \
	klish/kexec.h \
	klish/kpargv.h \
	klish/ksession.h

# XML-helper
nobase_include_HEADERS += \
	klish/kxml.h

#noinst_HEADERS += \
#	klish/khelper.h

EXTRA_DIST += \
	klish/ktp/Makefile.am \
	klish/kscheme/Makefile.am \
	klish/ischeme/Makefile.am \
	klish/ksession/Makefile.am \
	klish/xml-helper/Makefile.am

include $(top_srcdir)/klish/ktp/Makefile.am
include $(top_srcdir)/klish/kscheme/Makefile.am
include $(top_srcdir)/klish/ischeme/Makefile.am
include $(top_srcdir)/klish/ksession/Makefile.am
include $(top_srcdir)/klish/xml-helper/Makefile.am

#if TESTC
#include $(top_srcdir)/klish/testc_module/Makefile.am
#endif
