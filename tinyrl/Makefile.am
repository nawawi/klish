lib_LTLIBRARIES += libtinyrl.la
libtinyrl_la_SOURCES =
libtinyrl_la_LDFLAGS = $(AM_LDFLAGS) $(VERSION_INFO)

nobase_include_HEADERS += \
	tinyrl/vt100.h

EXTRA_DIST += \
	tinyrl/vt100/Makefile.am

include $(top_srcdir)/tinyrl/vt100/Makefile.am
