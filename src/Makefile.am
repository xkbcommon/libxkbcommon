SUBDIRS = makekeys

INCLUDES = -I$(top_srcdir)/include

BUILT_SOURCES = ks_tables.h
ks_tables.h: $(builddir)/makekeys/makekeys$(EXEEXT)
	@rm -f $@
	cat $(KEYSYMDEF_H) $(XF86KEYSYM_H) | \
	  $(builddir)/makekeys/makekeys$(EXEEXT) >$@

$(builddir)/makekeys/makekeys$(EXEEXT):
	@cd makekeys && $(MAKE) $(AM_MAKEFLAGS)

lib_LTLIBRARIES = libxkbcommon.la
libxkbcommon_la_CFLAGS = $(X11_CFLAGS)
libxkbcommon_la_SOURCES = \
	ks_tables.h \
	keysym.c
