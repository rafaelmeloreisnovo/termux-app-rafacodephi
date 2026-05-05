CC ?= cc
CFLAGS_COMMON := -Wall -Wextra -I.
ANDROID_API ?= 24

.PHONY: host-smoke selftest bootstrap_rafaelia_selftest arm64-freestanding clean
host-smoke: selftest

selftest: bootstrap_rafaelia_selftest

bootstrap_rafaelia_selftest:
	$(CC) $(CFLAGS_COMMON) raf_selftest.c raf_main.c -o raf_selftest
	./raf_selftest | tee selftest.log

arm64-freestanding:
	clang --target=aarch64-linux-android$(ANDROID_API) -ffreestanding -fno-builtin -nostdlib -nostdinc -I. -c raf_main.c -o raf_main.o
	clang --target=aarch64-linux-android$(ANDROID_API) -ffreestanding -nostdlib -c entry_arm64.S -o entry_arm64.o
	@echo "arm64-freestanding objects generated: raf_main.o entry_arm64.o"

clean:
	rm -f raf_selftest raf_main.o entry_arm64.o selftest.log
