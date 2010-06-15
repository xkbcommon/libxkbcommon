SUBDIRS = makekeys xkbcomp

INCLUDES = -I$(top_srcdir)/include -I$(builddir)/makekeys $(XLIB_CFLAGS)
AM_CFLAGS = $(X11_CFLAGS) $(CWARNFLAGS) $(XMALLOC_ZERO_CFLAGS)

lib_LTLIBRARIES = libxkbcommon.la
libxkbcommon_la_LIBADD = xkbcomp/libxkbcomp.la $(XLIB_LIBS)
libxkbcommon_la_SOURCES = \
	XKBcommonint.h \
	alloc.c \
	atom.c \
	galloc.c \
	geom.c \
	keysym.c \
	malloc.c \
	maprules.c \
	misc.c \
	text.c \
	xkb.c \
	xkballoc.h \
	xkbgeom.h \
	xkbmisc.h \
	xkbrules.h
