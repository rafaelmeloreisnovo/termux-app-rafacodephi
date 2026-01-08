LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)
LOCAL_LDLIBS := -llog
LOCAL_MODULE := local-socket
LOCAL_SRC_FILES := local-socket.cpp
# Critical: 16KB page alignment for Android 15/16 compatibility
LOCAL_LDFLAGS := -Wl,-z,max-page-size=16384
include $(BUILD_SHARED_LIBRARY)
