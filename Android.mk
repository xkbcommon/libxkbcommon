LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

LIBXKBCOMMON_TOP := $(LOCAL_PATH)

LIBXKBCOMMON_BUILT_MAKEFILES := \
	$(LIBXKBCOMMON_TOP)/Android_build.mk

LIBXKBCOMMON_CONFIGURE_TARGET := $(LIBXKBCOMMON_TOP)/Makefile

$(LIBXKBCOMMON_CONFIGURE_TARGET): $(CONFIGURE_DEPENDENCIES)
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

$(LIBXKBCOMMON_BUILT_MAKEFILES): $(LIBXKBCOMMON_CONFIGURE_TARGET)
	make -C $(dir $@) $(notdir $@)

.PHONY: libxkbcommon-reset libxkbcommon-clean

libxkbcommon-reset:
	cd $(LIBXKBCOMMON_TOP) && \
	git clean -qdxf && \
	git reset --hard HEAD

libxkbcommon-clean:

contrib-reset: libxkbcommon-reset
contrib-clean: libxkbcommon-clean

CONFIGURE_PKG_CONFIG_LIBDIR := $(CONFIGURE_PKG_CONFIG_LIBDIR):$(abspath $(LIBXKBCOMMON_TOP))

include $(LIBXKBCOMMON_BUILT_MAKEFILES)

