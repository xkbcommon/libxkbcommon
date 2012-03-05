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
AC_PREREQ([2.60])
AC_INIT([libxkbcommon], [0.1.0],
    [https://bugs.freedesktop.org/enter_bug.cgi?product=xorg])
AC_CONFIG_SRCDIR([Makefile.am])
AC_CONFIG_HEADERS([src/config.h])
AC_CONFIG_MACRO_DIR([m4])

# Initialize Automake
AM_INIT_AUTOMAKE([foreign dist-bzip2])
AM_MAINTAINER_MODE

# Initialize libtool
AC_PROG_LIBTOOL

# Require xorg-macros minimum of 1.8 for AM_SILENT_RULES
m4_ifndef([XORG_MACROS_VERSION],
	  [m4_fatal([must install xorg-macros 1.8 or later before running autoconf/autogen])])
XORG_MACROS_VERSION(1.8)
XORG_DEFAULT_OPTIONS
XORG_CHECK_MALLOC_ZERO

# Check for programs
AC_PROG_LEX
AC_PROG_YACC
AC_PATH_PROG([YACC_INST], $YACC)
if test ! -f "src/xkbcomp/xkbparse.c"; then
   if test -z "$YACC_INST"; then
      AC_MSG_ERROR([yacc not found - unable to compile src/xkbcomp/xkbparse.y])
   fi
fi

# Checks for library functions.
AC_CHECK_FUNCS([strcasecmp])

# Build native compiler needed for makekeys
AC_ARG_VAR([CC_FOR_BUILD], [Build native C compiler program])
if test "x$CC_FOR_BUILD" = x; then
    if test "$cross_compiling" != no; then
        AC_PATH_PROGS([CC_FOR_BUILD], [gcc cc], [cc])
    else
        CC_FOR_BUILD="$CC"
    fi
fi

if  test "x$GCC" = xyes ; then
  CFLAGS="$CFLAGS -fvisibility=hidden"
fi

# Obtain protocols headers include directives
PKG_CHECK_MODULES([X11], [xproto kbproto >= 1.0.4])

# Obtain the path to the X Window System Core Protocol
AC_MSG_CHECKING([for X11 includedir])
includex11dir="`$PKG_CONFIG --variable=includex11dir xproto`"
AC_MSG_RESULT([$includex11dir])

AC_MSG_CHECKING([keysym definitions])
KEYSYMDEFDIR=`$PKG_CONFIG --variable=includedir xproto`/X11
FILES="keysymdef.h XF86keysym.h Sunkeysym.h DECkeysym.h HPkeysym.h"
for i in $FILES; do
    if test -f "$KEYSYMDEFDIR/$i"; then
            KEYSYMDEFS="$KEYSYMDEFS $KEYSYMDEFDIR/$i"
    elif test "x$i" = "xkeysymdef.h"; then
            AC_MSG_ERROR([Cannot find keysymdef.h])
    fi
done
AC_MSG_RESULT([$KEYSYMDEFS])
AC_SUBST(KEYSYMDEFS)

# Define a configuration option for the XKB config root
xkb_base=`$PKG_CONFIG --variable=xkb_base xkeyboard-config`
AC_ARG_WITH([xkb_config_root],
    [AS_HELP_STRING([--with-xkb-config-root=<paths>],
        [Set default XKB config root (default: ${datadir}/X11/xkb)])],
    [XKBCONFIGROOT="$withval"],
    [XKBCONFIGROOT="$xkb_base"])
AC_SUBST([XKBCONFIGROOT])

AC_CONFIG_FILES([
	Makefile
	xkbcommon.pc
	include/Makefile
	makekeys/Makefile
	src/Makefile
	src/xkbcomp/Makefile
	test/Makefile])
AC_OUTPUT
