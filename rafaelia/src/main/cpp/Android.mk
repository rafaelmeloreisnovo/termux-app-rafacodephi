LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := termux-rafaelia
LOCAL_SRC_FILES := rafaelia.c \
  rafaelia_bitraf_core.c \
  raf_numbase.c \
  raf_termux_emul.c \
  raf_termux_registry.c \
  raf_termux_catalog.c \
  raf_termux_packages.c \
  raf_termux_essentials.c \
  raf_termux_toolset.c \
  raf_termux_exec.c \
  tools/raf_termux_pkg_tool.c

# Warnings remain visible. Intentional optional symbols must use RAF_UNUSED;
# truly unreachable functions/data are emitted into isolated sections and
# removed by --gc-sections at link time.
LOCAL_CFLAGS := -std=c11 -Wall -Wextra -Werror \
  -Wno-error=unused-function \
  -Wno-error=missing-field-initializers \
  -Os -fno-common -ffunction-sections -fdata-sections

# ECC32 compile-time policy. Compact is canonical for the current -Os module;
# speed builds may pass RAF_ECC32_PROFILE=speed to select full unrolling.
RAF_ECC32_PROFILE ?= compact
ifeq ($(RAF_ECC32_PROFILE),speed)
  LOCAL_CFLAGS += -DRAF_ECC32_FORCE_UNROLL=1
else ifeq ($(RAF_ECC32_PROFILE),compact)
  LOCAL_CFLAGS += -DRAF_ECC32_FORCE_COMPACT=1
else
  $(error Unsupported RAF_ECC32_PROFILE='$(RAF_ECC32_PROFILE)'; use compact or speed)
endif

# Critical: 16KB page alignment for Android 15/16 compatibility.
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,-z,max-page-size=16384

include $(BUILD_SHARED_LIBRARY)
