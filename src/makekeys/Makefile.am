AM_CPPFLAGS = -I$(top_srcdir)/include
AM_CFLAGS = $(X11_CFLAGS) $(CWARNFLAGS)

# need to use build-native compiler
CC = $(CC_FOR_BUILD)
noinst_PROGRAMS = makekeys

nodist_noinst_HEADERS = ks_tables.h
CLEANFILES = ks_tables.h

ks_tables.h: makekeys$(EXEEXT)
	@rm -f $@
	cat $(KS_HEADERS) | $(builddir)/makekeys$(EXEEXT) > $@
