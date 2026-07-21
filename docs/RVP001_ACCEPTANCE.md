# RVP-001 — EmberSim Reference Validation Project Acceptance Checklist

**Status:** Pending execution
**Date:** (fill after validation run)
**EmberSim commit:** (fill)
**Validator:** (fill)

---

## Purpose

RVP-001 is the canonical reference firmware for EmberSim. It is NOT an example or
a demo. It is a **release-gating conformance suite**.

Every EmberSim release must pass all 12 gates before shipping. If RVP-001 fails,
the release is blocked.

---

## Acceptance Gates

### Gate P0-01 — Native build succeeds

```bash
cd test-fixtures/projects/pilot-reference
make clean && make
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Exit code | 0 | | |
| Binary exists | `build/rvp-001` | | |
| Compiler warnings | 0 | | |

---

### Gate P0-02 — EmberSim init succeeds

```bash
embersim init \
  -f firmware/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h \
  -I firmware/Core/Inc \
  -I firmware/Drivers/STM32F4xx_HAL_Driver/Inc \
  -o ember_sim
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Exit code | 0 | | |
| `ember_sim/mocks/mock_hal.h` | Exists | | |
| `ember_sim/mocks/mock_hal.c` | Exists | | |
| `ember_sim/mocks/mock_tim.h` | Exists | | |
| `ember_sim/mocks/mock_tim.c` | Exists | | |
| `ember_sim/mocks/mock_uart.c` | Exists | | |
| `ember_sim/mocks/mock_spi.c` | Exists | | |
| `ember_sim/mocks/mock_spi.h` | Exists | | |
| `ember_sim/host_main.c` | Exists | | |
| `ember_sim/embersim.toml` | Exists | | |

---

### Gate P0-03 — Zero missing HAL symbols

```bash
embersim check -o ember_sim
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Exit code | 0 | | |
| HAL functions: MISSING | 0 | | |
| HAL types: MISSING | 0 | | |
| CMSIS macros: MISSING | 0 | | |
| Includes: SUGGESTED | ≤ 1 (documented) | | |

---

### Gate P0-04 — Simulation builds

```bash
embersim run -o ember_sim
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| "Building firmware..." | Printed | | |
| "Build successful" | Printed | | |

---

### Gate P0-05 — Simulation executes

(Continuation of P0-04)

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| "Running simulation..." | Printed | | |
| Exit code | 0 | | |

---

### Gate P0-06 — Trace produced

(Continuation of P0-05)

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| "Trace generated (N events)" | N > 1000 | | |
| `trace.jsonl` exists | Yes | | |
| Trace contains GPIO events | Yes (WritePin, TogglePin) | | |
| Trace contains UART events | Yes (Init, Receive_IT, Transmit) | | |
| Trace contains TIM events | Yes (Base_Init, Base_Start_IT, PWM_Start) | | |
| Trace contains SPI events | Yes (Init, TransmitReceive) | | |
| Trace contains callback events | Yes (PeriodElapsedCallback) | | |
| Trace contains NVIC events | Yes (SetPriority, EnableIRQ) | | |

---

### Gate P0-07 — Two consecutive traces identical

```bash
embersim run -o ember_sim
cp trace.jsonl trace1.jsonl
embersim run -o ember_sim
cp trace.jsonl trace2.jsonl
diff trace1.jsonl trace2.jsonl
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| `diff` exit code | 0 | | |
| Event count (run 1) | N | | |
| Event count (run 2) | N (identical) | | |

---

### Gate P0-08 — Golden baseline created

```bash
embersim baseline create -t trace.jsonl
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Exit code | 0 | | |
| `baseline/trace.jsonl` | Exists | | |
| Baseline event count | Matches trace | | |

---

### Gate P0-09 — Intentional regression detected

**Break:** Comment out `HAL_UART_Transmit` call in `uart_service.c:87`.

```bash
embersim run -o ember_sim
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| "Result: FAIL" | Printed | | |
| "First divergence" | Printed with line number | | |
| "Expected" | UART Transmit event | | |
| "Observed" | Different/no UART event | | |

**Revert the break.** Confirm PASS returns.

---

### Gate P0-10 — Baseline update restores green

After reverting the P0-09 break:

```bash
embersim baseline create -t trace.jsonl
embersim run -o ember_sim
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| "Result: PASS" | Printed | | |

---

### Gate P0-11 — CI workflow generates

```bash
embersim ci-init
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Exit code | 0 | | |
| `.github/workflows/embersim.yml` | Exists | | |
| Workflow contains 10 steps | Checkout → Compare baseline | | |

---

### Gate P0-12 — Entire onboarding under 15 minutes

Start timing from `git clone` of a fresh EmberSim checkout.

| Step | Time | Notes |
|------|------|-------|
| Install prerequisites (if needed) | ___ | |
| `embersim doctor` | ___ | |
| `embersim init` | ___ | |
| `embersim check` | ___ | |
| `embersim run` | ___ | |
| `embersim baseline create` | ___ | |
| `embersim ci-init` | ___ | |
| **Total** | **___** | **Target: < 15 min** |

---

### Gate P0-13 — Clean repository validation

Start from a completely fresh state. No prior EmberSim installation, no cached
build artifacts, no unpublished notes. Use ONLY the published documentation
(`README.md`, `docs/GETTING_STARTED.md`, etc.).

```bash
git clone <this-repo> /tmp/rvp-test
cd /tmp/rvp-test/test-fixtures/projects/pilot-reference
# Follow only GETTING_STARTED.md from this point forward.
# Do not consult source code or internal notes.
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Fresh clone succeeds | Yes | | |
| All steps completed without consulting source | Yes | | |
| All steps completed without unpublished notes | Yes | | |
| Documentation was sufficient at every step | Yes | | |
| Any documentation error found | None | | |
| Total time (fresh clone → green CI) | < 15 minutes | | |

**This gate proves that the documented onboarding workflow works for a
first-time user with no prior EmberSim knowledge.**

---

### Gate P0-14 — Fault injection validation

A conformance suite that only proves success is incomplete. These negative tests
verify that the validator detects failures.

**Configuration/structural injections:**

| Injection | Command | Expected result |
|-----------|---------|----------------|
| Remove a HAL function from firmware | `embersim check -o ember_sim` | Reports MISSING |
| Change firmware behavior (remove UART TX) | `embersim run -o ember_sim` | `Result: FAIL` with divergence |
| Corrupt baseline (truncate file) | `embersim compare -g baseline/trace.jsonl -c trace.jsonl` | Reports mismatch or parse error |
| Invalid config (missing `embersim.toml`) | `embersim run` | Clear error message, exit ≠ 0 |
| Missing `-f` flag | `embersim init` | Clear error message, suggests `--auto` |

**Firmware behavioral mutations (closer to real regressions):**

| Mutation | Expected result |
|----------|----------------|
| Change timer prescaler (8399 → 4199) | Trace divergence: different tick rate, callback count changes |
| Change UART baud rate (115200 → 57600) | Trace divergence: `HAL_UART_Init` parameters differ |
| Flip GPIO LED polarity (active-low → active-high) | Trace divergence: `WritePin` initial state differs |
| Change SPI CPOL (LOW → HIGH) | Trace divergence: `HAL_SPI_Init` parameters differ |

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| All 5 structural injections detected | Yes | | |
| All 4 behavioral mutations detected | Yes | | |
| Each failure has actionable error message | Yes | | |
| No false negatives (failure undetected) | 0 | | |

**This gate proves that EmberSim's validation detects both gross failures
and subtle behavioral changes — not just confirms success.**

---

## Final Verdict

| Gate | Status |
|------|--------|
| P0-01 | |
| P0-02 | |
| P0-03 | |
| P0-04 | |
| P0-05 | |
| P0-06 | |
| P0-07 | |
| P0-08 | |
| P0-09 | |
| P0-10 | |
| P0-11 | |
| P0-12 | |
| P0-13 | |
| P0-14 | |

**All 14 gates must pass.** Any failure blocks the release.

---

## Release Integration

After all gates pass, integrate RVP-001 into the EmberSim CI pipeline:

```yaml
# .github/workflows/embersim-self-test.yml
- name: Run RVP-001 validation
  run: |
    cd test-fixtures/projects/pilot-reference
    cargo run --manifest-path ../../../Cargo.toml -p embersim-cli -- \
      init -f firmware/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h \
      -I firmware/Core/Inc \
      -I firmware/Drivers/STM32F4xx_HAL_Driver/Inc \
      -o ember_sim
    cargo run --manifest-path ../../../Cargo.toml -p embersim-cli -- check -o ember_sim
    cargo run --manifest-path ../../../Cargo.toml -p embersim-cli -- run -o ember_sim
    cargo run --manifest-path ../../../Cargo.toml -p embersim-cli -- \
      compare -g baseline/trace.jsonl -c trace.jsonl
```

If RVP-001 fails, the EmberSim release fails.
