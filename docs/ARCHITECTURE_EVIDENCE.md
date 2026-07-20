# Architecture Evidence Log

**Purpose:** Every architectural claim must be supported by evidence. If evidence is missing, the claim stays unproven. This document is a living engineering artifact — updated with each validation milestone.

**Last updated:** 2026-07-19

---

## Architecture Gates

Progress is measured by gates passed, not features added.

| Gate | Claim | Evidence | Status |
|---|---|---|---|
| **G0** | Kernel compiles | `cargo build --workspace` passes | ✅ |
| **G1** | TIM executes on kernel | `test_tim_runtime` 6/6 PASS, deterministic trace | ✅ |
| **G2** | Independent second peripheral (UART) | `test_uart_runtime` 4/4 PASS, kernel diff = 0 | ✅ |
| **G3** | Cross-peripheral interaction (DMA) | — | ⬜ |
| **G4** | Third-party plugin (no internal code changes) | — | ⬜ |
| **G5** | Production-like firmware workload | `real-firmware/` — TIM+UART, 150K-line deterministic trace | ✅ |
| **G5b** | Failure diagnosis | Trace comparison: divergence at event #30036 | ✅ |
| **G5c** | External firmware | Firmware not written by EmberSim team | ⬜ |
| **G5d** | Customer pilot | External team validates workflow | ⬜ |

---

## Claims and Evidence

### Claim: Kernel is independent of peripherals

**Evidence:** UART Vertical Slice — added as a plugin with zero kernel changes.

| File | Lines changed for UART |
|---|---|
| `ember_sim_kernel.c` | 0 |
| `ember_sim_kernel.h` | 0 |
| `ember_regs.c` | 0 |
| `ember_regs.h` | 0 |
| `trace_log.c` | 0 |
| Scheduler | 0 |

**Status:** ✅ Strong evidence. DMA will test cross-peripheral independence.

---

### Claim: Deterministic execution

**Evidence:** TIM runtime produces identical 181-line trace across runs.

| Metric | Value |
|---|---|
| Lines | 181 |
| SHA256 | `E113449E9408B3838BD90B3DA47F81FA86881A523B874E1137BFF0AE8B48FF4B` |
| Runs compared | Run 1 vs Run 2 — 0 lines differ |

**Status:** ✅ Verified for TIM. UART determinism not yet measured.

---

### Claim: Peripheral API supports timer devices

**Evidence:** TIM runtime — period elapsed, PWM, input capture, state inspection, callback dispatch, interrupt enable/disable.

| Test | Result |
|---|---|
| Period elapsed via runtime | PASS |
| PWM pulse finished via runtime | PASS |
| Input capture via runtime | PASS |
| SR UIF set after tick (before dispatch) | PASS |
| UIF cleared and callback after dispatch | PASS |
| No callback when UIE disabled | PASS |

**Status:** ✅ TIM validates tick-driven peripheral class.

---

### Claim: Peripheral API supports serial/FIFO devices

**Evidence:** UART runtime — blocking TX/RX, interrupt-driven TX/RX with callbacks.

| Test | Result |
|---|---|
| Blocking TX (5 bytes) | PASS |
| Blocking RX (2 bytes) | PASS |
| IT TX with callback | PASS |
| IT RX with callback | PASS |

**Status:** ✅ UART validates state-machine peripheral class. Bugs found were peripheral semantics (writable mask, RX state machine) — not architecture failures.

---

### Claim: Cross-peripheral interaction

**Evidence:** — (DMA not yet implemented)

**Status:** ⬜ DMA is the first test of peripheral-to-peripheral communication. This is where abstractions typically begin to leak. Remaining uncertainty: can DMA be added as an `EmberPeripheral` that interacts with other peripherals through the bus without kernel changes?

---

### Claim: External plugin API is stable

**Evidence:** — (No external plugin has been built)

**Status:** ⬜ Requires: (1) a developer outside the core team writes a peripheral using only the SDK, (2) the SDK requires no changes to accommodate it.

---

### Claim: Real firmware validated

**Evidence:** — (No customer firmware has been run)

**Status:** ⬜ The next milestone: run one real STM32 firmware project that uses both TIM and UART, without kernel modification, producing a deterministic trace.

---

## Bug Classification

When a peripheral fails, classify before fixing:

| Class | Definition | Example |
|---|---|---|
| **Case A — Peripheral semantics** | Bug in the peripheral model's understanding of hardware behavior | UART SR writable mask, RX state machine |
| **Case B — Event architecture** | Peripheral requires a new event type or bus mechanism | Would indicate ADR-002 revisit |
| **Case C — Kernel coupling** | Peripheral requires a special case in kernel code | Architecture failure — stop and fix kernel interface |

UART found two Case A bugs, zero Case B, zero Case C.
