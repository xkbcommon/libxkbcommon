ACLOCAL_AMFLAGS = -I m4

SUBDIRS = . test

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xkbcommon.pc

MAINTAINERCLEANFILES = ChangeLog INSTALL

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL

AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src
AM_CFLAGS = \
	$(X11_CFLAGS) \
	$(XORG_COMPILER_FLAGS) \
	$(XMALLOC_ZERO_CFLAGS) \
	-DDFLT_XKB_CONFIG_ROOT='"$(XKBCONFIGROOT)"'
AM_YFLAGS = -d

xkbcommonincludedir = $(includedir)/xkbcommon
xkbcommoninclude_HEADERS = include/xkbcommon/xkbcommon.h

lib_LTLIBRARIES = libxkbcommon.la
libxkbcommon_la_LDFLAGS = -no-undefined
libxkbcommon_la_SOURCES = \
	src/xkbcomp/action.c \
	src/xkbcomp/action.h \
	src/xkbcomp/alias.c \
	src/xkbcomp/alias.h \
	src/xkbcomp/compat.c \
	src/xkbcomp/expr.c \
	src/xkbcomp/expr.h \
	src/xkbcomp/indicators.c \
	src/xkbcomp/indicators.h \
	src/xkbcomp/keycodes.c \
	src/xkbcomp/keycodes.h \
	src/xkbcomp/keymap.c \
	src/xkbcomp/keytypes.c \
	src/xkbcomp/misc.c \
	src/xkbcomp/misc.h \
	src/xkbcomp/parseutils.c \
	src/xkbcomp/parseutils.h \
	src/xkbcomp/symbols.c \
	src/xkbcomp/vmod.c \
	src/xkbcomp/vmod.h \
	src/xkbcomp/xkbcomp.c \
	src/xkbcomp/xkbcomp.h \
	src/xkbcomp/xkbparse.y \
	src/xkbcomp/xkbpath.c \
	src/xkbcomp/xkbpath.h \
	src/xkbcomp/xkbscan.l \
	src/alloc.c \
	src/atom.c \
	src/keysym.c \
	src/malloc.c \
	src/map.c \
	src/maprules.c \
	src/misc.c \
	src/state.c \
	src/text.c \
	src/utils.c \
	src/utils.h \
	src/xkb.c \
	src/xkballoc.h \
	src/xkbmisc.h \
	src/xkbrules.h \
	src/XKBcommonint.h \
	include/xkbcommon/xkbcommon.h

BUILT_SOURCES = src/xkbcomp/xkbparse.h src/ks_tables.h
CLEANFILES = src/ks_tables.h

noinst_PROGRAMS = makekeys/makekeys
makekeys_makekeys_SOURCES = makekeys/makekeys.c
makekeys_makekeys_CC = $(CC_FOR_BUILD)
makekeys_makekeys_CPPFLAGS = $(AM_CPPFLAGS) $(CPPFLAGS_FOR_BUILD)
makekeys_makekeys_CFLAGS = $(AM_CFLAGS) $(CFLAGS_FOR_BUILD)
makekeys_makekeys_LDFLAGS = $(AM_LDFLAGS) $(LDFLAGS_FOR_BUILD)

src/ks_tables.h: $(KEYSYMDEFS) $(top_builddir)/makekeys/makekeys$(EXEEXT)
	$(top_builddir)/makekeys/makekeys $(KEYSYMDEFS) > $@
