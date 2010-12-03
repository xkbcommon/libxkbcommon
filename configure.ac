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

AC_PREREQ([2.60])
AC_INIT([libxkbcommon], [0.1.0],
    [https://bugs.freedesktop.org/enter_bug.cgi?product=xorg])
AM_INIT_AUTOMAKE([dist-bzip2 foreign])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

AC_PROG_LIBTOOL
AC_PROG_CC
AC_PROG_YACC
AC_PROG_LEX

m4_ifndef([PKG_PROG_PKG_CONFIG],
    [m4_fatal([Could not locate the pkg-config autoconf macros.
    These are usually located in /usr/share/aclocal/pkg.m4. If your
    macros are in a different location, try setting the environment
    variable ACLOCAL="aclocal -I/other/macro/dir" before running
    autoreconf/autogen.sh.])])
PKG_PROG_PKG_CONFIG

# Require xorg-macros minimum of 1.8 for AM_SILENT_RULES
m4_ifndef([XORG_MACROS_VERSION],
	  [m4_fatal([must install xorg-macros 1.8 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.8)
XORG_DEFAULT_OPTIONS
XORG_CHECK_MALLOC_ZERO

dnl Build native compiler needed for makekeys
AC_ARG_VAR([CC_FOR_BUILD], [Build native C compiler program])
if test "x$CC_FOR_BUILD" = x; then
    if test "$cross_compiling" != no; then
        AC_PATH_PROGS([CC_FOR_BUILD], [gcc cc], [cc])
    else
        CC_FOR_BUILD="$CC"
    fi
fi

AC_CHECK_FUNCS([strcasecmp])

if  test "x$GCC" = xyes ; then
  CFLAGS="$CFLAGS -fvisibility=hidden"
fi

PKG_CHECK_MODULES([X11], [xproto kbproto >= 1.0.5])

dnl Ensure we have keysym headers
AC_MSG_CHECKING([for X11 includedir])
x11includedir="`$PKG_CONFIG --variable=includedir xproto`/X11"
AC_MSG_RESULT([$x11includedir])

AC_MSG_CHECKING([for keysymdef.h])
KEYSYMDEF_H="$x11includedir/keysymdef.h"
test -f "$KEYSYMDEF_H" || AC_MSG_ERROR([can't locate keysymdef.h in $x11includedir])
AC_MSG_RESULT([$KEYSYMDEF_H])
AC_SUBST([KEYSYMDEF_H])

AC_MSG_CHECKING([for XF86keysym.h])
XF86KEYSYM_H="$x11includedir/XF86keysym.h"
test -f "$XF86KEYSYM_H" || AC_MSG_ERROR([can't locate XF86keysym.h in $x11includedir])
AC_MSG_RESULT([$XF86KEYSYM_H])
AC_SUBST([XF86KEYSYM_H])

AC_SUBST([KS_HEADERS], ['$(KEYSYMDEF_H) $(XF86KEYSYM_H)'])

AC_ARG_WITH([xkb_config_root],
    [AS_HELP_STRING([--with-xkb-config-root=<paths>],
        [Set default XKB config root (default: ${datadir}/X11/xkb)])],
    [XKBCONFIGROOT="$withval"],
    [XKBCONFIGROOT='${datadir}/X11/xkb'])
AC_SUBST([XKBCONFIGROOT])

AC_CONFIG_FILES([
Makefile
xkbcommon.pc
include/Makefile
src/Makefile
src/makekeys/Makefile
src/xkbcomp/Makefile
test/Makefile
])
AC_OUTPUT
