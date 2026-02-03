LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := rmr
LOCAL_SRC_FILES := rmr.c
LOCAL_CFLAGS := -std=c11 -Wall -Wextra -Werror -Os -fno-stack-protector
LOCAL_LDFLAGS := -Wl,--gc-sections

include $(BUILD_SHARED_LIBRARY)
