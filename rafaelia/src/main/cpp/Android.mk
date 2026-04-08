LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := termux-rafaelia
LOCAL_SRC_FILES := rafaelia.c \
  rafaelia_bitraf_core.c \
  raf_termux_emul.c \
  raf_termux_registry.c \
  raf_termux_catalog.c \
  raf_termux_packages.c \
  raf_termux_essentials.c \
  raf_termux_toolset.c \
  raf_termux_exec.c \
  tools/raf_termux_pkg_tool.c
LOCAL_CFLAGS := -Wall -Wextra -Werror -Wno-unused-function -Os
# Critical: 16KB page alignment for Android 15/16 compatibility
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,-z,max-page-size=16384

include $(BUILD_SHARED_LIBRARY)
