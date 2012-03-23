SUBDIRS = xkbcomp

AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = $(X11_CFLAGS) $(CWARNFLAGS) $(XMALLOC_ZERO_CFLAGS)

lib_LTLIBRARIES = libxkbcommon.la
libxkbcommon_la_LIBADD = xkbcomp/libxkbcomp.la
libxkbcommon_la_LDFLAGS = -no-undefined
libxkbcommon_la_SOURCES = \
	XKBcommonint.h \
	alloc.c \
	atom.c \
	keysym.c \
	malloc.c \
	map.c \
	maprules.c \
	misc.c \
	state.c \
	text.c \
	utils.c \
	utils.h \
	xkb.c \
	xkballoc.h \
	xkbmisc.h \
	xkbrules.h

BUILT_SOURCES = ks_tables.h
CLEANFILES = $(BUILT_SOURCES)

ks_tables.h: $(KEYSYMDEFS) $(top_builddir)/makekeys/makekeys$(EXEEXT)
	$(top_builddir)/makekeys/makekeys $(KEYSYMDEFS) > $@
