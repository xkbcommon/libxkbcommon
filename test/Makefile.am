INCLUDES = -I$(top_srcdir)/include
AM_CFLAGS = $(X11_CFLAGS) $(CWARNFLAGS)

check_PROGRAMS = xkey
xkey_SOURCES = xkey.c
xkey_LDADD = $(top_builddir)/src/libxkbcommon.la

TESTS = xkey.sh
TESTS_ENVIRONMENT = $(SHELL)
