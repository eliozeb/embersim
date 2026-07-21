# Pilot 0 Reference Validation Project (RVP-001) — Makefile
#
# Release-gating conformance build. Must compile with zero warnings under
# both GCC and Clang. Supports sanitizer and static analysis targets.
#
# Usage:
#   make                  Release build with strict warnings
#   make CC=clang         Build with Clang
#   make asan             Build with AddressSanitizer
#   make ubsan            Build with UndefinedBehaviorSanitizer
#   make analyze          Run cppcheck static analysis
#   make clean            Remove build artifacts

CC       ?= gcc

# Strict warning policy — zero warnings required for release.
# -Wno-cast-align is suppressed because STM32 HAL register address casts
#   (e.g. GPIOA = (GPIO_TypeDef *)0x40020000U) are an inherent pattern
#   in embedded firmware and cannot be avoided without losing realism.
# -Wno-double-promotion is suppressed because float-to-double promotion
#   in variadic functions (snprintf, etc.) is required by the C standard
#   and is not a defect.
CFLAGS   := -std=c11 -O0 -g \
            -Wall -Wextra -Wpedantic -Werror \
            -Wshadow -Wconversion \
            -Wformat=2 -Wundef -Wstrict-prototypes \
            -Wmissing-prototypes -Wmissing-declarations \
            -Wno-cast-align -Wno-double-promotion

INC      := -Ifirmware/Core/Inc \
            -Ifirmware/Drivers/STM32F4xx_HAL_Driver/Inc
BUILD_DIR := build
TARGET   := $(BUILD_DIR)/rvp-001

# Firmware source files (compiled by both native and EmberSim builds)
FW_SRC   := firmware/Core/Src/board.c \
            firmware/Core/Src/app.c \
            firmware/Core/Src/uart_service.c \
            firmware/Core/Src/motor_control.c \
            firmware/Core/Src/sensor.c \
            firmware/Core/Src/main.c

# HAL stub source (native build only — not compiled by EmberSim)
HAL_SRC  := firmware/Drivers/STM32F4xx_HAL_Driver/Src/stm32f4xx_hal.c

SRCS     := $(FW_SRC) $(HAL_SRC)
OBJS     := $(patsubst %.c,$(BUILD_DIR)/%.o,$(SRCS))

.PHONY: all clean asan ubsan analyze clang

# ---- Default release build ----
all: $(TARGET)

$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CC) $(CFLAGS) $(OBJS) -o $@ -lm
	@echo "Build successful: $(TARGET)"

$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) $(INC) -c $< -o $@

# ---- Clang build ----
clang:
	$(MAKE) CC=clang

# ---- AddressSanitizer build ----
asan: CFLAGS += -fsanitize=address -fno-omit-frame-pointer
asan: clean all
	@echo "ASan build complete. Run with: ASAN_OPTIONS=detect_leaks=1 $(TARGET) trace.jsonl"

# ---- UndefinedBehaviorSanitizer build ----
ubsan: CFLAGS += -fsanitize=undefined -fno-omit-frame-pointer
ubsan: clean all
	@echo "UBSan build complete. Run: $(TARGET) trace.jsonl"

# ---- Static analysis (cppcheck) ----
analyze:
	@echo "Running cppcheck..."
	cppcheck --enable=all --inconclusive --suppress=missingIncludeSystem \
	         --error-exitcode=1 $(INC:%=%) $(FW_SRC) $(HAL_SRC)
	@echo "cppcheck: PASS"

# ---- Clean ----
clean:
	rm -rf $(BUILD_DIR)
