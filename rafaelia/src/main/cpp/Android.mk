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

# RAFAELIA ZERO — RFZ1 binary conversation-chunk kernel.
# The core has no heap, libc calls, syscalls, I/O, reflection or runtime dispatch.
# JNI is an outer shell and accepts DirectByteBuffer for the canonical hot path.
include $(CLEAR_VARS)
LOCAL_MODULE := termux_rafaelia_zero
LOCAL_SRC_FILES := zero/rafz.c zero/rafz_android_jni.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/zero/include
LOCAL_CFLAGS := -std=c11 -O3 -Wall -Wextra -Werror \
  -ffreestanding -fno-builtin -fno-stack-protector -fno-common \
  -fvisibility=hidden -ffunction-sections -fdata-sections \
  -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-ident

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
  LOCAL_CFLAGS += -march=armv7-a -mfpu=neon-vfpv4 -mfloat-abi=softfp
endif
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
  LOCAL_CFLAGS += -march=armv8-a+simd
endif
ifeq ($(TARGET_ARCH_ABI),x86)
  LOCAL_CFLAGS += -march=i686 -msse2
endif
ifeq ($(TARGET_ARCH_ABI),x86_64)
  LOCAL_CFLAGS += -march=x86-64 -msse2
endif

LOCAL_LDFLAGS := -Wl,--gc-sections \
  -Wl,-z,max-page-size=16384 \
  -Wl,-z,common-page-size=16384
include $(BUILD_SHARED_LIBRARY)
