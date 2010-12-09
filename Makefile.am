# Order: makekeys before src
SUBDIRS = include makekeys src test

pkgconfigdir = $(libdir)/pkgconfig
pkgconfig_DATA = xkbcommon.pc

MAINTAINERCLEANFILES = ChangeLog INSTALL

.PHONY: ChangeLog INSTALL

INSTALL:
	$(INSTALL_CMD)

ChangeLog:
	$(CHANGELOG_CMD)

dist-hook: ChangeLog INSTALL
