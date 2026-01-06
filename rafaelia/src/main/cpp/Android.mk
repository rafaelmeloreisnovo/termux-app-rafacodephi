LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := termux-rafaelia
LOCAL_SRC_FILES := rafaelia.c
LOCAL_CFLAGS := -Wall -Wextra -Werror -Os -fno-stack-protector
LOCAL_LDFLAGS := -Wl,--gc-sections

include $(BUILD_SHARED_LIBRARY)
