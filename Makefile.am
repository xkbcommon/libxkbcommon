ACLOCAL_AMFLAGS = -I m4

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xkbcommon.pc

MAINTAINERCLEANFILES = ChangeLog INSTALL

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL

AM_CPPFLAGS = \
	-DDFLT_XKB_CONFIG_ROOT='"$(XKBCONFIGROOT)"' \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/xkbcomp \
	-include $(top_srcdir)/src/config.h

AM_CFLAGS = \
	$(BASE_CFLAGS) \
	$(XMALLOC_ZERO_CFLAGS)

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
	src/xkbcomp/parser.y \
	src/xkbcomp/parseutils.c \
	src/xkbcomp/parseutils.h \
	src/xkbcomp/path.c \
	src/xkbcomp/scanner.l \
	src/xkbcomp/symbols.c \
	src/xkbcomp/vmod.c \
	src/xkbcomp/vmod.h \
	src/xkbcomp/xkbcomp.c \
	src/xkbcomp/xkbcomp.h \
	src/xkbcomp/xkbcomp-priv.h \
	src/alloc.c \
	src/alloc.h \
	src/atom.c \
	src/atom.h \
	src/context.c \
	src/keysym.c \
	src/map.c \
	src/misc.c \
	src/rules.c \
	src/rules.h \
	src/state.c \
	src/text.c \
	src/text.h \
	src/utils.c \
	src/utils.h \
	src/xkb.c \
	src/xkb-priv.h

BUILT_SOURCES = \
	src/xkbcomp/parser.c \
	src/xkbcomp/parser.h \
	src/xkbcomp/scanner.c \
	src/ks_tables.h
CLEANFILES = $(BUILT_SOURCES)

noinst_PROGRAMS = makekeys/makekeys
makekeys_makekeys_SOURCES = makekeys/makekeys.c
makekeys_makekeys_CC = $(CC_FOR_BUILD)
makekeys_makekeys_CPPFLAGS = $(AM_CPPFLAGS) $(CPPFLAGS_FOR_BUILD)
makekeys_makekeys_CFLAGS = $(AM_CFLAGS) $(CFLAGS_FOR_BUILD)
makekeys_makekeys_LDFLAGS = $(AM_LDFLAGS) $(LDFLAGS_FOR_BUILD)

src/ks_tables.h: $(KEYSYMDEFS) $(top_builddir)/makekeys/makekeys$(EXEEXT)
	$(top_builddir)/makekeys/makekeys $(KEYSYMDEFS) > $@

# Tests

TESTS_ENVIRONMENT =

TESTS = \
	test/xkey \
	test/filecomp \
	test/namescomp \
	test/rulescomp \
	test/canonicalise \
	test/state \
	test/context
TESTS_LDADD = libxkbcommon.la

test_xkey_LDADD = $(TESTS_LDADD)
test_filecomp_LDADD = $(TESTS_LDADD)
test_namescomp_LDADD = $(TESTS_LDADD)
test_rulescomp_LDADD = $(TESTS_LDADD)
test_canonicalise_LDADD = $(TESTS_LDADD)
test_state_LDADD = $(TESTS_LDADD)
test_context_LDADD = $(TESTS_LDADD)

check_PROGRAMS = $(TESTS)

EXTRA_DIST = test/data
