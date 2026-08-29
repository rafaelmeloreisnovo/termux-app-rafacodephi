CC ?= clang
CFLAGS_BASE ?= -O2 -fno-strict-aliasing -Wall -Wextra -Werror=implicit-function-declaration
ARCH ?= host
EXTRA_CFLAGS ?=

.PHONY: all clean diagnose selftest extract-abi-bootstrap attractor-table-complete-gate attractor-coherence-gate aarch64-vectorpulse-gate lyapunov-convergence-gate

all: diagnose

diagnose:
	$(MAKE) -C bootstrap_rafaelia clean
	$(MAKE) -C bootstrap_rafaelia CC=$(CC) ARCH=$(ARCH) CFLAGS_COMMON="$(CFLAGS_BASE) $(EXTRA_CFLAGS) -I." selftest

selftest: diagnose

attractor-table-complete-gate: rmr/Rrr/attractor_table.c rmr/Rrr/attractor_table.h rmr/Rrr/attractor_table_validator.c
	@echo "=== BUG-01 Attractor Table Complete Gate ==="
	@mkdir -p build
	$(CC) $(CFLAGS_BASE) -I rmr/Rrr -I rmr/include \
		rmr/Rrr/attractor_table.c rmr/Rrr/attractor_table_validator.c -o build/attractor_validator
	./build/attractor_validator
	@echo "✅ BUG-01 closure gate: PASS"

attractor-coherence-gate:
	@echo "=== BUG-02 Attractor Coherence Gate (Decision Point) ==="
	@grep -q "Option 1 — Remove Attractor #22" docs/BUG02_DECISION_RECORD.md && echo "✅ BUG-02 decision: IMPLEMENTED" || echo "❌ BUG-02 decision: PENDING"

aarch64-vectorpulse-gate: rmr/Rrr/vectra_pulse.S rmr/Rrr/vectra_pulse.h rmr/Rrr/vectra_pulse_validator.c
	@echo "=== BUG-03 AArch64 Vectorpulse Gate ==="
	@mkdir -p build
	$(CC) $(CFLAGS_BASE) -I rmr/Rrr -I rmr/include \
		rmr/Rrr/vectra_pulse_validator.c -o build/vectra_pulse_validator
	./build/vectra_pulse_validator
	@echo "✅ BUG-03 closure gate: PASS"

lyapunov-convergence-gate: rmr/Rrr/lyapunov_convergence.c rmr/Rrr/lyapunov_convergence.h rmr/Rrr/lyapunov_convergence_validator.c
	@echo "=== BUG-08 Lyapunov Convergence Gate ==="
	@mkdir -p build
	$(CC) $(CFLAGS_BASE) -I rmr/Rrr -I rmr/include \
		rmr/Rrr/lyapunov_convergence.c rmr/Rrr/lyapunov_convergence_validator.c -o build/lyapunov_validator -lm
	./build/lyapunov_validator
	@echo "✅ BUG-08 closure gate: PASS"

clean:
	$(MAKE) -C bootstrap_rafaelia clean || true
	rm -rf build
	rm -f bootstrap_rafaelia/selftest.log
	rm -f rmr/Rrr/attractor_table.o rmr/Rrr/attractor_table_validator.o
	rm -f rmr/Rrr/vectra_pulse_validator.o rmr/Rrr/lyapunov_convergence.o rmr/Rrr/lyapunov_convergence_validator.o
	@echo "Cleaned top-level build artifacts."

extract-abi-bootstrap:
	python3 tools/bootstrap/extract_abi_bootstrap.py
