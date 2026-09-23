ARCH ?= x86_64
BUILD_DIR ?= build

.PHONY: all clean

all:
	@echo "Toolchain not configured yet. See OS_MASTER_SPEC.md section 5 / section 8 (Phase 0)."

clean:
	rm -rf $(BUILD_DIR)
