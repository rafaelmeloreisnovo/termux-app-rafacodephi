LOCAL_PATH:= $(call my-dir)

# Bootstrap library
include $(CLEAR_VARS)
LOCAL_MODULE := libtermux-bootstrap
LOCAL_SRC_FILES := termux-bootstrap-zip.S termux-bootstrap.c
LOCAL_CFLAGS += -fno-common -ffunction-sections -fdata-sections
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,-z,max-page-size=16384
include $(BUILD_SHARED_LIBRARY)

# Bare-metal low-level library
include $(CLEAR_VARS)
LOCAL_MODULE := termux-baremetal
ifeq ($(RMR_PURE_CORE),1)
LOCAL_SRC_FILES := lowlevel/baremetal_nomalloc.c
LOCAL_CFLAGS += -DRAFAELIA_NO_MALLOC=1
LOCAL_CFLAGS += -DRMR_PURE_CORE=1
LOCAL_CFLAGS += -DRMR_NO_HEAP=1
LOCAL_CFLAGS += -DRMR_NO_STDIO=1
LOCAL_CFLAGS += -DRMR_NO_LIBM=1
LOCAL_CFLAGS += -DRMR_NO_DEBUG_STRING=1
LOCAL_CFLAGS += -DRMR_USE_Q16=1
LOCAL_CFLAGS += -DRMR_ENABLE_ASM=1
LOCAL_CFLAGS += -DRMR_ENABLE_BRANCHLESS=1
LOCAL_CFLAGS += -fvisibility=hidden
LOCAL_CFLAGS += -fno-unwind-tables
LOCAL_CFLAGS += -fno-asynchronous-unwind-tables
LOCAL_CFLAGS += -fno-ident
else
ifeq ($(RAFAELIA_NO_MALLOC),1)
LOCAL_SRC_FILES := lowlevel/baremetal_nomalloc.c
LOCAL_CFLAGS += -DRAFAELIA_NO_MALLOC=1
else
LOCAL_SRC_FILES := lowlevel/baremetal.c
endif
endif
LOCAL_SRC_FILES += lowlevel/baremetal_jni.c lowlevel/rafaelia_gpu_orchestrator.c lowlevel/rafaelia_commit_gate_ll.c lowlevel/bootstrap_baremetal_guard.c lowlevel/bootstrap_baremetal_jni.c
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES += lowlevel/baremetal_asm.S
    LOCAL_CFLAGS += -DHAS_BM_NEON_ASM=1
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES += lowlevel/baremetal_asm.S
    LOCAL_CFLAGS += -DHAS_BM_NEON_ASM=1
endif
LOCAL_CFLAGS += -std=c11 -Wall -Wextra -Werror -Os -fno-stack-protector -fno-common
LOCAL_CFLAGS += -ffast-math
LOCAL_CFLAGS += -ffunction-sections -fdata-sections
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384

ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_CFLAGS += -march=armv7-a -mfloat-abi=softfp -mfpu=neon -ftree-vectorize
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
LOCAL_LDLIBS := -llog -ldl
ifneq ($(RMR_NO_LIBM),1)
LOCAL_LDLIBS += -lm
endif
include $(BUILD_SHARED_LIBRARY)

# RAFAELIA direct JNI helper (legacy app bridge; not used by PA ELF)
include $(CLEAR_VARS)
LOCAL_MODULE := termux_rafaelia_direct
LOCAL_SRC_FILES := lowlevel/rafaelia_jni_direct.c lowlevel/raf_vcpu.c lowlevel/raf_clock.c lowlevel/raf_memory_layers.c lowlevel/raf_bitraf.c lowlevel/raf_gp_dimension.c
ifneq ($(RMR_NO_DEBUG_STRING),1)
LOCAL_SRC_FILES += lowlevel/raf_bitraf_debug.c
endif
ifeq ($(RMR_PURE_CORE),1)
LOCAL_CFLAGS += -DRMR_NO_DEBUG_STRING=1
endif
LOCAL_CFLAGS += -std=c11 -Wall -Wextra -Os -fno-stack-protector -fno-common
LOCAL_CFLAGS += -ffunction-sections -fdata-sections
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384
LOCAL_LDLIBS := -llog
ifneq ($(RMR_NO_LIBM),1)
LOCAL_LDLIBS += -lm
endif
include $(BUILD_SHARED_LIBRARY)

# api_lowlevel — existing API bridge only. PA benchmark JNI was removed.
include $(CLEAR_VARS)
LOCAL_MODULE := api_lowlevel
LOCAL_SRC_FILES := \
    lowlevel/api_lowlevel.c \
    lowlevel/api_jni_bridge.c
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES += lowlevel/api_ll_asm.S
    LOCAL_CFLAGS += -march=armv8-a+crc+simd -DHAS_CRC32C_HW=1 -DHAS_NEON=1
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES += lowlevel/api_ll_asm.S
    LOCAL_CFLAGS += -march=armv7-a -mfpu=neon -DHAS_NEON=1
endif
LOCAL_CFLAGS += -std=c11 -O3 -fno-stack-protector -fvisibility=hidden -fno-common
LOCAL_CFLAGS += -ffunction-sections -fdata-sections
LOCAL_CFLAGS += -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-ident
LOCAL_CFLAGS += -DAPI_LL_NOMALLOC=1
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384
LOCAL_LDLIBS := -llog
include $(BUILD_SHARED_LIBRARY)

# PA silicon core — executable ET_DYN loaded directly by Android linker.
# Headerless C + architecture entry ASM. No JNI, libc, malloc or DT_NEEDED.
ifneq ($(filter $(TARGET_ARCH_ABI),armeabi-v7a arm64-v8a),)
include $(CLEAR_VARS)
LOCAL_MODULE := raf_pa_core
LOCAL_SRC_FILES := freestanding/raf_pa_core.c
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES += freestanding/raf_pa_entry_arm64.S
    LOCAL_CFLAGS += -march=armv8-a
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES += freestanding/raf_pa_entry_arm32.S
    LOCAL_CFLAGS += -march=armv7-a -mfloat-abi=softfp -mfpu=neon-vfpv4
endif
LOCAL_CFLAGS += -std=c11 -O3 -fPIC -ffreestanding -fno-builtin
LOCAL_CFLAGS += -fno-stack-protector -fno-common -fvisibility=hidden
LOCAL_CFLAGS += -ffunction-sections -fdata-sections
LOCAL_CFLAGS += -fno-unwind-tables -fno-asynchronous-unwind-tables -fno-ident
LOCAL_LDFLAGS := -nostdlib -nodefaultlibs -Wl,--no-undefined
LOCAL_LDFLAGS += -Wl,--gc-sections -Wl,--build-id=none -Wl,-e,_start
LOCAL_LDFLAGS += -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384
LOCAL_SYSTEM_SHARED_LIBRARIES :=
LOCAL_LDLIBS :=
LOCAL_ALLOW_UNDEFINED_SYMBOLS := false
LOCAL_STRIP_MODULE := true
include $(BUILD_SHARED_LIBRARY)
endif

# RAFAELIA ZERO runtime — same canonical source compiled directly into the APK.
include $(CLEAR_VARS)
LOCAL_MODULE := termux_rafaelia_zero_runtime
LOCAL_SRC_FILES := \
    ../../../../rafaelia/src/main/cpp/zero/rafz.c \
    lowlevel/rafaelia_zero_runtime_jni.c
LOCAL_C_INCLUDES := $(LOCAL_PATH)/../../../../rafaelia/src/main/cpp/zero/include
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
