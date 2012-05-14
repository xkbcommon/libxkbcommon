LOCAL_PATH:= $(call my-dir)

include $(CLEAR_VARS)

LIBXKBCOMMON_TOP := $(LOCAL_PATH)

LIBXKBCOMMON_BUILT_MAKEFILES := \
	$(LIBXKBCOMMON_TOP)/Android_build.mk

libxkbcommon-configure:
	cd $(LIBXKBCOMMON_TOP) && autoreconf -fiv
	cd $(LIBXKBCOMMON_TOP) && \
		CC="$(CONFIGURE_CC)" \
		CFLAGS="$(CONFIGURE_CFLAGS)" \
		LD=$(TARGET_LD) \
		LDFLAGS="$(CONFIGURE_LDFLAGS)" \
		CPP=$(CONFIGURE_CPP) \
		CPPFLAGS="$(CONFIGURE_CPPFLAGS)" \
		PKG_CONFIG_LIBDIR=$(CONFIGURE_PKG_CONFIG_LIBDIR) \
		PKG_CONFIG_TOP_BUILD_DIR=$(PKG_CONFIG_TOP_BUILD_DIR) \
		./configure --host=arm-linux-androideabi \
		--prefix /system \
		--with-xkb-config-root=/system/usr/share/xkb
	rm -f $(LIBXKBCOMMON_BUILT_MAKEFILES)
	@for file in $(LIBXKBCOMMON_BUILT_MAKEFILES); do \
		echo "make -C $$(dirname $$file) $$(basename $$file)" ; \
		make -C $$(dirname $$file) $$(basename $$file) ; \
	done

libxkbcommon-reset:
	cd $(LIBXKBCOMMON_TOP) && \
	git clean -qdxf && \
	git reset --hard HEAD

libxkbcommon-clean:

.PHONY: libxkbcommon-configure libxkbcommon-clean libxkbcommon-reset

CONFIGURE_TARGETS += libxkbcommon-configure
CONFIGURE_RESET_TARGETS += libxkbcommon-reset
AGGREGATE_CLEAN_TARGETS += libxkbcommon-clean
CONFIGURE_PKG_CONFIG_LIBDIR := $(CONFIGURE_PKG_CONFIG_LIBDIR):$(LIBXKBCOMMON_TOP)

-include $(LIBXKBCOMMON_BUILT_MAKEFILES)

