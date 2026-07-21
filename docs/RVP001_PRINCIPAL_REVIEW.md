# RVP-001 — Independent Principal Engineer Design & Release-Readiness Review

**Review type:** Adversarial pre-certification audit
**Reviewer role:** Independent Principal Embedded Systems Engineer
**Date:** 2026-07-21
**Standard:** Equivalent to release-gating review at ST / ARM / Google / Microsoft
**Mandate:** Attempt to prove RVP-001 is NOT ready for certification

---

## Executive Summary

RVP-001 is a **well-designed but entirely unproven** conformance suite. The architecture, governance, and acceptance framework are professionally structured. However, the project has **zero empirical evidence** supporting any of its core claims. More concerningly, source-code inspection reveals four findings that undermine specific claims made in the documentation: a fabricated error path that never calls the HAL, a state machine whose transitions are invisible to the trace, a dead EXTI callback, and a strict-warning Makefile that has never been compiled.

**Certification decision: REJECT** (not "ready for certification execution")

The rejection is not because RVP-001 is bad. It is because RVP-001 claims capabilities it has not demonstrated, and four specific code-level issues must be resolved before any empirical gates can produce meaningful evidence.

---

## 1. Evidence Audit

Every claim in the RVP-001 documentation is classified below.

| # | Claim | Source | Classification | Basis |
|---|-------|--------|---------------|-------|
| C1 | "Native build succeeds with zero warnings" | P0-01 | **Unsupported** | Makefile never executed |
| C2 | "Firmware exercises 21 HAL functions across 5 peripherals" | RVP001_ACCEPTANCE.md | **Assumed** | Counted from source, not verified via trace |
| C3 | "100 consecutive runs produce identical traces" | P0-07 | **Unsupported** | Never executed |
| C4 | "Cross-platform determinism confirmed" | P0-07b | **Unsupported** | Never executed |
| C5 | "Generated mock ABI is compatible with firmware types" | P0-02b | **Unsupported** | sizeof/offsetof never measured |
| C6 | "Intentional regression detected" | P0-09 | **Unsupported** | Never executed |
| C7 | "Onboarding completes in < 15 minutes" | P0-12 | **Unsupported** | Never timed |
| C8 | "Clean repository validation passes" | P0-13 | **Unsupported** | Never executed |
| C9 | "SPI error recovery path exercises HAL_BUSY retry" | sensor.c, RVP001_CERTIFICATION.md P0-06 | **Speculative** | See Finding F1 below |
| C10 | "UART state machine produces richer trace output" | uart_service.c header comment | **Speculative** | See Finding F2 below |
| C11 | "EXTI interrupt handler validates NVIC dispatch" | board.c:155-159 | **Speculative** | See Finding F3 below |
| C12 | "RVP-001 is frozen" | RVP_POLICY.md §4 | **Factually incorrect** | Files are uncommitted, modified multiple times this session |
| C13 | "BSP layer mirrors production STM32 project structure" | board.h header comment | **Supported** | Structure matches CubeMX output pattern |
| C14 | "All peripheral access goes through HAL APIs" | RVP_POLICY.md §3.1 | **Supported** | Verified in all 5 source modules |
| C15 | "No RTOS, DMA, USB, Ethernet, CAN dependencies" | RVP_POLICY.md §3.1 | **Supported** | Verified — none present |

**Summary: 3 claims Supported by code inspection, 10 claims Unsupported/Speculative, 1 claim Factually incorrect, 0 claims Verified by execution.**

---

## 2. Critical Findings

### Finding F1 — SPI error path fabricates HAL_BUSY in application code (CRITICAL)

**File:** `sensor.c:38-48`
**Claim:** "Exercises the HAL error-return path and retry logic"
**Reality:** The HAL is never asked to return BUSY.

```c
g_spi_read_count++;

if (g_spi_read_count == 1) {
    status = HAL_BUSY;   // ← assigned directly, NOT returned by HAL_SPI_TransmitReceive
} else {
    status = HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, 2, 100);
}

if (status == HAL_BUSY) {
    status = HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, 2, 100);  // retry
}
```

On the first call, `HAL_SPI_TransmitReceive` is **never invoked**. The BUSY value is a C assignment, not a HAL return. The mock SPI implementation's BUSY path is never exercised. The retry logic at line 56 calls `HAL_SPI_TransmitReceive` for the first time on the second attempt — but the function has already fabricated its own error and decided to retry, without the HAL ever being involved in the error decision.

**Impact:** P0-06 claims "Error-recovery events: HAL_BUSY → retry → HAL_OK" will appear in the trace. They will not. The trace will show one `HAL_SPI_TransmitReceive` call (the retry), not a BUSY return followed by a retry. The certification report's trace invariants table expects "HAL_BUSY recovery path: Present" — this is unachievable with the current code.

**Severity:** CRITICAL. This is not a simulation simplification; it is a fabrication that produces a different trace than the documentation claims. The certification report would record false evidence.

**Fix:** Remove the `if (g_spi_read_count == 1)` fabrication. Instead, configure the SPI mock (via its test API) to return `HAL_BUSY` on the first call. If the mock does not support configurable return values, document that HAL error-return path testing is deferred to a future RVP and remove the BUSY-retry logic from sensor.c. A conformance suite must not fabricate the conditions it claims to test.

### Finding F2 — UART state machine transitions are invisible to the trace (MAJOR)

**File:** `uart_service.c:111-152`
**Claim:** "This produces richer trace output than a simple strncmp() approach because each state transition exercises different HAL call patterns."
**Reality:** The state transitions occur entirely inside `HAL_UART_RxCpltCallback`. The trace captures HAL function calls. The callback calls `HAL_UART_Receive_IT` in every state — IDLE, RECEIVING, and FRAME_READY all produce the same `HAL_UART_Receive_IT` trace event.

```c
case UART_PARSE_IDLE:       HAL_UART_Receive_IT(...); break;
case UART_PARSE_RECEIVING:  HAL_UART_Receive_IT(...); break;
case UART_PARSE_FRAME_READY: HAL_UART_Receive_IT(...); break;
```

The trace for all three states is identical: `HAL_UART_Receive_IT(huart2, addr, 1)`. The parse state is not a HAL call parameter and is not captured in the trace. The state machine is architecturally better code, but the claim that it produces "richer trace output" is incorrect. The trace is indistinguishable from the simpler strncmp approach.

**Impact:** Documentation overstates the trace value. The acceptance criteria do not depend on this claim, so it does not block certification — but it erodes credibility.

**Severity:** MAJOR. Documentation claim does not match observable behavior.

### Finding F3 — EXTI callback is dead code (MAJOR)

**File:** `board.c:155-159`
**Claim (from `board_gpio_init`):** "The EXTI interrupt fires deterministically in simulation when the fault pin is asserted low."
**Reality:** The callback body is empty:

```c
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BOARD_FAULT_PIN) {
        /* Fault detected — application layer responds in app_run() */
    }
}
```

The comment says "application layer responds in app_run()." The application layer (`app.c:126`) reads the GPIO pin directly via `HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0)`. It does NOT check any flag set by the callback. The EXTI callback has zero observable effect on program behavior.

**Impact:**
- The NVIC EXTI dispatch path (SetPriority → EnableIRQ → EXTI trigger → callback invocation) is never validated through observable behavior.
- If the NVIC model had a bug where EXTI callbacks were silently not called, this code would never detect it — the fault is detected by polling, not by interrupt.
- The trace would show `HAL_GPIO_EXTI_Callback` being called, but since it does nothing, the trace event has no semantic value.

**Severity:** MAJOR. A documented validation target (NVIC EXTI dispatch) is not actually validated.

**Fix:** The callback should set a volatile flag that `app_run()` checks, OR the fault detection should be moved entirely into the callback (with `app_run()` checking only the flag). This makes the interrupt path the primary detection mechanism and validates that NVIC dispatch works.

### Finding F4 — Strict-warning Makefile has never been compiled (MAJOR)

**File:** `Makefile:17-22`
**Claim:** P0-01 requires "zero warnings under -Wall -Wextra -Wpedantic -Werror -Wshadow -Wconversion -Wformat=2 -Wundef"
**Reality:** The Makefile was written with these flags but never executed. Several likely warnings:

1. `-Wconversion`: Timeout literal `100` in `uart_service.c:88` is `int` passed to `uint32_t` parameter. Requires `100U`.
2. `-Wconversion`: `8399` (int) assigned to `uint32_t Prescaler` in `board.c:113`.
3. `-Wconversion`: `strlen()` returns `size_t`; cast to `uint16_t` at `uart_service.c:87` may warn.
4. `-Wmissing-prototypes`: Static functions in `main.c` (none currently, but any added later).
5. `-Wundef`: No obvious violations, but unverified.

**Impact:** P0-01 is the FIRST gate. If it fails, every subsequent gate is blocked. The strict flags were added on the final editing pass without compilation testing. The risk that P0-01 fails on first execution is high.

**Severity:** MAJOR. The first certification gate is unverified against the current code.

---

## 3. Hidden Risks

### Risk R1 — Trace format dependency

RVP-001's golden baseline is tied to the current trace format (`trace.jsonl`). If EmberSim's trace format evolves (adds fields, changes event naming, alters JSON structure), the baseline comparison will break. The RVP_POLICY.md does not address trace schema versioning or migration. If the trace format changes in EmberSim v0.2.0, RVP-001 may need a new baseline — but the freeze policy forbids baseline changes except for "toolchain-compatibility re-baselines."

**Recommendation:** Add a trace schema version to the baseline metadata and define a policy for when re-baselining is permitted due to format changes.

### Risk R2 — Overfitting to current mock behavior

`sensor.c` returns `SPI_TEMP_FIXED` regardless of what the SPI mock returns. `motor_control.c` uses `HAL_TIM_PWM_ConfigChannel` instead of `__HAL_TIM_SET_COMPARE` because the latter is unsupported. If EmberSim's mock implementations change behavior (e.g., `mock_tim` gains CCR register write support), the firmware's workarounds become unnecessary but remain in the code. The fixture would continue passing while masking mock improvements.

**Recommendation:** Document each workaround and its expected deprecation condition (e.g., "when mock_tim supports CCR writes, switch motor_set_duty to use __HAL_TIM_SET_COMPARE").

### Risk R3 — 100 ms simulation duration may be insufficient

The simulation runs for 100 ms (`embersim.toml:39`). At a 10 ms timer tick, this produces ~10 timer callbacks and ~1 LED toggle. This is enough to prove the timer works but too short to detect timer wrap bugs, long-duration drift, or PWM accumulation errors. If a future EmberSim change introduces a 1% timing error that accumulates over 1000 cycles, the 100 ms run will not detect it.

**Recommendation:** Add a P0-06b gate for a longer-duration run (e.g., 5000 ms) and verify that the LED toggle count matches expectations (50 toggles for 100 ms period).

### Risk R4 — No uninitialized memory detection

The HAL stubs in `stm32f4xx_hal.c` zero-initialize nothing. If firmware reads from a peripheral handle field that was never written, the value is whatever was on the stack. The native build runs under ASan which detects some of these, but ASan only catches heap/user-after-free, not uninitialized stack reads in all cases. MemorySanitizer (MSan) would catch these but is not in the acceptance gates.

**Recommendation:** Add `-Wuninitialized` to CFLAGS (already covered by `-Wall`) and consider adding an MSan build target.

---

## 4. Missing Certification Gates

The 14-gate framework is strong for correctness. The following gates are standard in mature conformance suites and are absent:

| # | Missing gate | Why it matters | Priority |
|---|-------------|---------------|----------|
| G1 | **Binary reproducibility** | Same source + same compiler = bit-identical binary. Proves build determinism. | High |
| G2 | **Long-duration stability** | 5000+ ms run. Detects timer wrap, memory leaks, accumulation errors. | High |
| G3 | **Trace schema compatibility** | Baseline from v1.0 must be comparable against v1.1 trace output, OR a documented migration path must exist. | High |
| G4 | **Mutation testing** | Automated small changes to the HAL header (add/remove/reorder functions) → verify `embersim check` detects changes. | Medium |
| G5 | **Repeated CI execution** | 10 consecutive CI runs on the same commit → all pass. Detects flaky tests, network-dependent failures. | Medium |
| G6 | **Compiler version matrix** | Test with GCC 11, 12, 13, 14 and Clang 15, 16, 17, 18. Prevents toolchain-specific regressions. | Medium |
| G7 | **Memory leak detection** | Valgrind on native binary. Detects leaks in mock implementations. | Medium |
| G8 | **Golden baseline evolution** | Defined process for updating baseline when trace format changes. Currently no policy. | Medium |

---

## 5. Scores (Independent, From Scratch)

Each score is justified with evidence. No score exceeds 8/10 because no empirical evidence exists.

| Category | Score | Justification |
|----------|-------|--------------|
| **Architecture** | 7/10 | BSP/App separation is clean. Peripheral handle ownership is well-defined. Deduction: (a) EXTI callback is dead code (-1), (b) SPI error path fabricates conditions instead of testing the mock (-1), (c) UART state machine adds complexity without trace benefit (-1). All deductions are from source inspection, not execution. |
| **Firmware quality** | 6/10 | HAL usage is realistic in structure. Deduction: (a) sensor.c error path is fabricated, not HAL-driven (-2), (b) EXTI callback is empty (-1), (c) `app_get_state()` returns mutable pointer, is dead code (-1). |
| **Simulation quality** | 4/10 | **Zero empirical evidence.** Design intent is sound but: (a) ABI compatibility unverified (-2), (b) trace determinism unverified (-2), (c) SPI error path doesn't exercise mock (-1), (d) EXTI path doesn't validate NVIC dispatch (-1). Deductions are substantial because simulation quality IS the thing being validated. |
| **Release engineering** | 6/10 | Makefile has good targets (asan, ubsan, analyze, clang). Deduction: (a) strict-warning flags unverified against actual code (-1), (b) no CI pipeline exists yet (-1), (c) no binary reproducibility target (-1), (d) no compiler version matrix (-1). |
| **Documentation** | 7/10 | Governance and acceptance framework are comprehensive. Deduction: (a) sensor.c error path documented incorrectly — claims HAL exercise that doesn't happen (-1), (b) UART state machine claims "richer trace" incorrectly (-1), (c) some documents reference "12 gates" while current count is 14 — version skew risk (-1). |
| **Maintainability** | 8/10 | Module boundaries are clear. File sizes are small (< 160 lines each). Single point of truth for peripheral handles (board.h). Deduction: (a) RVP is uncommitted and unfrozen (-1), (b) no version identifier in source files (-1). |
| **Developer experience** | 5/10 | Well-structured code. Deduction: (a) onboarding never tested (-2), (b) `embersim init` never run against this fixture (-2), (c) Makefile never executed (-1). Developer experience is the most evidence-dependent category. |
| **Regression quality** | 5/10 | P0-09 and P0-10 design good regression tests. Deduction: (a) never executed (-2), (b) only tests HAL call removal, not parameter changes, ordering changes, or timing changes (-1), (c) fault path never triggered in normal execution (-1), (d) SPI error path is fabricated, so error-recovery regression testing is invalid (-1). |
| **Industrial readiness** | 5/10 | Governance is professional. Deduction: (a) zero empirical evidence (-2), (b) RVP is unfrozen and uncommitted (-1), (c) no CI integration exists (-1), (d) no external reviewer has validated the fixture (-1). |
| **Certification readiness** | **2/10** | The certification report is a template. Zero of 14 gates have been executed. Four code-level issues (F1-F4) would cause specific gates to produce incorrect evidence if executed today. The certification framework is complete but the certification candidate is not. |
| **Overall readiness** | **4/10** | Strong design, zero evidence, four blocking code issues. Not ready for certification execution. The correct state is: fix F1-F4, freeze and commit the fixture, THEN execute the gates. |

---

## 6. Certification Decision

**REJECT**

RVP-001 is not ready for certification execution. This is not a rejection of the architecture or governance — both are strong. It is a rejection of the claim that the fixture is ready to produce meaningful evidence.

Four issues must be resolved before any gate execution:

| # | Issue | Blocks gates |
|---|-------|-------------|
| F1 | SPI error path fabricates HAL_BUSY | P0-06 (trace invariants would be wrong) |
| F2 | UART state machine claim is incorrect | Documentation accuracy (not a gate, but erodes credibility) |
| F3 | EXTI callback is dead code | P0-06 (NVIC dispatch not validated) |
| F4 | Strict-warning Makefile never compiled | P0-01, P0-01b (first gates, highest failure risk) |

After these are resolved AND the fixture is committed (actually frozen), the 14 gates can be executed. The certification report should then be filled with real data: real SHA-256 hashes, real sizeof values, real stopwatch times, real trace event counts.

---

## 7. Strengths (Acknowledged)

These aspects survived adversarial review:

1. **BSP/Application separation** is clean and mirrors production STM32 project structure. The `board_init()` → `app_init()` call chain is correct.
2. **Peripheral handle ownership** — `board.c` is the single definition point; `board.h` is the single declaration point. No bare `extern` in `.c` files.
3. **Governance framework** — RVP_POLICY.md, RVP001_ACCEPTANCE.md, and RVP001_CERTIFICATION.md together form a coherent release-gating process that would be credible at ST, ARM, or Google.
4. **Makefile design** — separate targets for asan, ubsan, analyze, and clang are well-structured.
5. **HAL surface** — 21 functions across 5 peripherals is the right scope for a baseline conformance suite. Not too narrow, not overreaching.
6. **Intentional constraints** — no RTOS, no DMA, no USB, no CAN. Correctly scoped.

---

## 8. Action Plan (Priority Order)

### Blocking (must fix before certification execution)

| Priority | Action | Files |
|----------|--------|-------|
| **P0** | Fix F1: Replace fabricated HAL_BUSY with a real mock configuration call, OR remove the error path and document deferral | `sensor.c` |
| **P0** | Fix F3: Make EXTI callback set a volatile flag; make `app_run()` check the flag instead of polling GPIO | `board.c`, `app.c` |
| **P1** | Fix F4: Add `U` suffix to integer literals; verify Makefile compiles with zero warnings | `board.c`, `uart_service.c`, `motor_control.c` |
| **P1** | Fix F2: Correct the UART state machine header comment — remove "richer trace" claim | `uart_service.c` |
| **P2** | Commit and freeze the fixture | `test-fixtures/projects/pilot-reference/` |

### Execution (after blocking fixes)

| Priority | Action |
|----------|--------|
| **P2** | Execute gates P0-01 through P0-01e (native build ± sanitizers ± static analysis) |
| **P2** | Execute gates P0-02 through P0-06 (EmberSim init → trace) |
| **P2** | Execute gate P0-02b (ABI compatibility — sizeof/offsetof measurements) |
| **P2** | Execute gate P0-07 (100-run determinism) |
| **P2** | Execute gates P0-08 through P0-14 (baseline, regression, CI, onboarding, fault injection) |
| **P3** | Fill certification report with actual measurements |
| **P3** | Sign Phase A certification |
| **P4** | Add P0-06b (long-duration stability, 5000 ms) |
| **P4** | Add G1 (binary reproducibility) |
| **P4** | Add CI pipeline for RVP-001 as mandatory PR gate |

### Deferred (post-certification)

| Priority | Action |
|----------|--------|
| **P5** | Add G4 (mutation testing) |
| **P5** | Add G6 (compiler version matrix) |
| **P5** | Add G8 (golden baseline evolution policy) |
| **P5** | Create RVP-002 (ADC + DMA scope) |

---

## 9. Reviewer's Closing Statement

RVP-001 is a professionally designed conformance suite that has not been executed. The architecture, governance, and acceptance framework are strong enough that I would approve them for use at any of the organizations I've worked with — conditional on empirical validation.

However, four code-level issues prevent me from approving execution today. The SPI error path fabricates the condition it claims to test. The EXTI callback validates nothing. The UART state machine makes a trace claim that isn't true. And the Makefile's warning policy has never been verified against the code.

These are not architectural problems. They are implementation details that the design-review process was supposed to catch. They survived because no one has compiled or run this code.

The fix is straightforward: resolve F1-F4, commit the frozen fixture, and execute the 14 gates. At that point, I would expect RVP-001 to pass and become EmberSim's first certified release gate.

The design is ready. The code needs four targeted fixes. The evidence does not exist. The path forward is clear.
