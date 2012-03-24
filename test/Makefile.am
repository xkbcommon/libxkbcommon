AM_CPPFLAGS = -I$(top_srcdir)/include -I$(top_srcdir)/src
AM_CFLAGS = $(X11_CFLAGS) $(XORG_COMPILER_FLAGS)
LDADD = $(top_builddir)/libxkbcommon.la

TESTS_ENVIRONMENT = $(SHELL)

check_PROGRAMS = xkey filecomp namescomp rulescomp canonicalise state
TESTS = $(check_PROGRAMS:=.sh)

EXTRA_DIST =		\
	$(TESTS)	\
	bad.xkb		\
	basic.xkb	\
	default.xkb	\
	named.xkb	\
	comprehensive-plus-geom.xkb

clean-local:
	rm -f *.log
