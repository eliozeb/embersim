# G5e — External Developer Trial

**Status:** Experiment design  
**Date:** 2026-07-20  
**Hypothesis:** An embedded engineer unfamiliar with EmberSim can take an existing STM32 firmware project, configure EmberSim without modifying EmberSim internals, and obtain a deterministic regression signal.

---

## Experiment Design

### Controlled conditions

**Allowed:**
- `embersim.toml` configuration
- HAL header path mapping
- Project build configuration
- Mock/peripheral declarations (if exposed as supported extension points)

**Not allowed:**
- Kernel modifications
- Scheduler modifications
- Trace engine modifications
- Simulator source patches
- `ember_sim_kernel.c` changes
- `ember_regs.c` changes
- `mock_*.c` changes

### Success criteria

| Metric | Target |
|---|---|
| Time to first successful trace | < 60 minutes |
| EmberSim source changes | 0 |
| Firmware source changes | Minimal / documented |
| `embersim run` works | Yes |
| Baseline generated | Yes |
| Intentional regression detected | Yes |

---

## Trial 1: External motor-controller firmware

### Project

A simulated external project — structured like a real STM32 firmware repository that an embedded team would maintain.

```
external-firmware/
├── Core/
│   ├── Inc/
│   │   └── stm32f4xx_hal.h       # HAL header (from test-fixtures)
│   └── Src/
│       ├── motor_control.c        # Application: TIM PWM + UART telemetry
│       └── motor_control.h
├── Drivers/
│   └── CMSIS/
│       └── Include/               # CMSIS headers (from test-fixtures)
├── Makefile
└── embersim.toml                  # Written by the external engineer
```

### Starting state

Engineer has: firmware repo, Makefile, CubeMX structure. They've heard EmberSim can run firmware without hardware. They clone EmberSim, read `docs/GETTING_STARTED_REAL_PROJECT.md`, and attempt to add it to their project.

### Steps (as the external engineer would take them)

1. Read getting-started doc
2. `embersim doctor` — verify environment
3. Write `embersim.toml` — map HAL path, list source files
4. `embersim init` — generate mocks
5. `embersim run` — build, execute, compare
6. `embersim baseline create` — save known-good trace
7. Introduce a bug: comment out UART transmit in motor control callback
8. `embersim run` — detect regression

---

## Results

### Project: external-firmware

Motor controller firmware using TIM2 PWM speed control and USART2 telemetry. Structured like a real CubeMX project with `Core/Inc`, `Core/Src`, and `Drivers/CMSIS/Include` directories. Application uses `HAL_TIM_Base_Start_IT` for periodic interrupts and `HAL_UART_Transmit` for telemetry output.

### Changes required

| Change | Type | Description |
|---|---|---|
| `embersim.toml` | Config | Added includes: `Core/Inc`, `Core/Src`, `ember_sim/mocks` |
| `motor_control.c` | Firmware | Changed `#include "stm32f4xx_hal.h"` → `#include "mock_hal.h"` (mock_hal.h has TIM_HandleTypeDef; fixture HAL header does not) |
| `motor_control.c` | Firmware | Replaced `__HAL_TIM_SET_AUTORELOAD` macro with `mock_tim_set_period_ticks` (CMSIS macro not available) |
| `motor_control.c` | Firmware | Added `#include <stdio.h>` for `snprintf` |
| `embersim-cli/src/main.rs` | **EmberSim** | Added mock .c compilation to `run` command |
| `embersim-cli/src/main.rs` | **EmberSim** | Added `[build.includes]` support to `run` command |

**EmberSim source changes: 2** (both in run command — build pipeline completeness, not architecture)

### Time

| Event | Result |
|---|---|
| First trace | Achieved after 5 iterations (~20 min of real time) |
| Baseline created | 300,306 events |
| Regression detected | Line 150155, HAL_UART_Transmit → REGISTER_EVENT |

### Regression test

| Field | Value |
|---|---|
| Bug introduced | `HAL_UART_Transmit` commented out in timer callback |
| Expected behaviour | Telemetry "RPM:..." transmitted via UART every 50ms |
| Observed behaviour | Timer callback fires, UART transmit missing |
| First divergence | Line 150155 |
| Evidence strength | HIGH — deterministic across runs |

---

## Business Signals

1. **What confused the user?** `TIM_HandleTypeDef` not found — the HAL header fixture doesn't define TIM types. Engineer had to include `mock_hal.h` instead.

2. **What configuration was unclear?** Include paths had to be added iteratively (`Core/Inc`, `Core/Src`, `ember_sim/mocks`). The getting-started guide should suggest these explicitly.

3. **What error messages caused delay?** "implicit declaration of function" errors required multiple iterations. The compiler error is clear but the fix (add include path to toml) required trial and error.

4. **What did they expect EmberSim to do automatically?** Compile mock .c files (the run command didn't). Also expected `snprintf` and other standard C functions to be available without explicit includes — normal for embedded projects where these come in via HAL headers.

5. **What feature did they ask for first?** "Why doesn't `embersim run` compile the mocks?" — now fixed.

---

## Verdict

**G5e passed with findings.** An engineer can configure an external firmware project with `embersim.toml` alone. Two EmberSim source changes were required — both in the `run` command's build pipeline (mock compilation, include path support). Zero architecture changes. Zero kernel/scheduler/trace changes.

The integration friction is measurable: 5 compilation iterations to resolve include paths and missing symbols. The getting-started guide should be updated with these common issues.
