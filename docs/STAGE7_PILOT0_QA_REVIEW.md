# Independent QA Review — Pilot 0 Reference Firmware

**Review type:** Technical Due Diligence / Engineering Design Review
**Reviewer role:** Independent Principal Embedded Systems Engineer
**Date:** 2026-07-21
**Scope:** `test-fixtures/projects/pilot-reference/` — all 15 files
**Standard:** Professional embedded reference project (not MISRA/ISO 26262)

---

## Executive Summary

The Pilot 0 reference firmware is a well-structured, thoughtfully designed validation fixture with clear architectural intent. It exercises 15 HAL functions across 4 peripherals in a representative industrial pattern (motor controller + sensor node). However, it contains **one Critical finding** — the include strategy for EmberSim mock types is architecturally unverified — and several Major findings around type compatibility, dead code, and missing regression coverage that must be resolved before this fixture can serve as EmberSim's permanent internal validation reference.

**Preliminary recommendation:** APPROVE WITH MAJOR CHANGES. The architecture is sound but the HAL/mock type boundary must be proven correct through actual `embersim init && embersim run` execution. Without this, the fixture produces false confidence.

---

## 1. Architecture

### 1.1 Module Structure

```
main.c          — entry, SystemClock, MX_* init, peripheral handles (globals)
app.c           — state machine, command dispatch, TIM callback
uart_service.c  — UART RX/TX, command parsing
motor_control.c — TIM PWM motor start/stop
sensor.c        — SPI temperature read
```

**Separation of responsibilities:** Good. Each module owns a clear domain. `app.c` is the only module that coordinates across subsystems.

**Coupling assessment:**

| Coupling | Mechanism | Assessment |
|----------|-----------|-----------|
| app → uart_service | Function calls + shared buffer | Acceptable |
| app → motor_control | Function calls + shared state via mutators | Acceptable |
| app → sensor | Function call | Good — single consumer |
| All → main.c | `extern` peripheral handles | **Fragile** — no single definition point checked by compiler |

**Finding A1 — Fragile extern coupling (Major)**

`huart2`, `htim2`, `hspi1` are defined in `main.c` and redeclared via `extern` in `app.c` (implicitly, through callbacks), `uart_service.c` (line 22), `motor_control.c` (line 15), `sensor.c` (line 16). Each module independently declares the extern. If the type changes in `main.c`, the other modules get link errors, not compile errors.

**Evidence:** `uart_service.c:22`: `extern UART_HandleTypeDef huart2;` — independently declared. `motor_control.c:15`: `extern TIM_HandleTypeDef htim2;`. No shared header validates these match.

**Recommendation:** Move extern declarations to a shared `peripherals.h` or declare them in the module headers (`uart_service.h`, `motor_control.h`, `sensor.h`) rather than in `.c` files. This is standard embedded practice.

### 1.2 Initialization Order

```
HAL_Init → SystemClock_Config → MX_GPIO_Init → MX_USART2 → MX_TIM2 → MX_SPI1 → app_init
  → uart_service_init → motor_init → HAL_TIM_Base_Start_IT
```

**Assessment:** Correct. TIM is configured before interrupts are enabled. UART RX is armed after init. No circular dependencies.

**Finding A2 — Double initialization of TIM PWM (Minor)**

`MX_TIM2_Init()` calls `HAL_TIM_Base_Init()` (line 96). `motor_init()` calls `HAL_TIM_PWM_Init()` (line 32). In the real STM32 HAL, these are the same function — `HAL_TIM_PWM_Init` is a thin wrapper. Calling both on the same handle is redundant. In EmberSim's `mock_tim.c`, the behavior depends on the mock implementation. If the mock resets state on `HAL_TIM_PWM_Init`, the `PWM_ConfigChannel` call from `MX_TIM2_Init` is undone.

**Evidence:** `main.c:96` and `motor_control.c:32` both initialize `htim2`.

**Recommendation:** Consolidate TIM initialization. Either call `HAL_TIM_PWM_Init` in `MX_TIM2_Init` (replacing `HAL_TIM_Base_Init`) or remove the call from `motor_init`.

---

## 2. HAL Usage

### 2.1 GPIO

| Call site | Function | Assessment |
|-----------|----------|-----------|
| `main.c:64,70` | `HAL_GPIO_Init` | Correct — fields set before each call |
| `app.c:63` | `HAL_GPIO_WritePin` | Correct |
| `app.c:117` | `HAL_GPIO_ReadPin` | Correct |
| `app.c:140` | `HAL_GPIO_TogglePin` | Correct |

**Finding H1 — `MX_GPIO_Init` reuses struct without clearing (Minor)**

`main.c:57` zeroes `gpio` with `{0}`, then sets fields for PA5, calls `HAL_GPIO_Init`. Then sets fields for PA0 but does NOT re-zero the struct. Fields from PA5 config (`Speed`, `Alternate`) persist in PA0 config. On real hardware, `HAL_GPIO_Init` uses only the fields relevant to the pin configuration, so this is safe — but it's a code smell. CubeMX always re-initializes the struct for each pin.

**Evidence:** `main.c:57-70`: `gpio` is zeroed once, then reused.

**Recommendation:** Add `gpio = (GPIO_InitTypeDef){0};` before the PA0 configuration. Standard CubeMX pattern.

### 2.2 UART

**Finding H2 — `HAL_UART_Receive_IT` called with Size=1, but `HAL_UART_RxCpltCallback` calls it again with Size=1 (Minor)**

This is standard STM32 UART IT pattern — receive one byte at a time, callback re-arms. However, `uart_service_init()` (line 43) also calls `HAL_UART_Receive_IT(&huart2, rx_buf, 1)`. This means the first byte goes to `rx_buf[0]`. The callback (line 115) re-arms with `&rx_buf[rx_len]`. If `rx_len` was incremented to 1 (line 109), the second byte goes to `rx_buf[1]`. This is correct — standard pattern.

**Assessment:** PASS. This is exactly how STM32 UART IT reception works in CubeMX-generated code.

### 2.3 TIM

**Finding H3 — `HAL_TIM_Base_Start_IT` called before `motor_init` (Major-ish)**

`app_init()` calls `HAL_TIM_Base_Start_IT(&htim2)` (line 60) BEFORE calling `motor_init()` (line 57). Wait — let me re-check the order. `app.c:55` calls `uart_service_init()`, line 57 calls `motor_init()`, line 60 calls `HAL_TIM_Base_Start_IT`. So `motor_init()` IS called before `HAL_TIM_Base_Start_IT`. Correct.

**Assessment:** PASS. Order is correct.

Wait — actually, let me re-read `app_init()`:

```c
void app_init(void)
{
    g_state.tick_count    = 0;
    g_state.motor_running = 0;
    g_state.temperature   = SPI_TEMP_FIXED;
    g_state.led_state     = 0;

    uart_service_init();
    motor_init();

    HAL_TIM_Base_Start_IT(&htim2);

    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}
```

`motor_init()` is called at line 57, then `HAL_TIM_Base_Start_IT` at line 60. But `motor_init()` calls `HAL_TIM_PWM_Init(&htim2)`. Is it correct to call `HAL_TIM_PWM_Init` before `HAL_TIM_Base_Start_IT`? In the real STM32 HAL, yes — `HAL_TIM_PWM_Init` configures the timer, `HAL_TIM_Base_Start_IT` enables interrupts. The order is correct.

But `HAL_TIM_Base_Start_IT` is ONLY for the base timer (update interrupt). PWM start is separate (`motor_start()` calls `HAL_TIM_PWM_Start`). So the timer update interrupt fires every 10ms, calling `HAL_TIM_PeriodElapsedCallback`. PWM is started separately. This is correct.

**Assessment:** PASS.

### 2.4 SPI

**Finding H4 — `sensor_read_temperature` performs SPI transaction then discards result (Major)**

`sensor.c:25` calls `HAL_SPI_TransmitReceive`. Lines 28-34 parse the raw value, then explicitly discard it with `(void)raw` and return the hardcoded `SPI_TEMP_FIXED` (25.0°C).

**Evidence:** `sensor.c:32`: `(void)raw;` / `sensor.c:33`: `temp = SPI_TEMP_FIXED;`

**Assessment:** For a validation fixture, returning a fixed value is the CORRECT design choice — it ensures deterministic trace output. The SPI transaction IS exercised (trace event generated). However, the code constructs and then discards `raw`, which means the SPI response parsing code path is dead. If the mock returned actual data, this code would work identically. This is an **acceptable simulation tradeoff** but must be documented as such.

### 2.5 RCC

**Finding H5 — `SystemClock_Config` is a complete no-op (Minor)**

`main.c:33-48`: All `RCC_OscInitTypeDef` and `RCC_ClkInitTypeDef` fields are set to 0. `HAL_RCC_OscConfig` and `HAL_RCC_ClockConfig` are called but the configuration is empty. On real hardware, this would leave the clock at the reset-default HSI frequency (16 MHz), which is actually a valid state — the MCU would run, just at a default speed.

**Assessment:** Acceptable simplification. EmberSim doesn't model clock trees. The calls exist to exercise the HAL functions and produce trace events, not to configure real clocks. The function name and structure are retained for architectural realism.

---

## 3. STM32 Realism

### Classification: MINOR SIMPLIFICATION

| Aspect | Assessment |
|--------|-----------|
| Startup flow | Correct: `HAL_Init → SystemClock_Config → MX_*_Init` |
| Peripheral handle structure | Correct: `huart2`, `htim2`, `hspi1` with standard CubeMX naming |
| GPIO config | Correct: pin, mode, pull, speed fields used |
| UART config | Correct: baud rate, word length, stop bits, parity, mode, flow control |
| TIM config | Correct: prescaler calculation (84MHz / 8400 = 10kHz), period for 100Hz |
| SPI config | Correct: master mode, 8-bit, CPOL/CPHA, NSS soft, baud rate prescaler |
| Callback pattern | Correct: `HAL_TIM_PeriodElapsedCallback` with instance check |
| `__HAL_TIM_SET_COMPARE` | **Removed** — this macro is standard in STM32 firmware but was deliberately excluded because it writes TIM registers directly. The resulting gap (PWM duty cycle is tracked in firmware state only) is architecturally honest but creates a feature that doesn't fully work (see Finding R1). |

**Finding R1 — `motor_set_duty()` is a no-op at runtime (Major)**

`motor_control.c:69-81`: `motor_set_duty()` clamps the value and stores it in `g_motor_duty`. If the motor is running, it does `(void)duty;` — a deliberate no-op. The comment says "a full HAL_TIM_PWM_ConfigChannel is needed for runtime changes." This means the function signature promises duty cycle control but delivers none. The trace will show no HAL call when `motor_set_duty` is invoked.

**Impact on regression testing:** A firmware change that calls `motor_set_duty(750)` instead of `motor_set_duty(250)` would produce IDENTICAL traces. The regression is invisible. This is a blind spot.

**Recommendation:** Either (a) implement the function with `HAL_TIM_PWM_ConfigChannel` + `HAL_TIM_PWM_Start` (stop/reconfigure/restart pattern), or (b) document that runtime duty changes are not validated by this fixture and add a test that explicitly exercises `HAL_TIM_PWM_ConfigChannel`.

**Finding R2 — `motor_control.h` uses `TIM_CHANNEL_1` without including the HAL header (Minor)**

`motor_control.h:13`: `#define MOTOR_PWM_CHANNEL TIM_CHANNEL_1`. The header includes `<stdint.h>` but not `stm32f4xx_hal.h` where `TIM_CHANNEL_1` is defined. If a compilation unit includes `motor_control.h` before the HAL header AND uses `MOTOR_PWM_CHANNEL`, the preprocessor fails.

**Evidence:** In `app.c`, the include order is `motor_control.h` (line 13) before `stm32f4xx_hal.h` (line 15). `app.c` doesn't use `MOTOR_PWM_CHANNEL` directly, so it compiles. But this is fragile.

**Recommendation:** Add `#include "stm32f4xx_hal.h"` to `motor_control.h` (standard STM32 pattern) or move the `#define` to `motor_control.c`.

---

## 4. Simulation Friendliness

### Include Strategy — CRITICAL FINDING

**Finding S1 — Firmware include strategy is unverified against EmberSim mock type generation (CRITICAL)**

The Pilot 0 firmware source files include `"stm32f4xx_hal.h"` from `firmware/Drivers/STM32F4xx_HAL_Driver/Inc/`. However, evidence from the g5e-fresh fixture shows that EmberSim's `embersim init` generates `mock_hal.h` with **different struct layouts** than the original HAL header:

| Type | Pilot 0 HAL header | g5e-fresh generated mock_hal.h |
|------|-------------------|-------------------------------|
| `UART_HandleTypeDef` | Fields: `Instance, Init, pTxBuffPtr, TxXferSize, TxXferCount, pRxBuffPtr, RxXferSize, RxXferCount, Lock, ErrorCode` | Fields: `Instance, Init, pTxBuffPtr, TxXferSize, TxXferCount, pRxBuffPtr, RxXferSize, RxXferCount, gState, RxState, ErrorCode` |
| `GPIO_TypeDef` | Full register struct (10 fields, 40 bytes) | `struct { volatile uint32_t dummy; }` (4 bytes) |
| `TIM_HandleTypeDef` | Fields: `Instance, Init (struct), Channel, Lock, State` | **Not defined in mock_hal.h** — must come from elsewhere |

**Evidence:**
- Pilot 0 `stm32f4xx_hal.h:225-231`: 5-field `TIM_HandleTypeDef` with nested `TIM_Base_InitTypeDef`
- g5e-fresh `mock_hal.h:25`: 11-field `UART_HandleTypeDef` with `gState`, `RxState` added by stubgen
- g5e-fresh `mock_tim.h`: Does NOT define `TIM_HandleTypeDef`

**What this means:** If Pilot 0 firmware compilation units see `stm32f4xx_hal.h` types while mock `.c` compilation units see `mock_hal.h` types, the struct layouts differ. Passing a `UART_HandleTypeDef*` from firmware code to mock code crosses an ABI boundary with mismatched struct sizes. This is **undefined behavior** — the mock code accesses fields (`gState`, `RxState`) that don't exist in the firmware's version of the struct.

**The correct pattern** (demonstrated by the ci-test fixture's `app.c`):

```c
#include "mock_hal.h"    // ← mock types, NOT stm32f4xx_hal.h
#include "mock_tim.h"    // ← mock peripheral model API
```

The Pilot 0 firmware does NOT follow this pattern. It includes the hardware HAL header directly. Whether this works depends on whether `embersim init` generates mock types that are ABI-compatible with the HAL header — something that cannot be verified without actually running the toolchain.

**Recommendation:** Run `embersim init` against the Pilot 0 HAL header, inspect the generated `mock_hal.h`, and verify type compatibility. If types differ, the firmware must include `mock_hal.h` instead of (or in addition to, with careful include guard management) `stm32f4xx_hal.h`. The HAL header should be used ONLY as input to `embersim init`, not as a compilation include under EmberSim.

**Severity:** CRITICAL — this determines whether the fixture actually works with EmberSim.

**Finding S2 — `uart_service_inject_rx()` bypasses EmberSim's UART RX interrupt path (Major)**

`uart_service.c:86-95`: `uart_service_inject_rx()` directly writes to `rx_buf` and sets `cmd_pending = 1`, bypassing `HAL_UART_Receive_IT` and the `HAL_UART_RxCpltCallback`. This function exists ONLY for simulation — production firmware would receive data through the UART ISR.

**Assessment:** The function is correctly documented as simulation-only ("In simulation, RX is injected via uart_service_inject_rx()"). However, it creates a second RX path that cannot be exercised in the same test run as the interrupt path. The trace will show either:
- `HAL_UART_Receive_IT` → callback chain (interrupt path), OR
- Direct buffer write (injection path)

But not both. For regression testing, the interrupt path (callback chain) is what validates EmberSim's NVIC dispatch. The injection path skips this entirely.

**Recommendation:** The fixture README should document that Pilot 0 validation exercises the UART TX path (via `uart_service_respond` → `HAL_UART_Transmit`) for regression detection, and the RX path is exercised through EmberSim's interrupt model (not injection) during actual testing. The `inject_rx` function should be used only for manual interactive testing.

---

## 5. Determinism

**Finding D1 — `HAL_GPIO_ReadPin` return value depends on mock state (Low risk)**

`app.c:117`: `HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)` reads PA0 (fault input). In EmberSim's `mock_state.c`, this returns the last written value. Since PA0 is configured as input with pull-up, and nothing ever writes to it in the normal execution path, it always returns `GPIO_PIN_SET` (logic high — not asserted). The fault path at lines 118-122 is NEVER taken in normal execution.

**Assessment:** This is correct for deterministic execution — the fault path exists as architecture but is not triggered in the golden trace. For regression testing, this means the `HAL_GPIO_ReadPin` call IS traced but the fault response code is dead. To test fault handling, you'd need to inject a low value on PA0 via the mock API.

**Recommendation:** Document that the fault path is architectural (demonstrates input GPIO + conditional motor stop) but not exercised in the default trace. This is acceptable — the code path exists and would be exercised if a test script wrote to PA0 via mock_state.

**Finding D2 — `sensor_read_temperature` always returns 25.0°C regardless of SPI data (Nit)**

As discussed in H4. The deterministic override is correct for validation but means the SPI data path is never tested with variable input. If the mock SPI implementation had a bug that returned wrong data, this code would never detect it.

**Assessment:** Acceptable tradeoff for a deterministic reference fixture. The `HAL_SPI_TransmitReceive` call IS exercised and traced.

### Determinism verdict: PASS

Two identical executions will produce identical traces. No random number generation, no time-dependent behavior, no uninitialized state. The only variable input (`HAL_GPIO_ReadPin`) returns a fixed value in the default path.

---

## 6. Regression Quality

**Finding Q1 — Trace covers HAL function calls but not application logic changes (Major)**

The EmberSim trace captures HAL function calls (function name + parameters). It does NOT capture:
- Changes to `g_state.tick_count`
- Changes to `g_state.led_state`
- Which command was dispatched in `app_run()`
- The value returned by `sensor_read_temperature()`

This means: a firmware change that alters application behavior WITHOUT changing which HAL functions are called produces an identical trace. Example: changing the heartbeat rate from 10 ticks to 20 ticks changes `app.c:139` from `% 10` to `% 20` — the trace shows the same `HAL_GPIO_TogglePin` calls (just at a different frequency, but the trace only counts events, not timing).

**Assessment:** This is an architectural limitation of EmberSim's trace format (HAL-level), not a bug in the fixture. The trace proves the HAL surface is stable. Application-level changes that don't alter the HAL call pattern are invisible. This is a known scope limitation — EmberSim validates firmware BEHAVIOR at the HAL boundary, not internal application state.

**Recommendation:** Document this scope limitation in the fixture README and in STAGE7_PILOT0_REFERENCE_SPEC.md. The fixture validates HAL behavioral stability, not full application correctness.

**Finding Q2 — Regression test via removing `HAL_UART_Transmit` is valid but minimal (Minor)**

The acceptance criteria §A7 specifies commenting out `HAL_UART_Transmit` in `uart_service.c`. This produces a clear trace divergence (UART TX event disappears). However, this tests only removal of a HAL call. Other regression types are not tested:
- Changing HAL call parameters (different baud rate, different timeout)
- Adding a new HAL call
- Changing HAL call ordering
- Changing which peripheral is used

**Assessment:** The single regression test is sufficient for Pilot 0 validation. Additional regression scenarios can be added in future iterations.

---

## 7. Test Coverage Value

### What this fixture CAN detect as regressions:

| Regression type | Detectable? | How |
|----------------|------------|-----|
| HAL function removed | ✅ | Trace event count changes, function missing |
| HAL function added | ✅ | New function appears in trace |
| HAL function parameter changed | ⚠️ Partial | If trace captures parameters (depends on mock implementation) |
| GPIO write removed | ✅ | `HAL_GPIO_WritePin`/`TogglePin` events disappear |
| TIM callback removed | ✅ | `HAL_TIM_PeriodElapsedCallback` events disappear |
| UART TX removed | ✅ | `HAL_UART_Transmit` events disappear |
| SPI transfer removed | ✅ | `HAL_SPI_TransmitReceive` events disappear |
| PWM start/stop removed | ✅ | `HAL_TIM_PWM_Start`/`Stop` events disappear |
| Application logic changed (same HAL calls) | ❌ | Trace is identical |
| Timing behavior changed | ❌ | Trace only counts events, not intervals |
| Fault handling logic changed | ❌ | Fault path is never triggered in default trace |

### Blind spots:

| Blind spot | Why | Risk |
|-----------|-----|------|
| `motor_set_duty()` | No-op when running — no HAL call generated | Low — documented limitation |
| Fault input path (`app.c:117-122`) | Never triggered in default trace | Medium — architectural, not exercised |
| `uart_service_inject_rx()` path | Simulation-only, bypasses NVIC | Medium — doesn't test interrupt dispatch for RX |
| SPI response parsing (`sensor.c:28`) | Result discarded, fixed value returned | Low — `HAL_SPI_TransmitReceive` is exercised |
| `uart_service_poll()` parsing of specific commands | Only tested if commands are injected | Low — all 5 commands are parseable by construction |

---

## 8. Maintainability

| Aspect | Assessment |
|--------|-----------|
| Module size | Good — largest file is `app.c` at 145 lines, well under 200-line threshold |
| Naming | Good — CubeMX-style `MX_*_Init`, clear function names |
| Comments | Good — Doxygen-style headers, inline explanations for simulation tradeoffs |
| API design | Adequate — each module exposes 4-6 functions, clean boundaries |
| Duplication | Low — no copy-pasted code |
| Extensibility | Adequate — adding a new command requires changes to `uart_service_poll()` and `app_run()` only |

**Finding M1 — `app_get_state()` returns a mutable pointer to private state (Minor)**

`app.h:26`: `AppState *app_get_state(void);` returns a pointer to `g_state`. Any module that calls this can mutate application state directly, bypassing `app_set_motor_running()` and `app_set_temperature()`. This breaks encapsulation.

**Evidence:** `app.c:28-31`: Returns `&g_state` — caller gets unrestricted access.

**Recommendation:** Either (a) remove `app_get_state()` and add specific getter functions, or (b) return `const AppState *` to prevent mutation. The function is not currently called by any other module, so it's also dead code.

---

## 9. Native Build Quality

**Finding N1 — HAL stubs are exhaustive but hide ABI issues (Major)**

`stm32f4xx_hal.c` provides no-op implementations for all HAL functions. The native build links successfully against these stubs. However, the stubs use the SAME types as the HAL header (`stm32f4xx_hal.h`), while EmberSim's mocks use DIFFERENT types (`mock_hal.h`). This means the native build succeeds with one set of types while the EmberSim build uses another. The native build does NOT validate EmberSim compatibility.

**Evidence:** Native `make` links firmware against `stm32f4xx_hal.c` stubs using `stm32f4xx_hal.h` types. `embersim run` links firmware against `mock_hal.c`/`mock_uart.c`/etc. using `mock_hal.h` types. These are DIFFERENT type systems.

**Recommendation:** This is a fundamental limitation of the dual-build approach (native for sanity, EmberSim for simulation). The native build proves the firmware is syntactically correct. It does NOT prove EmberSim compatibility. This should be explicitly documented.

**Finding N2 — Makefile uses VPATH-relative object paths that may collide (Nit)**

`Makefile:24`: Object paths mirror source paths under `build/`. With 6 `.c` files, this is fine. At scale, flat source trees or filename collisions could be an issue.

**Assessment:** Not a problem for 6 files.

---

## 10. Customer Credibility

**Verdict:** This firmware would be perceived as a **well-structured demo/reference project**, but not as production firmware.

**Evidence supporting credibility:**
- CubeMX-style initialization pattern (`MX_*_Init`)
- Realistic TIM prescaler/period calculation with documented math
- Standard STM32 HAL callback pattern with instance checking
- Proper include guards, Doxygen headers
- 4-peripheral design that mirrors real industrial products

**Evidence undermining credibility:**
- `SystemClock_Config` is a no-op (would be caught in any real HW test)
- `motor_set_duty` is a documented no-op (honest but looks broken)
- Sensor always returns 25.0°C regardless of SPI data (documented but looks like a stub)
- No error handling beyond the fault input (no timeout handling, no HAL return value checking)
- `extern` declarations scattered across `.c` files (sloppy in a reference project)
- Include strategy for EmberSim types is unverified (a reviewer would catch this)

**Rating:** 6/10 — competent for a demo, not credible as production firmware.

---

## 11. Acceptance Against Stage 7 Goals

| Goal | Status | Evidence |
|------|--------|----------|
| Onboarding steps work | ⚠️ Unverified | `embersim init` not yet run against this fixture |
| HAL generation | ⚠️ Unverified | Mock types must be verified compatible |
| Simulation executes | ⚠️ Unverified | Cannot confirm without running `embersim run` |
| Deterministic trace | ✅ By design | No sources of nondeterminism |
| Regression detection | ✅ By design | Removing `HAL_UART_Transmit` will produce divergence |
| Baseline creation | ⚠️ Unverified | Cannot confirm without first trace |

**Key gap:** The fixture has never been run through `embersim init && embersim run`. Every claim about Stage 7 goal satisfaction is based on design analysis, not empirical evidence.

---

## 12. Missing Industrial Features

The following are NOT recommended additions — they are noted as architectural omissions for completeness:

| Feature | Status | Why omitted |
|---------|--------|------------|
| Watchdog | Absent | Not in EmberSim scope |
| Error handling (HAL return value checks) | Absent | Simplifies code; HAL always returns OK in simulation |
| Assertions | Absent | Would add runtime checks not relevant to trace validation |
| Timeout handling | Absent | Simulation is synchronous; timeouts don't occur |
| Fault recovery | Present (skeletal) | Fault input stops motor but doesn't handle recovery |
| Logging/tracing beyond HAL | Absent | EmberSim trace IS the logging layer |
| State validation | Minimal | `motor_start/stop` check current state; no other guards |
| Configuration isolation | Partial | `embersim.toml` separates config; peripheral handles are global |
| Dependency injection | Absent | `extern` globals are hard-linked |

**Recommendation:** Add HAL return value checking in at least one place (e.g., `uart_service_respond` checks if `HAL_UART_Transmit` returns `HAL_OK`). This would exercise the HAL return value path in the trace and demonstrate that the firmware handles error returns — even if they never occur in simulation.

---

## 13. Hidden Risks

### Risk 1: Type incompatibility crash (Critical)

If `embersim init` generates mock types that are ABI-incompatible with the Pilot 0 HAL header types, the linked binary will have undefined behavior. Symptoms range from silent data corruption to segfault.

### Risk 2: Include path resolution order (Major)

`embersim.toml` lists `firmware/Drivers/STM32F4xx_HAL_Driver/Inc` BEFORE `ember_sim/mocks`. Since firmware sources include `"stm32f4xx_hal.h"` (found in the Drivers path), the mock header is never consulted. This is the intended behavior for the HAL header but may cause issues if the firmware needs mock-specific types.

### Risk 3: `host_main.c` entry point mismatch (Major)

`embersim.toml:37`: `entry = "app_init"`. The generated `ember_sim/host_main.c` calls `app_init()` once then `app_run()` in a loop. But `main.c` ALSO defines `main()` (line 125). When compiled under EmberSim, both `host_main.c` (which provides `main()`) and `main.c` (which also provides `main()`) are compiled. The linker will see TWO `main()` functions → duplicate symbol error.

**Evidence:** `embersim.toml:17-24` includes `main.c` in `[build.sources]`. `main.c:125` defines `int main(void)`. `host_main.c` also defines `int main()`. These conflict.

**This is a build failure waiting to happen.** The ci-test fixture avoids this by NOT including a `main()` in its firmware — it has `app_init()`/`app_run()` in `app.c` and relies on `host_main.c` for the entry point.

**Recommendation:** Either (a) remove `main.c` from `embersim.toml [build.sources]` (the `main()` function is only for native build), or (b) wrap `main.c`'s `main()` in `#ifndef EMBERSIM` / `#endif`, or (c) split the entry point so `main.c` only has `MX_*_Init` functions and the actual `main()` is in a separate file excluded from EmberSim compilation.

This is a BLOCKING issue — the fixture cannot produce a trace until this is resolved.

### Risk 4: `main.c` includes `stm32f4xx_hal.h` but EmberSim expects `mock_hal.h` (Critical)

Related to S1. The firmware's include strategy may not work with the EmberSim build.

### Risk 5: `app_init` / `app_run` defined in `app.c`, but `main.c` also calls them from `main()` (Minor)

If `main.c` is excluded from EmberSim build (to avoid duplicate `main()`), the `MX_*_Init` functions must be called from somewhere. Currently they're only called from `main()` in `main.c`.

---

## 14. Evidence Table

| # | Finding | Severity | Evidence | Recommendation |
|---|---------|----------|----------|---------------|
| S1 | Include strategy unverified — firmware uses HAL header types, mocks use generated types | **Critical** | Pilot 0 `UART_HandleTypeDef` has 10 fields; g5e-fresh `mock_hal.h` `UART_HandleTypeDef` has 11 fields with different names | Verify type compatibility by running `embersim init` against Pilot 0 HAL header |
| R3 | Duplicate `main()` — `main.c` and `host_main.c` both define entry point | **Critical** | `embersim.toml` includes `main.c` in sources; `main.c:125` defines `int main(void)` | Remove `main.c` from EmberSim sources or guard `main()` with `#ifndef EMBERSIM` |
| A1 | Fragile `extern` coupling across modules | Major | 3 modules independently declare `extern` handles | Move to shared header |
| H4 | SPI result discarded — sensor parsing code path is dead | Major | `sensor.c:32`: `(void)raw;` | Document as acceptable simulation tradeoff |
| Q1 | Application logic changes invisible to trace | Major | Trace captures HAL calls only, not internal state | Document scope limitation |
| R1 | `motor_set_duty()` is a no-op at runtime | Major | `motor_control.c:80`: `(void)duty;` | Implement or document exclusion |
| S2 | `uart_service_inject_rx()` bypasses NVIC dispatch | Major | Direct buffer write, no interrupt path | Document: used for manual testing only |
| N1 | Native build uses different types than EmberSim build | Major | `stm32f4xx_hal.c` stubs vs `mock_hal.c` mocks | Document dual-build limitation |
| A2 | Double initialization of TIM | Minor | `HAL_TIM_Base_Init` + `HAL_TIM_PWM_Init` on same handle | Consolidate |
| H1 | `MX_GPIO_Init` doesn't re-zero struct between pins | Minor | `main.c:57-70` | Add struct re-init |
| H5 | `SystemClock_Config` is a no-op | Minor | All fields set to 0 | Document as simulation simplification |
| R2 | `motor_control.h` uses `TIM_CHANNEL_1` without including HAL header | Minor | Include-order dependency | Add `#include "stm32f4xx_hal.h"` to header |
| M1 | `app_get_state()` returns mutable pointer to private state | Minor | `app.c:28-31` | Return `const` or remove (currently dead code) |
| D1 | Fault input path never exercised in default trace | Minor | `app.c:117` always reads HIGH | Document architectural code path |
| H2 | UART IT reception pattern is correct | — | Verified against STM32 standard pattern | No action |
| D2 | Sensor returns fixed value | Nit | `sensor.c:33` | Already documented |

---

## 15. Final Scores

| Category | Score | Rationale |
|----------|-------|-----------|
| Architecture | 7/10 | Clean module boundaries, good init order. Minus 3 for extern coupling and double-init |
| STM32 realism | 7/10 | Convincing peripheral config, standard patterns. Minus 3 for no-op clock config and missing `__HAL_TIM_SET_COMPARE` |
| Simulation quality | 4/10 | **Unverified.** Include strategy, type compatibility, and duplicate `main()` not resolved |
| Maintainability | 8/10 | Well-structured, well-commented, small modules. One encapsulation issue |
| Industrial credibility | 6/10 | Looks like a competent demo, not production firmware. Honest about limitations |
| Regression value | 5/10 | HAL removal detection works. Blind spots: duty cycle changes, fault path, app logic |
| Documentation alignment | 8/10 | README and spec match code. Comments explain tradeoffs. Spec-to-code traceability is good |
| **Overall readiness** | **4/10** | **Cannot be validated without running `embersim init && embersim run`. Two Critical blockers.** |

---

## 16. Final Recommendation

**APPROVE WITH MAJOR CHANGES**

The architectural design is sound. The module structure, HAL usage patterns, and regression detection strategy are appropriate for a reference validation fixture. However, the fixture has never been executed through the EmberSim toolchain. Two Critical findings (type compatibility and duplicate `main()`) must be resolved before this fixture can serve as EmberSim's permanent internal validation reference.

---

## 17. Blocking Issues (Must Resolve Before Pilot 0 Acceptance)

| # | Blocker | Resolution |
|---|---------|-----------|
| B1 | **Type compatibility unverified.** Does `embersim init` generate `mock_hal.h` types that are ABI-compatible with `stm32f4xx_hal.h`? | Run `embersim init` against Pilot 0 HAL header. Compare generated `mock_hal.h` type layouts. If incompatible, switch firmware includes to `mock_hal.h`. |
| B2 | **Duplicate `main()` symbol.** `main.c` and `host_main.c` both define `int main(void)`. Build will fail at link time. | Remove `firmware/Core/Src/main.c` from `embersim.toml [build.sources]`, or wrap `main()` in `#ifndef EMBERSIM`, or split `MX_*_Init` functions into a separate file. |
| B3 | **Empirical validation.** The fixture has never produced a trace. | Run the full 9-step onboarding path from GETTING_STARTED.md against this fixture. Fill out the B4 dry-run report template with actual measurements. |

---

## Appendix A: Items That Are Correct By Design

These were initially flagged as concerns but determined to be correct after analysis:

- **UART IT reception pattern:** Standard STM32 single-byte interrupt receive. Matches CubeMX output.
- **TIM initialization order:** `HAL_TIM_PWM_Init` before `HAL_TIM_Base_Start_IT` is correct.
- **Deterministic execution:** No sources of nondeterminism found.
- **HAL function coverage:** 15 functions across 4 peripherals is appropriate for Pilot 0.
- **Callback instance checking:** `htim->Instance == TIM2` guard in `HAL_TIM_PeriodElapsedCallback` is correct.
- **SPI read-then-discard pattern:** Correct for deterministic validation; `HAL_SPI_TransmitReceive` is exercised.

## Appendix B: Items That Are Acceptable Simplifications

These are deliberate design choices for a simulation reference fixture:

- **Fixed sensor temperature (25.0°C):** Ensures deterministic trace. SPI transfer is still exercised.
- **No-op `SystemClock_Config`:** Clocks don't matter in simulation. Function structure preserved for realism.
- **No HAL return value checking:** All HAL functions return `HAL_OK` in mock implementations. Checking is redundant.
- **No debounce on fault input:** Simulation has no electrical noise. Immediate response is correct.
- **`extern` peripheral handles:** Standard embedded C pattern (though a shared header would be better).
