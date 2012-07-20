ACLOCAL_AMFLAGS = -I m4

SUBDIRS = makekeys

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
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/xkbcomp \
	-I$(top_builddir)/src/xkbcomp \
	-include $(top_builddir)/src/config.h

AM_CFLAGS = \
	$(BASE_CFLAGS) \
	$(XMALLOC_ZERO_CFLAGS)

AM_YFLAGS = -d

xkbcommonincludedir = $(includedir)/xkbcommon
xkbcommoninclude_HEADERS = \
	xkbcommon/xkbcommon.h \
	xkbcommon/xkbcommon-names.h \
	xkbcommon/xkbcommon-keysyms.h

lib_LTLIBRARIES = libxkbcommon.la
libxkbcommon_la_LDFLAGS = -no-undefined
libxkbcommon_la_SOURCES = \
	src/xkbcomp/action.c \
	src/xkbcomp/action.h \
	src/xkbcomp/compat.c \
	src/xkbcomp/expr.c \
	src/xkbcomp/expr.h \
	src/xkbcomp/keycodes.c \
	src/xkbcomp/keytypes.c \
	src/xkbcomp/misc.c \
	src/xkbcomp/parser.y \
	src/xkbcomp/parseutils.c \
	src/xkbcomp/parseutils.h \
	src/xkbcomp/path.c \
	src/xkbcomp/path.h \
	src/xkbcomp/rules.c \
	src/xkbcomp/rules.h \
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
	src/darray.h \
	src/keymap-dump.c \
	src/keysym.c \
	src/keysym-utf.c \
	src/list.h \
	src/map.c \
	src/state.c \
	src/text.c \
	src/text.h \
	src/utils.h \
	src/xkb-priv.h

BUILT_SOURCES = \
	src/xkbcomp/parser.c \
	src/xkbcomp/parser.h \
	src/xkbcomp/scanner.c \
	src/ks_tables.h
CLEANFILES = $(BUILT_SOURCES)

src/xkbcomp/parser.c: $(top_builddir)/src/$(am__dirstamp) $(top_builddir)/src/xkbcomp/$(am__dirstamp)
src/xkbcomp/parser.h: $(top_builddir)/src/$(am__dirstamp) $(top_builddir)/src/xkbcomp/$(am__dirstamp)
src/xkbcomp/scanner.c: $(top_builddir)/src/$(am__dirstamp) $(top_builddir)/src/xkbcomp/$(am__dirstamp)

src/ks_tables.h: $(top_builddir)/makekeys/makekeys$(EXEEXT)
	$(AM_V_GEN)$(top_builddir)/makekeys/makekeys $(top_srcdir)/xkbcommon/xkbcommon-keysyms.h > $@

$(top_builddir)/makekeys/makekeys$(EXEEXT): $(top_srcdir)/makekeys/makekeys.c
	$(MAKE) -C makekeys

# Tests

# Some tests need to use unexported symbols, so we link them against
# a private copy of libxkbcommon with all symbols exposed.
noinst_LTLIBRARIES = libtest.la
libtest_la_LDFLAGS = $(libxkbcommon_la_LDFLAGS)
libtest_la_SOURCES = \
	$(libxkbcommon_la_SOURCES) \
	test/common.c

TESTS_ENVIRONMENT =

TESTS = \
	test/xkey \
	test/filecomp \
	test/namescomp \
	test/rulescomp \
	test/state \
	test/context \
	test/rules-file \
	test/dump \
	test/stringcomp \
	test/keyseq
TESTS_LDADD = libtest.la

test_xkey_LDADD = $(TESTS_LDADD)
test_filecomp_LDADD = $(TESTS_LDADD)
test_namescomp_LDADD = $(TESTS_LDADD)
test_rulescomp_LDADD = $(TESTS_LDADD) -lrt
test_state_LDADD = $(TESTS_LDADD)
test_context_LDADD = $(TESTS_LDADD)
test_rules_file_CFLAGS = $(AM_CFLAGS) -Wno-declaration-after-statement
test_rules_file_LDADD = $(TESTS_LDADD)
test_dump_LDADD = $(TESTS_LDADD)
test_stringcomp_LDADD = $(TESTS_LDADD)
test_keyseq_LDADD = $(TESTS_LDADD)

check_PROGRAMS = $(TESTS)

EXTRA_DIST = \
	test/data \
	test/test.h

# This sed script strips out lines that start with '#define _' which
# removes #define _OSF_Keysyms and such.  The XK_Ydiaeresis case is to
# handle a duplicate definition in HPkeysyms.h which kicks in if it's
# not already defined.

X11_INCLUDEDIR = /usr/include/X11
KEYSYMDEFS = \
	 $(X11_INCLUDEDIR)/keysymdef.h \
	 $(X11_INCLUDEDIR)/XF86keysym.h \
	 $(X11_INCLUDEDIR)/Sunkeysym.h \
	 $(X11_INCLUDEDIR)/DECkeysym.h \
	 $(X11_INCLUDEDIR)/HPkeysym.h
update-keysyms:
	sed -e '/XK_Ydiaeresis\s*0x100000ee/d; /#define _/d; s/#define\s*\(\w*\)XK_/#define XKB_KEY_\1/; /\(#ifdef\|#ifndef\|#endif\)/d' $(KEYSYMDEFS) > xkbcommon/xkbcommon-keysyms.h
