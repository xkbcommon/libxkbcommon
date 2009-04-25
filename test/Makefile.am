INCLUDES = -I$(top_srcdir)/include -I$(top_srcdir)/src
AM_CFLAGS = $(X11_CFLAGS) $(CWARNFLAGS)

TESTS_ENVIRONMENT = $(SHELL)

check_PROGRAMS = xkey filecomp namescomp rulescomp canonicalise
TESTS = xkey.sh filecomp.sh namescomp.sh rulescomp.sh canonicalise.sh

clean-local:
	rm -f *.log

xkey_SOURCES = xkey.c
xkey_LDADD = $(top_builddir)/src/libxkbcommon.la

rulescomp_SOURCES = rulescomp.c
rulescomp_LDADD = $(top_builddir)/src/libxkbcommon.la

namescomp_SOURCES = namescomp.c
namescomp_LDADD = $(top_builddir)/src/libxkbcommon.la

filecomp_SOURCES = filecomp.c
filecomp_LDADD = $(top_builddir)/src/libxkbcommon.la

canonicalise_SOURCES = canonicalise.c
canonicalise_LDADD = $(top_builddir)/src/libxkbcommon.la
