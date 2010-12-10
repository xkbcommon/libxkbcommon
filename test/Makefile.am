AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src
AM_CFLAGS = $(X11_CFLAGS) $(CWARNFLAGS)
LDADD = $(top_builddir)/src/libxkbcommon.la

TESTS_ENVIRONMENT = $(SHELL)

check_PROGRAMS = xkey filecomp namescomp rulescomp canonicalise
TESTS = $(check_PROGRAMS:=.sh)

EXTRA_DIST =		\
	$(TESTS)	\
	bad.xkb		\
	basic.xkb	\
	default.xkb	\
	named.xkb

clean-local:
	rm -f *.log
