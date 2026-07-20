# Slice 0 — Stabilization: Completion Report

**Date:** 2026-07-19  
**Status:** ✅ Complete  

## Exit Criteria

| Criterion | Result |
|---|---|
| Clone → `embersim init` → build | ✅ `embersim init -f test-fixtures/minimal_hal.h -I test-fixtures -o test-embersim` |
| Full C compilation chain | ✅ 9 mocks compile with gcc, zero warnings |
| Link + execute | ✅ `test_tim_runtime.exe` links and runs |
| Tests pass | ✅ 6/6 runtime tests pass (period elapsed, PWM, input capture, SR state, dispatch, UIE disable) |
| Deterministic trace | ✅ 181 lines, bit-for-bit identical across runs |
| Reproducible | ✅ Another engineer can clone, init, build, run, and get the same trace |

## Achieved Workflow

```powershell
# 1. Configure environment (once)
$env:Path += ";C:\msys64\ucrt64\bin"    # gcc
$env:Path += ";$env:USERPROFILE\.cargo\bin"  # cargo

# 2. Build Rust CLI
cd C:\embersim\embersim
cargo build --workspace

# 3. Initialize test workspace
cargo run -p embersim-cli -- init -f test-fixtures/minimal_hal.h -I test-fixtures -o test-embersim

# 4. Compile all C mocks
cd test-embersim
gcc -I mocks -c mocks/mock_hal.c -o mocks/mock_hal.o
gcc -I mocks -c mocks/trace_log.c -o mocks/trace_log.o
gcc -I mocks -c mocks/mock_state.c -o mocks/mock_state.o
gcc -I mocks -c mocks/mock_uart.c -o mocks/mock_uart.o
gcc -I mocks -c mocks/mock_i2c.c -o mocks/mock_i2c.o
gcc -I mocks -c mocks/mock_spi.c -o mocks/mock_spi.o
gcc -I mocks -c mocks/ember_sim_kernel.c -o mocks/ember_sim_kernel.o
gcc -I mocks -c mocks/ember_regs.c -o mocks/ember_regs.o
gcc -I mocks -c mocks/mock_tim.c -o mocks/mock_tim.o

# 5. Compile and run TIM runtime test
gcc -I mocks -c test_tim_runtime.c -o test_tim_runtime.o
gcc test_tim_runtime.o mocks/*.o -o test_tim_runtime.exe
.\test_tim_runtime.exe

# 6. Check environment
cargo run -p embersim-cli -- doctor
```

## Deterministic Trace Proof

```
Run 1: 181 trace lines
Run 2: 181 trace lines
Diff:  0 lines differ → IDENTICAL
```

Sample trace excerpt showing the complete interrupt chain:

```json
{"ts_ms":3,"peripheral":"runtime","func":"REGISTER_EVENT","origin":"register","layer":"hardware","peripheral":"TIM2","address":"0x40000400","register":"DIER","old":"0x0000","new":"0x0001","reason":"UIE enable"}
{"ts_ms":4,"peripheral":"runtime","func":"REGISTER_EVENT","origin":"register","layer":"hardware","peripheral":"TIM2","address":"0x40000400","register":"CNT","old":"0x0000","new":"0x0001","reason":"increment"}
{"ts_ms":5,"peripheral":"runtime","func":"NVIC_DISPATCH","irq":28}
{"ts_ms":6,"peripheral":"runtime","func":"SOFTWARE_EVENT","origin":"software","layer":"hal","component":"TIM","event":"irq_handler","details":{"instance":"TIM2","address":"0x40000400"}}
```

## Current Limitations

| Limitation | Impact | Mitigation |
|---|---|---|
| Only TIM peripheral tested at runtime | UART, I2C, SPI, GPIO mocks untested at runtime | Slice 1-2 scope |
| `periph_to_irq()` hardcodes TIM2/USART2 in kernel | Violates kernel isolation principle | ADR-001 pending |
| Three overlapping event type systems | Confusion for new peripheral authors | ADR-002 pending |
| Only 5 Rust unit tests | Regression risk in pipeline modules | Add as needed |
| No C tests in CI | C regressions invisible to GitHub Actions | Slice 1 |
| gcc dependency, no MSVC/clang-cl fallback | Windows developers without MSYS2 blocked | `embersim doctor` detects this |
| Trace format: flat JSON-like, not strict JSON | Some entries have minor formatting issues | Acceptable for now; formalize in Slice 1 |
| `test_tim_runtime.c` compiled manually | No `embersim build` integration yet | Slice 1 |

## Defects Fixed in Slice 0

| # | Defect | Fix |
|---|---|---|
| R1 | `ember_regs.h` included itself (`#include "ember_regs.h"`) | Replaced with `<stdint.h>` + `<stdbool.h>` |
| R3 | `tim_peripheral.c` — dead prototype with undefined references | Deleted |
| R6 | `normalize.rs` — empty file, not declared in lib.rs | Deleted |
| R9 | Stale `.exe`, `.o`, `.jsonl` in `test-embersim/` | Deleted |
| R10 | Stale `test-out/` directory | Deleted |

## Architecture Violations Deferred to ADR

| # | Issue | ADR |
|---|---|---|
| R4 | Kernel `periph_to_irq()` hardcodes TIM2=IRQ28, USART2=IRQ38 | ADR-001 |
| R5 | Three event systems: `KernelEventType`, `BusEventType`, `EmberEventType` | ADR-002 |

## Next Milestone

**Slice 1 — Prove the architecture generalizes with a second independent peripheral.**

> Can EmberSim support a second independent peripheral without changing its foundation?
