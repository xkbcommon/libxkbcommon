INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src
AM_CFLAGS = $(X11_CFLAGS) $(CWARNFLAGS) \
	-DDFLT_XKB_CONFIG_ROOT='"$(XKBCONFIGROOT)"'

BUILT_SOURCES = xkbparse.c
MAINTAINERCLEANFILES = $(BUILT_SOURCES)

noinst_LTLIBRARIES = libxkbcomp.la
libxkbcomp_la_SOURCES = \
	action.c \
	action.h \
	alias.c \
	alias.h \
	compat.c \
	compat.h \
	expr.c \
	expr.h \
	geometry.c \
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
	tokens.h \
	utils.c \
	utils.h \
	vmod.c \
	vmod.h \
	xkbcomp.c \
	xkbcomp.h \
	xkbparse.y \
	xkbpath.c \
	xkbpath.h \
	xkbscan.c
