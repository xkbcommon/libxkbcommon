ACLOCAL_AMFLAGS = -I m4 ${ACLOCAL_FLAGS}

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xkbcommon.pc

EXTRA_DIST = \
	scripts/makeheader \
	scripts/makekeys \
	scripts/update-keysyms \
	scripts/update-keywords \
	scripts/doxygen-wrapper \
	src/xkbcomp/keywords.gperf \
	test/data \
	README.md \
	doc/quick-guide.md \
	doc/compat.md \
	doc/keymap-format-text-v1.md \
	doc/rules-format.txt \
	doc/doxygen-extra.css \
	xkbcommon.map \
	xkbcommon-x11.map \
	PACKAGING \
	LICENSE \
	meson.build \
	meson_options.txt \
	test/symbols-leak-test.bash

# The constants are added here instead of using AC_DEFINE_UNQUOTED
# because it does not expand variables recursively.
AM_CPPFLAGS = \
	-DDFLT_XKB_CONFIG_ROOT='"$(XKBCONFIGROOT)"' \
	-DXLOCALEDIR='"$(XLOCALEDIR)"' \
	-I$(top_srcdir)/src \
	-I$(top_builddir)/src/xkbcomp \
	-include $(top_builddir)/src/config.h

AM_CFLAGS = $(BASE_CFLAGS)

AM_LDFLAGS = -no-undefined
if HAVE_NO_UNDEFINED
AM_LDFLAGS += -Wl,--no-undefined
endif

AM_YFLAGS = -d -p _xkbcommon_

xkbcommonincludedir = $(includedir)/xkbcommon
xkbcommoninclude_HEADERS = \
	xkbcommon/xkbcommon.h \
	xkbcommon/xkbcommon-compat.h \
	xkbcommon/xkbcommon-compose.h \
	xkbcommon/xkbcommon-keysyms.h \
	xkbcommon/xkbcommon-names.h

lib_LTLIBRARIES = libxkbcommon.la
libxkbcommon_la_SOURCES = \
	src/compose/parser.c \
	src/compose/parser.h \
	src/compose/paths.c \
	src/compose/paths.h \
	src/compose/state.c \
	src/compose/table.c \
	src/compose/table.h \
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
	src/xkbcomp/keymap.c \
	src/xkbcomp/keymap-dump.c \
	src/xkbcomp/keywords.c \
	src/xkbcomp/parser.y \
	src/xkbcomp/parser-priv.h \
	src/xkbcomp/rules.c \
	src/xkbcomp/rules.h \
	src/xkbcomp/scanner.c \
	src/xkbcomp/symbols.c \
	src/xkbcomp/types.c \
	src/xkbcomp/vmod.c \
	src/xkbcomp/vmod.h \
	src/xkbcomp/xkbcomp.c \
	src/xkbcomp/xkbcomp-priv.h \
	src/atom.c \
	src/atom.h \
	src/context.c \
	src/context.h \
	src/context-priv.c \
	src/darray.h \
	src/keysym.c \
	src/keysym.h \
	src/keysym-utf.c \
	src/ks_tables.h \
	src/keymap.c \
	src/keymap.h \
	src/keymap-priv.c \
	src/scanner-utils.h \
	src/state.c \
	src/text.c \
	src/text.h \
	src/utf8.c \
	src/utf8.h \
	src/utils.c \
	src/utils.h
if HAVE_VERSION_SCRIPT
libxkbcommon_la_LDFLAGS = -Wl,--version-script=${top_srcdir}/xkbcommon.map
endif HAVE_VERSION_SCRIPT

if ENABLE_X11
pkgconfig_DATA += xkbcommon-x11.pc

xkbcommon_x11includedir = $(xkbcommonincludedir)
xkbcommon_x11include_HEADERS = \
	xkbcommon/xkbcommon-x11.h

lib_LTLIBRARIES += libxkbcommon-x11.la

libxkbcommon_x11_la_CFLAGS = $(AM_CFLAGS) $(XCB_XKB_CFLAGS)
libxkbcommon_x11_la_CPPFLAGS = $(AM_CPPFLAGS)
libxkbcommon_x11_la_LIBADD = libxkbcommon.la $(XCB_XKB_LIBS)
if HAVE_VERSION_SCRIPT
libxkbcommon_x11_la_LDFLAGS = -Wl,--version-script=${top_srcdir}/xkbcommon-x11.map
endif HAVE_VERSION_SCRIPT

libxkbcommon_x11_la_SOURCES = \
	src/x11/keymap.c \
	src/x11/state.c \
	src/x11/util.c \
	src/x11/x11-priv.h \
	src/context.h \
	src/context-priv.c \
	src/keymap.h \
	src/keymap-priv.c \
	src/atom.h \
	src/atom.c
endif ENABLE_X11

BUILT_SOURCES = \
	src/xkbcomp/parser.c \
	src/xkbcomp/parser.h
CLEANFILES = $(BUILT_SOURCES)

src/xkbcomp/parser.c: $(top_builddir)/src/$(am__dirstamp) $(top_builddir)/src/xkbcomp/$(am__dirstamp)
src/xkbcomp/parser.h: $(top_builddir)/src/$(am__dirstamp) $(top_builddir)/src/xkbcomp/$(am__dirstamp)

##
# Documentation
##

if ENABLE_DOCS
if HAVE_DOXYGEN
doxygen_input = \
	README.md \
	doc/quick-guide.md \
	doc/compat.md \
	xkbcommon/xkbcommon.h \
	xkbcommon/xkbcommon-names.h \
	xkbcommon/xkbcommon-x11.h \
	xkbcommon/xkbcommon-compose.h

doc/html: doc/Doxyfile $(doxygen_input)
	$(AM_V_GEN)(\
	    cat $<; \
	    echo "INPUT = $(doxygen_input)"; \
	    echo "OUTPUT_DIRECTORY = $(abs_top_builddir)/doc"; \
	) | $(top_srcdir)/scripts/doxygen-wrapper "$(DOXYGEN)" - "$(abs_top_srcdir)"
clean-doxygen:
	rm -rf doc/html

doc: doc/html
clean-doc: clean-doxygen
all-local:: doc
clean-local:: clean-doc

install-data-local:: doc
	$(MKDIR_P) $(DESTDIR)$(htmldir)/html
	$(INSTALL_DATA) doc/html/* $(DESTDIR)$(htmldir)/html

uninstall-local::
	rm -rf $(DESTDIR)$(htmldir)
endif HAVE_DOXYGEN
endif ENABLE_DOCS

##
# Tests
##

# Some tests need to use unexported symbols, so we link them against
# a private copy of libxkbcommon with all symbols exposed.
check_LTLIBRARIES = libtest.la
libtest_la_SOURCES = \
	$(libxkbcommon_la_SOURCES) \
	test/common.c \
	test/test.h \
	test/evdev-scancodes.h

AM_TESTS_ENVIRONMENT = \
	XKB_LOG_LEVEL=debug; export XKB_LOG_LEVEL; \
	XKB_LOG_VERBOSITY=10; export XKB_LOG_VERBOSITY; \
	$(XORG_MALLOC_DEBUG_ENV) \
	top_srcdir=$(top_srcdir)

build_run_tests = \
	test/keysym \
	test/keymap \
	test/filecomp \
	test/context \
	test/rules-file \
	test/stringcomp \
	test/buffercomp \
	test/log \
	test/atom \
	test/utf8 \
	test/state \
	test/keyseq \
	test/rulescomp \
	test/compose
build_only_tests = \
	test/rmlvo-to-kccgst \
	test/print-compiled-keymap
run_only_tests = \
	test/symbols-leak-test.bash

TESTS_LDADD = libtest.la

test_keysym_LDADD = $(TESTS_LDADD)
test_keymap_LDADD = $(TESTS_LDADD)
test_filecomp_LDADD = $(TESTS_LDADD)
test_context_LDADD = $(TESTS_LDADD)
test_rules_file_CFLAGS = $(AM_CFLAGS) -Wno-declaration-after-statement
test_rules_file_LDADD = $(TESTS_LDADD)
test_stringcomp_LDADD = $(TESTS_LDADD)
test_buffercomp_LDADD = $(TESTS_LDADD)
test_log_LDADD = $(TESTS_LDADD)
test_atom_LDADD = $(TESTS_LDADD)
test_utf8_LDADD = $(TESTS_LDADD)
test_state_LDADD = $(TESTS_LDADD)
test_keyseq_LDADD = $(TESTS_LDADD)
test_rulescomp_LDADD = $(TESTS_LDADD)
test_rmlvo_to_kccgst_LDADD = $(TESTS_LDADD)
test_print_compiled_keymap_LDADD = $(TESTS_LDADD)
test_compose_LDADD = $(TESTS_LDADD) $(RT_LIBS)

if BUILD_LINUX_TESTS
build_only_tests += \
	test/interactive-evdev

test_interactive_evdev_LDADD = $(TESTS_LDADD)
endif BUILD_LINUX_TESTS

if ENABLE_X11
build_run_tests += \
	test/x11
# test/x11comp is meant to be run, but it is (temporarily?) disabled.
# See: https://github.com/xkbcommon/libxkbcommon/issues/30
build_only_tests += \
	test/x11comp \
	test/interactive-x11

TESTS_X11_LDADD = $(XCB_XKB_LIBS) $(TESTS_LDADD) libxkbcommon-x11.la
TESTS_X11_CFLAGS = $(XCB_XKB_CFLAGS)

test_x11_LDADD = $(TESTS_X11_LDADD)
test_x11_CFLAGS = $(TESTS_X11_CFLAGS)
test_x11comp_LDADD = $(TESTS_X11_LDADD)
test_x11comp_CFLAGS = $(TESTS_X11_CFLAGS)
test_interactive_x11_LDADD = $(TESTS_X11_LDADD)
test_interactive_x11_CFLAGS = $(TESTS_X11_CFLAGS)
endif ENABLE_X11

if ENABLE_WAYLAND
build_only_tests += \
	test/interactive-wayland

TESTS_WAYLAND_LDADD = $(WAYLAND_LIBS) $(TESTS_LDADD)
TESTS_WAYLAND_CFLAGS = $(WAYLAND_CFLAGS)

.SECONDEXPANSION:

define protostability
$(if $(findstring unstable,$1),unstable,stable)
endef

define protoname
$(shell echo $1 | sed 's/\([a-z\-]\+\)-[a-z]\+-v[0-9]\+/\1/')
endef

%-protocol.c: $(WAYLAND_PROTOCOLS_DATADIR)/$$(call protostability,$$*)/$$(call protoname,$$*)/$$*.xml
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(wayland_scanner) code < $< > $@
%-client-protocol.h: $(WAYLAND_PROTOCOLS_DATADIR)/$$(call protostability,$$*)/$$(call protoname,$$*)/$$*.xml
	$(AM_V_GEN)$(MKDIR_P) $(dir $@) && $(wayland_scanner) client-header < $< > $@

INTERACTIVE_WL_XDG_SHELL_SRCS = \
	xdg-shell-unstable-v6-protocol.c \
	xdg-shell-unstable-v6-client-protocol.h

test_interactive_wayland_SOURCES = \
	test/interactive-wayland.c \
	$(INTERACTIVE_WL_XDG_SHELL_SRCS)
test_interactive_wayland_LDADD = $(TESTS_WAYLAND_LDADD)
test_interactive_wayland_CFLAGS = $(TESTS_WAYLAND_CFLAGS)

BUILT_SOURCES += \
	$(INTERACTIVE_WL_XDG_SHELL_SRCS)
endif

check_PROGRAMS = $(build_run_tests) $(build_only_tests)
TESTS = $(build_run_tests) $(run_only_tests)

##
# Benchmarks
##
check_LTLIBRARIES += libbench.la
libbench_la_SOURCES = \
	bench/bench.c \
	bench/bench.h
BENCH_LDADD  = libbench.la
BENCH_LDADD += $(TESTS_LDADD) $(RT_LIBS)

check_PROGRAMS += \
	bench/key-proc \
	bench/rules \
	bench/rulescomp \
	bench/compose
bench_key_proc_LDADD = $(BENCH_LDADD)
bench_rules_LDADD = $(BENCH_LDADD)
bench_rulescomp_LDADD = $(BENCH_LDADD)
bench_compose_LDADD = $(BENCH_LDADD)
