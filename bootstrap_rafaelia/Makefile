CC ?= cc
CFLAGS_COMMON := -Wall -Wextra -I.

.PHONY: host-smoke selftest bootstrap_rafaelia_selftest arm64-freestanding
host-smoke: selftest

selftest: bootstrap_rafaelia_selftest

bootstrap_rafaelia_selftest:
	$(CC) $(CFLAGS_COMMON) raf_selftest.c raf_main.c -o raf_selftest
	./raf_selftest

arm64-freestanding:
	clang --target=aarch64-linux-android24 -ffreestanding -fno-builtin -nostdlib -nostdinc -I. -c raf_main.c -o raf_main.o
	clang --target=aarch64-linux-android24 -ffreestanding -nostdlib -c entry_arm64.S -o entry_arm64.o
	@echo "arm64-freestanding objects generated: raf_main.o entry_arm64.o"
