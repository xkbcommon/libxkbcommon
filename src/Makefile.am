INCLUDES = -I$(top_srcdir)/include

noinst_PROGRAMS = makekeys
makekeys_CFLAGS = $(X11_CFLAGS)
makekeys: CC = $(CC_FOR_BUILD)

BUILT_SOURCES = ks_tables.h
ks_tables.h: makekeys
	@rm -f $@
	cat $(KEYSYMDEF_H) $(XF86KEYSYM_H) | $(builddir)/makekeys$(EXEEXT) >$@

lib_LTLIBRARIES = libxkbcommon.la
libxkbcommon_la_CFLAGS = $(X11_CFLAGS)
libxkbcommon_la_SOURCES = \
	ks_tables.h \
	keysym.c
