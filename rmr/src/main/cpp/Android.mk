LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE := rmr
LOCAL_SRC_FILES := rmr.c
LOCAL_CFLAGS := -std=c11 -Wall -Wextra -Werror -Os \
  -fno-stack-protector -fno-common \
  -ffunction-sections -fdata-sections
# Necessário para compatibilidade com dispositivos de page size 16KB.
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,-z,max-page-size=16384

include $(BUILD_SHARED_LIBRARY)
