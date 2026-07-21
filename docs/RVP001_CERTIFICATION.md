# RVP-001 Certification Report

**Status:** Pending execution
**RVP version:** 1.0.0
**Certification phase:** Phase A — Reference Firmware Certification

---

## Purpose

This document is the permanent record of RVP-001 certification against a
specific EmberSim version. It is currently a **template** — all fields
marked "(fill)" require empirical evidence from executing the acceptance
gates. Once completed and signed, RVP-001 is frozen and becomes a stable
benchmark. Every subsequent EmberSim release must pass RVP-001 (Phase B —
Product Validation). If RVP-001 ever fails after certification, the
EmberSim release is blocked.

---

## Certification Metadata

| Field | Value |
|-------|-------|
| RVP-001 git commit | (fill) |
| EmberSim git commit | (fill) |
| Rust version | (fill — `rustc --version`) |
| GCC version | (fill — `gcc --version`) |
| Clang version | (fill — `clang --version`, or N/A) |
| Operating system (primary) | (fill — `uname -a` or `ver`) |
| Operating system (cross-check) | (fill — or N/A) |
| Certification date | (fill) |
| Reviewer | (fill) |
| Final approval | PENDING / APPROVED / REJECTED |

---

## Phase A — Reference Firmware Certification Gates

### Gate P0-01 — Native build succeeds (GCC, strict warnings)

```bash
cd test-fixtures/projects/pilot-reference
make clean && make
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Exit code | 0 | | |
| Binary | `build/rvp-001` exists | | |
| Compiler warnings | 0 (including `-Wconversion`, `-Wshadow`) | | |

---

### Gate P0-01b — Native build succeeds (Clang)

```bash
make clean && make CC=clang
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Exit code | 0 | | |
| Compiler warnings | 0 | | |

---

### Gate P0-01c — AddressSanitizer clean

```bash
make clean && make asan
ASAN_OPTIONS=detect_leaks=1 build/rvp-001 /tmp/rvp-trace.jsonl
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Exit code | 0 | | |
| ASan errors | 0 | | |

---

### Gate P0-01d — UndefinedBehaviorSanitizer clean

```bash
make clean && make ubsan
build/rvp-001 /tmp/rvp-trace.jsonl
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Exit code | 0 | | |
| UBSan errors | 0 | | |

---

### Gate P0-01e — Static analysis clean

```bash
make analyze
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Exit code | 0 | | |
| cppcheck errors | 0 | | |
| cppcheck warnings | 0 (or documented) | | |

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

### Gate P0-02b — Generated mock ABI verification

Manual inspection of `ember_sim/mocks/mock_hal.h` against
`firmware/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h`.

**sizeof verification:**

| Type | Firmware (bytes) | Generated mock (bytes) | Match? |
|------|-----------------|----------------------|--------|
| `GPIO_TypeDef` | | | |
| `GPIO_InitTypeDef` | | | |
| `UART_HandleTypeDef` | | | |
| `UART_InitTypeDef` | | | |
| `TIM_HandleTypeDef` | | | |
| `TIM_Base_InitTypeDef` | | | |
| `TIM_OC_InitTypeDef` | | | |
| `SPI_HandleTypeDef` | | | |
| `SPI_InitTypeDef` | | | |
| `RCC_OscInitTypeDef` | | | |
| `RCC_ClkInitTypeDef` | | | |

**Key field offset verification:**

| Type | Field | Firmware offset | Mock offset | Match? |
|------|-------|----------------|-------------|--------|
| `UART_HandleTypeDef` | `Instance` | | | |
| `UART_HandleTypeDef` | `pTxBuffPtr` | | | |
| `UART_HandleTypeDef` | `pRxBuffPtr` | | | |
| `TIM_HandleTypeDef` | `Instance` | | | |
| `TIM_HandleTypeDef` | `Init.Period` | | | |
| `TIM_HandleTypeDef` | `Channel` | | | |
| `SPI_HandleTypeDef` | `Instance` | | | |
| `SPI_HandleTypeDef` | `pTxBuffPtr` | | | |
| `SPI_HandleTypeDef` | `pRxBuffPtr` | | | |

**Enum value verification:**

| Enum | Value | Firmware | Mock | Match? |
|------|-------|----------|------|--------|
| `HAL_StatusTypeDef` | `HAL_OK` | | | |
| `HAL_StatusTypeDef` | `HAL_ERROR` | | | |
| `HAL_StatusTypeDef` | `HAL_BUSY` | | | |
| `HAL_StatusTypeDef` | `HAL_TIMEOUT` | | | |

**ABI verification method:** (fill — e.g., "compiled test program printing sizeof/offsetof", "manual header comparison")

---

### Gate P0-03 — Zero missing HAL symbols

```bash
embersim check -o ember_sim
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
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

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| "Running simulation..." | Printed | | |
| Exit code | 0 | | |

---

### Gate P0-06 — Trace produced

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| "Trace generated (N events)" | N > 1000 | | |
| `trace.jsonl` exists | Yes | | |
| GPIO events present | WritePin, TogglePin | | |
| UART events present | Init, Receive_IT, Transmit | | |
| TIM events present | Base_Init, Base_Start_IT, PWM_Start | | |
| SPI events present | Init, TransmitReceive | | |
| NVIC events present | SetPriority, EnableIRQ | | |
| Callback events present | PeriodElapsedCallback, EXTI_Callback | | |
| Error-recovery events | HAL_BUSY → retry → HAL_OK | | |

---

### Gate P0-07 — Determinism: 100 consecutive identical traces

```bash
for i in $(seq 1 100); do
    embersim run -o ember_sim > /dev/null 2>&1
    cp trace.jsonl run_$i.jsonl
done
sha256sum run_*.jsonl | awk '{print $1}' | sort -u | wc -l
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Unique SHA-256 hashes | 1 | | |
| Trace count | 100 | | |
| Any divergence | None | | |

**Trace checksum (SHA-256):** (fill)

---

### Gate P0-07b — Cross-platform determinism

Repeat P0-07 on a second operating system.

| Measure | Primary OS | Secondary OS | Match? |
|---------|-----------|-------------|--------|
| OS | (fill) | (fill) | |
| SHA-256 of trace | (fill) | (fill) | |

---

### Gate P0-08 — Golden baseline created

```bash
embersim baseline create -t trace.jsonl
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| Exit code | 0 | | |
| `baseline/trace.jsonl` | Exists | | |
| Baseline event count | Matches P0-06 count | | |

**Baseline checksum (SHA-256):** (fill)

---

### Gate P0-09 — Intentional regression detected

**Break:** In `uart_service.c`, comment out line 87:
```c
/* HAL_UART_Transmit(&huart2, (uint8_t *)response, len, 100); */
```

```bash
embersim run -o ember_sim
```

| Measure | Expected | Actual | Pass? |
|---------|----------|--------|-------|
| "Result: FAIL" | Printed | | |
| "First divergence" | Printed with line number | | |
| "Expected" | HAL_UART_Transmit | | |
| "Observed" | Different/missing event | | |

**Revert the break.** Confirm PASS returns. Then re-create baseline.

---

### Gate P0-10 — Baseline update restores green

```bash
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

---

### Gate P0-12 — End-to-end onboarding under 15 minutes

| Step | Time | Notes |
|------|------|-------|
| `embersim doctor` | ___ | |
| `embersim init` | ___ | |
| `embersim check` | ___ | |
| `embersim run` | ___ | |
| `embersim baseline create` | ___ | |
| `embersim ci-init` | ___ | |
| **Total** | **___** | **Target: < 15 min** |

---

## Trace Invariants Verification

| Invariant | Expected value | Actual | Pass? |
|-----------|---------------|--------|-------|
| Total events | > 1000 | | |
| `HAL_GPIO_WritePin` count | ≥ 1 | | |
| `HAL_GPIO_TogglePin` count | ≥ 5 | | |
| `HAL_UART_Transmit` count | ≥ 1 | | |
| `HAL_UART_Receive_IT` count | ≥ 1 | | |
| `HAL_TIM_Base_Start_IT` count | 1 | | |
| `HAL_TIM_PWM_Start` count | ≥ 1 | | |
| `HAL_TIM_PeriodElapsedCallback` count | ≥ 5 | | |
| `HAL_SPI_TransmitReceive` count | ≥ 1 | | |
| `HAL_BUSY` recovery path | Architectural (code present, deferred to future mock config) | | |
| `HAL_NVIC_SetPriority` count | 1 | | |
| `HAL_NVIC_EnableIRQ` count | 1 | | |

---

## Phase A Certification Decision

### Prerequisites

- [ ] All 12 gates pass
- [ ] ABI compatibility verified (P0-02b)
- [ ] 100-run determinism confirmed (P0-07)
- [ ] Cross-platform determinism confirmed (P0-07b, or documented N/A)
- [ ] Sanitizer builds clean (P0-01c, P0-01d)
- [ ] Static analysis clean (P0-01e)
- [ ] Trace invariants verified
- [ ] No manual edits to generated mocks required

### Decision

**PENDING / APPROVED / REJECTED**

### Signatures

| Role | Name | Date | Signature |
|------|------|------|-----------|
| Reviewer | (fill) | (fill) | |
| Approver | (fill) | (fill) | |

---

## Phase B — Product Validation (Post-Certification)

Once RVP-001 is certified, every EmberSim commit must pass:

```text
RVP-001 (frozen, certified)
        │
        ▼
Every EmberSim PR
        │
        ▼
Run RVP-001 CI job
        │
        ├── PASS → merge allowed
        └── FAIL → merge blocked
```

The RVP-001 CI job runs the following (automated subset of Phase A):

1. Native build (GCC, strict warnings)
2. `embersim init` → `embersim check` → `embersim run`
3. `embersim compare -g baseline/trace.jsonl -c trace.jsonl`

Any divergence from the certified golden baseline blocks the release.

---

## Revision History

| Version | Date | Author | Change |
|---------|------|--------|--------|
| 1.0.0 | (fill) | (fill) | Initial certification |
