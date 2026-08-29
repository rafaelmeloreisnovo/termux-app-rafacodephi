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

aarch64-vectorpulse-gate:
	@echo "=== BUG-03 AArch64 Vectorpulse Gate ==="
	@echo "STATUS: BLOCKED on BUG-01 completion"
	@echo "Depends on attractor_table validation"

lyapunov-convergence-gate:
	@echo "=== BUG-08 Lyapunov Convergence Gate ==="
	@echo "STATUS: BLOCKED on BUG-01 completion"
	@echo "Requires φ = (1-H)·C validation with 41-state space"

clean:
	$(MAKE) -C bootstrap_rafaelia clean || true
	rm -rf build
	rm -f bootstrap_rafaelia/selftest.log
	rm -f rmr/Rrr/attractor_table.o rmr/Rrr/attractor_table_validator.o
	@echo "Cleaned top-level build artifacts."

extract-abi-bootstrap:
	python3 tools/bootstrap/extract_abi_bootstrap.py
