lib_LTLIBRARIES += libfaux.la
libfaux_la_SOURCES =
libfaux_la_LIBADD =

nobase_include_HEADERS += \
	faux/faux.h \
	faux/ctype.h \
	faux/str.h \
	faux/sysdb.h \
	faux/conv.h \
	faux/log.h \
	faux/list.h \
	faux/ini.h \
	faux/file.h

#	faux/argv.h \
#	faux/dump.h \
#	faux/system.h

EXTRA_DIST += \
	faux/base/Makefile.am \
	faux/ctype/Makefile.am \
	faux/str/Makefile.am \
	faux/sysdb/Makefile.am \
	faux/conv/Makefile.am \
	faux/log/Makefile.am \
	faux/list/Makefile.am \
	faux/ini/Makefile.am \
	faux/file/Makefile.am

#	faux/argv/Makefile.am \
#	faux/dump/Makefile.am \
#	faux/system/Makefile.am \
#	faux/README

include $(top_srcdir)/faux/base/Makefile.am
include $(top_srcdir)/faux/ctype/Makefile.am
include $(top_srcdir)/faux/str/Makefile.am
include $(top_srcdir)/faux/sysdb/Makefile.am
include $(top_srcdir)/faux/conv/Makefile.am
include $(top_srcdir)/faux/log/Makefile.am
include $(top_srcdir)/faux/list/Makefile.am
include $(top_srcdir)/faux/ini/Makefile.am
include $(top_srcdir)/faux/file/Makefile.am

if TESTC
include $(top_srcdir)/faux/testc_module/Makefile.am
endif
