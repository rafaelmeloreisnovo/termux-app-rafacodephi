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
# Assembly optimizations enabled when the target ABI guarantees SIMD support
ifeq ($(TARGET_ARCH_ABI),arm64-v8a)
    LOCAL_SRC_FILES += lowlevel/baremetal_asm.S
    LOCAL_CFLAGS += -DHAS_BM_NEON_ASM=1
endif
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    LOCAL_SRC_FILES += lowlevel/baremetal_asm.S
    LOCAL_CFLAGS += -DHAS_BM_NEON_ASM=1
endif
LOCAL_CFLAGS += -std=c11 -Wall -Wextra -Werror -Os -fno-stack-protector
LOCAL_CFLAGS += -ffast-math
LOCAL_CFLAGS += -ffunction-sections -fdata-sections
# Critical: 16KB page alignment for Android 15/16 compatibility
LOCAL_LDFLAGS := -Wl,--gc-sections -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384

# Architecture-specific optimizations
ifeq ($(TARGET_ARCH_ABI),armeabi-v7a)
    # Keep ARM32 baseline-compatible and rely on runtime capability checks.
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

# Link against log and math libraries
LOCAL_LDLIBS := -llog -ldl
ifneq ($(RMR_NO_LIBM),1)
LOCAL_LDLIBS += -lm
endif
include $(BUILD_SHARED_LIBRARY)


# RAFAELIA direct JNI helper (zero-copy DirectByteBuffer path)
include $(CLEAR_VARS)
LOCAL_MODULE := termux_rafaelia_direct
LOCAL_SRC_FILES := lowlevel/rafaelia_jni_direct.c lowlevel/raf_vcpu.c lowlevel/raf_clock.c lowlevel/raf_memory_layers.c lowlevel/raf_bitraf.c lowlevel/raf_gp_dimension.c
ifneq ($(RMR_NO_DEBUG_STRING),1)
LOCAL_SRC_FILES += lowlevel/raf_bitraf_debug.c
endif
ifeq ($(RMR_PURE_CORE),1)
LOCAL_CFLAGS += -DRMR_NO_DEBUG_STRING=1
endif
LOCAL_CFLAGS += -std=c11 -Wall -Wextra -Os -fno-stack-protector
LOCAL_LDFLAGS := -Wl,-z,max-page-size=16384 -Wl,-z,common-page-size=16384
LOCAL_LDLIBS := -llog
ifneq ($(RMR_NO_LIBM),1)
LOCAL_LDLIBS += -lm
endif
include $(BUILD_SHARED_LIBRARY)
