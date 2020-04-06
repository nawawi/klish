lib_LTLIBRARIES += libfaux.la
libfaux_la_SOURCES =
libfaux_la_LIBADD =

nobase_include_HEADERS += \
	faux/types.h \
	faux/ctype.h \
	faux/str.h \
	faux/sysdb.h \
	faux/conv.h \
	faux/log.h \
	faux/list.h

#	faux/argv.h \
#	faux/dump.h \
#	faux/system.h \
#	faux/ini.h

EXTRA_DIST += \
	faux/ctype/Makefile.am \
	faux/str/Makefile.am \
	faux/sysdb/Makefile.am \
	faux/conv/Makefile.am \
	faux/log/module.am \
	faux/list/module.am

#	faux/argv/module.am \
#	faux/dump/module.am \
#	faux/system/module.am \
#	faux/ini/module.am \
#	faux/log/module.am \
#	faux/README

include $(top_srcdir)/faux/ctype/Makefile.am
include $(top_srcdir)/faux/str/Makefile.am
include $(top_srcdir)/faux/sysdb/Makefile.am
include $(top_srcdir)/faux/conv/Makefile.am
include $(top_srcdir)/faux/log/Makefile.am
include $(top_srcdir)/faux/list/Makefile.am

#include $(top_srcdir)/faux/argv/module.am
#include $(top_srcdir)/faux/dump/module.am
#include $(top_srcdir)/faux/system/module.am
#include $(top_srcdir)/faux/ini/module.am
