LOCAL_PATH:= $(call my-dir)

# Bootstrap library
include $(CLEAR_VARS)
LOCAL_MODULE := libtermux-bootstrap
LOCAL_SRC_FILES := termux-bootstrap-zip.S termux-bootstrap.c
include $(BUILD_SHARED_LIBRARY)

# Low-level bare-metal optimizations library
include $(CLEAR_VARS)
LOCAL_MODULE := libtermux-lowlevel
LOCAL_SRC_FILES := lowlevel/vectra_math.c lowlevel/lowlevel_jni.c lowlevel/vectra_asm.S
LOCAL_C_INCLUDES := $(LOCAL_PATH)/lowlevel
LOCAL_CFLAGS := -std=c11 -Wall -Wextra -Werror -Os -fno-stack-protector -Wl,--gc-sections
LOCAL_CFLAGS += -ftree-vectorize -ffast-math -march=armv7-a -mfloat-abi=softfp -mfpu=neon
LOCAL_LDLIBS := -lm -llog
include $(BUILD_SHARED_LIBRARY)
