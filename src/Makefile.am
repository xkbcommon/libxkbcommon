noinst_PROGRAMS = makekeys
makekeys_CFLAGS = $(X11_CFLAGS)
makekeys: CC = $(CC_FOR_BUILD)

#BUILT_SOURCES = ks_tables.h
noinst_HEADERS = ks_tables.h
ks_tables.h: makekeys
	@rm -f $@
	cat $(KEYSYMDEF_H) $(XF86KEYSYM_H) | $(builddir)/makekeys$(EXEEXT) >$@
