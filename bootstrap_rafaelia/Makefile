CC ?= cc
CFLAGS_COMMON := -Wall -Wextra -I.
ANDROID_API ?= 24
NDK_CLANG ?= clang
ARCH ?= arm64

# Supported experimental freestanding targets:
# arm32, arm64, x86, x86_64, riscv64, armv7a-neon
ifeq ($(ARCH),arm32)
TARGET_TRIPLE := armv7a-linux-androideabi$(ANDROID_API)
ENTRY_ASM :=
else ifeq ($(ARCH),arm64)
TARGET_TRIPLE := aarch64-linux-android$(ANDROID_API)
ENTRY_ASM := entry_arm64.S
else ifeq ($(ARCH),x86)
TARGET_TRIPLE := i686-linux-android$(ANDROID_API)
ENTRY_ASM :=
else ifeq ($(ARCH),x86_64)
TARGET_TRIPLE := x86_64-linux-android$(ANDROID_API)
ENTRY_ASM :=
else ifeq ($(ARCH),riscv64)
TARGET_TRIPLE := riscv64-linux-android35
ENTRY_ASM :=
else ifeq ($(ARCH),armv7a-neon)
TARGET_TRIPLE := armv7a-linux-androideabi$(ANDROID_API)
ENTRY_ASM :=
ARCH_CFLAGS := -mfpu=neon
else
$(error Unsupported ARCH=$(ARCH))
endif

.PHONY: host-smoke selftest bootstrap_rafaelia_selftest arm-freestanding termux-compat termux-compat-arm32 termux-compat-arm64 clean
host-smoke: selftest

selftest: bootstrap_rafaelia_selftest

bootstrap_rafaelia_selftest:
	$(CC) $(CFLAGS_COMMON) raf_selftest.c raf_main.c -o raf_selftest
	./raf_selftest | tee selftest.log

arm-freestanding:
	$(NDK_CLANG) --target=$(TARGET_TRIPLE) -ffreestanding -fno-builtin -nostdlib -nostdinc $(ARCH_CFLAGS) -I. -c raf_main.c -o raf_main.$(ARCH).o
	@if [ -n "$(ENTRY_ASM)" ]; then \
	  $(NDK_CLANG) --target=$(TARGET_TRIPLE) -ffreestanding -nostdlib -c $(ENTRY_ASM) -o entry_$(ARCH).o; \
	fi
	@echo "freestanding objects generated for $(ARCH) target $(TARGET_TRIPLE)"

clean:
	rm -f raf_selftest selftest.log raf_main.*.o entry_*.o libbootstrap_rafaelia_*.a

termux-compat-arm32:
	$(NDK_CLANG) --target=armv7a-linux-androideabi$(ANDROID_API) -ffreestanding -fno-builtin -nostdlib -nostdinc -I. -c raf_main.c -o raf_main.termux.arm32.o
	llvm-ar rcs libbootstrap_rafaelia_arm32.a raf_main.termux.arm32.o

termux-compat-arm64:
	$(NDK_CLANG) --target=aarch64-linux-android$(ANDROID_API) -ffreestanding -fno-builtin -nostdlib -nostdinc -I. -c raf_main.c -o raf_main.termux.arm64.o
	llvm-ar rcs libbootstrap_rafaelia_arm64.a raf_main.termux.arm64.o

termux-compat: termux-compat-arm32 termux-compat-arm64
	bash ./raf_termux_compat.sh

