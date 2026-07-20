# G5b — Failure Diagnosis: Completion Report

**Date:** 2026-07-19  
**Status:** ✅ Complete  

## Acceptance Criteria

| Requirement | Result |
|---|---|
| Known defect introduced without modifying EmberSim | ✅ `HAL_UART_Transmit` commented out in timer callback |
| Deterministic execution preserved | ✅ Both golden and buggy traces identical across runs |
| First divergence identified | ✅ Line #30036 — `HAL_UART_Transmit` (golden) vs `REGISTER_EVENT` (buggy) |
| Actionable from trace alone | ✅ Engineer can see: callback fires, UART transmit is missing |
| Golden trace unchanged | ✅ Original SHA256 preserved |

## Experiment

**Defect:** `HAL_UART_Transmit` call removed from `HAL_TIM_PeriodElapsedCallback`.

**Golden output (correct firmware):**
```
30.034ms  HAL_UART_Transmit "started"
```

**Buggy output (defective firmware):**
```
30.034ms  REGISTER_EVENT (TIM2 SR cleared — callback completed, but no transmit)
```

**First divergence:** Line 30036. Golden contains `HAL_UART_Transmit`. Buggy contains `REGISTER_EVENT` (CNT increment continues — firmware keeps running, just silently drops data).

## Product Output

```
=== Firmware Regression ===

Result: FAIL

Golden trace:     trace_golden.jsonl
Candidate trace:  trace_buggy.jsonl

Deterministic:
  ✓ Golden trace deterministic
  ✓ Candidate trace deterministic

── First behavioural divergence ──

Expected:
  TIM IRQ
  → HAL_TIM_IRQHandler
  → HAL_TIM_PeriodElapsedCallback
  → HAL_UART_Transmit

Observed:
  TIM IRQ
  → HAL_TIM_IRQHandler
  → HAL_TIM_PeriodElapsedCallback
  → (no HAL_UART_Transmit)

First divergence:
  Event #30036
  Time: 30.034 ms

── Evidence ──

Expected event:
  {"peripheral":"UART","func":"HAL_UART_Transmit","started"}

Observed event:
  {"peripheral":"runtime","func":"REGISTER_EVENT","register":"SR","reason":"HAL cleared UIF"}

Difference:
  HAL_UART_Transmit did not occur after the timer callback.
  Timer callback completed normally (UIF cleared) but no UART transmit was triggered.

── Assessment ──

Evidence strength: HIGH
  Definition: Same divergence reproduced across multiple deterministic runs.
  Both golden and candidate traces are deterministic (identical across 3 runs).
  The divergence is consistent — same event, same timestamp, every run.

── Candidate causes (not verified by trace alone) ──

  • HAL_UART_Transmit not executed (call removed or commented out)
  • Control flow bypassed UART transmit (condition timer_ticks % 10 == 0 not met)
  • UART initialization prevented transmit path (huart2 not configured)

  The trace alone does not distinguish between these. Further investigation
  would require examining the source code at the divergence point.
```

## What Was Proven vs What Was Not

| Claim | Status |
|---|---|
| Deterministic regression detection | ✅ Proven |
| First behavioural divergence identification | ✅ Proven |
| Root-cause diagnosis | ⚠️ Not yet — trace identifies *what* diverged, not *why* |
| Automatic explanation generation | ⚠️ Future work — currently manual trace inspection |

## Defensible Claim

> **EmberSim can deterministically identify the first behavioural divergence between a known-good firmware execution and a regressed execution without requiring hardware.**

This is trace comparison, not automated diagnosis. Evidence and interpretation are kept separate. The tool identifies *what* changed; the engineer determines *why*.
