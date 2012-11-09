dnl  Copyright © 2008 Dan Nicholson
dnl
dnl  Permission to use, copy, modify, distribute, and sell this software and its
dnl  documentation for any purpose is hereby granted without fee, provided that
dnl  the above copyright notice appear in all copies and that both that
dnl  copyright notice and this permission notice appear in supporting
dnl  documentation, and that the name of Keith Packard not be used in
dnl  advertising or publicity pertaining to distribution of the software without
dnl  specific, written prior permission.  Keith Packard makes no
dnl  representations about the suitability of this software for any purpose.  It
dnl  is provided "as is" without express or implied warranty.
dnl
dnl  DAN NICHOLSON DISCLAIMS ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
dnl  INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
dnl  EVENT SHALL KEITH PACKARD BE LIABLE FOR ANY SPECIAL, INDIRECT OR
dnl  CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
dnl  DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
dnl  TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
dnl  PERFORMANCE OF THIS SOFTWARE.
dnl
dnl Process this file with autoconf to create configure.

# Initialize Autoconf
AC_PREREQ([2.62])
AC_INIT([libxkbcommon], [0.2.0],
    [https://bugs.freedesktop.org/enter_bug.cgi?product=libxkbcommon])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-xz no-dist-gzip subdir-objects color-tests parallel-tests])
AM_MAINTAINER_MODE([enable])

# Get _GNU_SOURCE and friends
AC_USE_SYSTEM_EXTENSIONS

# Initialize libtool
LT_INIT

# Require xorg-macros minimum of 1.8 for AM_SILENT_RULES
m4_ifndef([XORG_MACROS_VERSION],
	  [m4_fatal([must install xorg-macros 1.8 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.16)
XORG_DEFAULT_OPTIONS
XORG_MEMORY_CHECK_FLAGS
XORG_ENABLE_DOCS
XORG_WITH_DOXYGEN

# Check for compiler features
AC_C_INLINE
AC_C_TYPEOF

# Check for programs
AC_PROG_MKDIR_P
PKG_PROG_PKG_CONFIG
AC_PROG_LEX
AC_PROG_YACC
AC_PATH_PROG([YACC_INST], $YACC)
if test ! -f "src/xkbcomp/parser.c"; then
   if test -z "$YACC_INST"; then
      AC_MSG_ERROR([yacc not found - unable to compile src/xkbcomp/parser.y])
   fi
fi

# Checks for library functions.
AC_CHECK_FUNCS([strcasecmp strncasecmp])
if test "x$ac_cv_func_strcasecmp" = xno || \
   test "x$ac_cv_func_strncasecmp" = xno; then
    AC_MSG_ERROR([C library does not support strcasecmp/strncasecmp])
fi

AC_CHECK_FUNCS([eaccess euidaccess])

XORG_TESTSET_CFLAG([BASE_CFLAGS], [-fvisibility=hidden])

# Define a configuration option for the XKB config root
xkb_base=`$PKG_CONFIG --variable=xkb_base xkeyboard-config`
if test "x$xkb_base" = x; then
    xkb_base=$datadir/X11/xkb
fi
AC_ARG_WITH([xkb_config_root],
    [AS_HELP_STRING([--with-xkb-config-root=<path>],
        [Set default XKB config root (default: xkeyboard-config install path)])],
    [XKBCONFIGROOT="$withval"],
    [XKBCONFIGROOT="$xkb_base"])
AC_SUBST([XKBCONFIGROOT])

AC_ARG_WITH([default_rules],
    [AS_HELP_STRING([--with-default-rules=<path>],
        [Default XKB ruleset (default: evdev)])],
    [DEFAULT_XKB_RULES="$withval"],
    [DEFAULT_XKB_RULES="evdev"])
AC_DEFINE_UNQUOTED([DEFAULT_XKB_RULES], ["$DEFAULT_XKB_RULES"],
                   [Default XKB ruleset])

AC_ARG_WITH([default_model],
    [AS_HELP_STRING([--with-default-model=<path>],
        [Default XKB model (default: pc105)])],
    [DEFAULT_XKB_MODEL="$withval"],
    [DEFAULT_XKB_MODEL="pc105"])
AC_DEFINE_UNQUOTED([DEFAULT_XKB_MODEL], ["$DEFAULT_XKB_MODEL"],
                   [Default XKB model])

AC_ARG_WITH([default_layout],
    [AS_HELP_STRING([--with-default-layout=<path>],
        [Default XKB layout (default: us)])],
    [DEFAULT_XKB_LAYOUT="$withval"],
    [DEFAULT_XKB_LAYOUT="us"])
AC_DEFINE_UNQUOTED([DEFAULT_XKB_LAYOUT], ["$DEFAULT_XKB_LAYOUT"],
                   [Default XKB layout])

AC_CONFIG_FILES([
    Makefile
    xkbcommon-uninstalled.pc
    xkbcommon.pc
    doc/Doxyfile
])
AC_OUTPUT
