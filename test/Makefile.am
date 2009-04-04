INCLUDES = -I$(top_srcdir)/include
AM_CFLAGS = $(X11_CFLAGS) $(CWARNFLAGS)

TESTS_ENVIRONMENT = $(SHELL)

check_PROGRAMS = xkey rulescomp
TESTS = xkey.sh rulescomp.sh

xkey_SOURCES = xkey.c
xkey_LDADD = $(top_builddir)/src/libxkbcommon.la

rulescomp_SOURCES = rulescomp.c
rulescomp_LDADD = $(top_builddir)/src/libxkbcommon.la
