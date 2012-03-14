AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src
AM_CFLAGS = $(X11_CFLAGS) $(CWARNFLAGS) \
	-DDFLT_XKB_CONFIG_ROOT='"$(XKBCONFIGROOT)"'
AM_YFLAGS = -d

noinst_LTLIBRARIES = libxkbcomp.la
libxkbcomp_la_SOURCES = \
	action.c \
	action.h \
	alias.c \
	alias.h \
	compat.c \
	expr.c \
	expr.h \
	indicators.c \
	indicators.h \
	keycodes.c \
	keycodes.h \
	keymap.c \
	keytypes.c \
	misc.c \
	misc.h \
	parseutils.c \
	parseutils.h \
	symbols.c \
	utils.c \
	utils.h \
	vmod.c \
	vmod.h \
	xkbcomp.c \
	xkbcomp.h \
	xkbparse.y \
	xkbpath.c \
	xkbpath.h \
	xkbscan.l

BUILT_SOURCES = xkbparse.h
