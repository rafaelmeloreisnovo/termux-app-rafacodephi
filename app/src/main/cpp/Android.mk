LOCAL_PATH:= $(call my-dir)

# Bootstrap library
include $(CLEAR_VARS)
LOCAL_MODULE := libtermux-bootstrap
LOCAL_SRC_FILES := termux-bootstrap-zip.S termux-bootstrap.c
# Critical: 16KB page alignment for Android 15/16 compatibility
LOCAL_LDFLAGS := -Wl,-z,max-page-size=16384
include $(BUILD_SHARED_LIBRARY)

# Bare-metal low-level library
include $(CLEAR_VARS)
LOCAL_MODULE := termux-baremetal
LOCAL_SRC_FILES := lowlevel/baremetal.c lowlevel/baremetal_jni.c
# Assembly optimizations enabled for NEON support
LOCAL_SRC_FILES += lowlevel/baremetal_asm.S
LOCAL_CFLAGS := -std=c11 -Wall -Wextra -Werror -Os -fno-stack-protector
LOCAL_CFLAGS += -ffast-math -fno-exceptions -fno-rtti
LOCAL_CFLAGS += -ffunction-sections -fdata-sections
# Critical: 16KB page alignment for Android 15/16 compatibility
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,-z,max-page-size=16384

# Architecture-specific optimizations
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_ARM_NEON := true
    LOCAL_CFLAGS += -march=armv7-a -mfpu=neon -mfloat-abi=softfp -ftree-vectorize
endif

ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_CFLAGS += -march=armv8-a -ftree-vectorize
endif

ifeq ($(TARGET_ARCH_ABI),x86)
    LOCAL_CFLAGS += -msse2 -msse4.2 -ftree-vectorize
endif

ifeq ($(TARGET_ARCH_ABI),x86_64)
    LOCAL_CFLAGS += -msse2 -msse4.2 -mavx -ftree-vectorize
endif

# Link against pthread for thread support and math library
LOCAL_LDLIBS := -llog -lm
include $(BUILD_SHARED_LIBRARY)

