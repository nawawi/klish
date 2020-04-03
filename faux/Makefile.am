lib_LTLIBRARIES += libfaux.la
libfaux_la_SOURCES =
libfaux_la_LIBADD =

nobase_include_HEADERS += \
	faux/types.h \
	faux/ctype.h \
	faux/str.h

#	faux/argv.h \
#	faux/list.h \
#	faux/c_decl.h \
#	faux/dump.h \
#	faux/system.h \
#	faux/db.h \
#	faux/ini.h \
#	faux/log.h \
#	faux/conv.h

EXTRA_DIST += \
	faux/ctype/Makefile.am \
	faux/str/Makefile.am

#	faux/argv/module.am \
#	faux/list/module.am \
#	faux/dump/module.am \
#	faux/system/module.am \
#	faux/db/module.am \
#	faux/ini/module.am \
#	faux/log/module.am \
#	faux/conv/module.am \
#	faux/README

include $(top_srcdir)/faux/ctype/Makefile.am
include $(top_srcdir)/faux/str/Makefile.am

#include $(top_srcdir)/faux/argv/module.am
#include $(top_srcdir)/faux/list/module.am
#include $(top_srcdir)/faux/dump/module.am
#include $(top_srcdir)/faux/system/module.am
#include $(top_srcdir)/faux/db/module.am
#include $(top_srcdir)/faux/ini/module.am
#include $(top_srcdir)/faux/log/module.am
#include $(top_srcdir)/faux/conv/module.am
