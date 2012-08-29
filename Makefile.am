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
	src/xkbcomp/ast.h \
	src/xkbcomp/ast-build.c \
	src/xkbcomp/ast-build.h \
	src/xkbcomp/compat.c \
	src/xkbcomp/expr.c \
	src/xkbcomp/expr.h \
	src/xkbcomp/include.c \
	src/xkbcomp/include.h \
	src/xkbcomp/keycodes.c \
	src/xkbcomp/keycodes.h \
	src/xkbcomp/keymap.c \
	src/xkbcomp/parser.y \
	src/xkbcomp/parser-priv.h \
	src/xkbcomp/rules.c \
	src/xkbcomp/rules.h \
	src/xkbcomp/scanner.l \
	src/xkbcomp/symbols.c \
	src/xkbcomp/types.c \
	src/xkbcomp/vmod.c \
	src/xkbcomp/vmod.h \
	src/xkbcomp/xkbcomp.c \
	src/xkbcomp/xkbcomp-priv.h \
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

AM_TESTS_ENVIRONMENT = \
	XKB_LOG=debug; export XKB_LOG; \
	XKB_VERBOSITY=10; export XKB_VERBOSITY;

TESTS = \
	test/keysym \
	test/filecomp \
	test/rulescomp \
	test/state \
	test/context \
	test/rules-file \
	test/stringcomp \
	test/keyseq \
	test/log
TESTS_LDADD = libtest.la

test_keysym_LDADD = $(TESTS_LDADD)
test_filecomp_LDADD = $(TESTS_LDADD)
test_rulescomp_LDADD = $(TESTS_LDADD) -lrt
test_state_LDADD = $(TESTS_LDADD)
test_context_LDADD = $(TESTS_LDADD)
test_rules_file_CFLAGS = $(AM_CFLAGS) -Wno-declaration-after-statement
test_rules_file_LDADD = $(TESTS_LDADD)
test_stringcomp_LDADD = $(TESTS_LDADD)
test_keyseq_LDADD = $(TESTS_LDADD)
test_log_LDADD = $(TESTS_LDADD)
test_interactive_LDADD = $(TESTS_LDADD)
test_rmlvo_to_kccgst_LDADD = $(TESTS_LDADD)

check_PROGRAMS = \
	$(TESTS) \
	test/interactive \
	test/rmlvo-to-kccgst

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

# Android stuff

Android_build.mk: Makefile $(BUILT_SOURCES)
	androgenizer \
	-:PROJECT libxkbcommon \
	-:REL_TOP $(top_srcdir) -:ABS_TOP $(abs_top_srcdir) \
	\
	-:STATIC libxkbcommon \
	-:TAGS eng debug \
	-:SOURCES $(filter-out %.l %.y,$(libxkbcommon_la_SOURCES)) $(BUILT_SOURCES) \
	-:CFLAGS $(DEFS) $(DEFAULT_INCLUDES) $(AM_CPPFLAGS) $(AM_CFLAGS) \
	-:LDFLAGS $(libxkbcommon_la_LDFLAGS) \
	\
	-:PASSTHROUGH 'libxkbcommon-clean: clean-libxkbcommon' \
	> $@
