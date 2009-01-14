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

AC_PREREQ([2.57])
AC_INIT([libxkbcommon], [0.1.0],
    [https://bugs.freedesktop.org/enter_bug.cgi?product=xorg])
AM_INIT_AUTOMAKE([dist-bzip2 foreign])
AM_MAINTAINER_MODE

AC_PROG_LIBTOOL

# Require xorg-macros version 1.1.0 or newer for XORG_WITH_LINT macro
m4_ifndef([XORG_MACROS_VERSION],
    [m4_fatal([must install xorg-macros 1.1 or later before running autoconf/autogen])])
XORG_MACROS_VERSION([1.1])
XORG_RELEASE_VERSION
XORG_CHANGELOG

AC_OUTPUT([
Makefile
])
