# RVP Policy — EmberSim Reference Validation Project Governance

**Version:** 1.0.0
**Date:** 2026-07-21
**Applies to:** All RVP-NNN projects under `test-fixtures/projects/`

---

## 1. Purpose

RVP (Reference Validation Project) projects are the canonical conformance
suite for EmberSim. They are NOT examples, demos, or tutorials. They are
**release-gating quality gates**.

Every EmberSim release must pass every active RVP before shipping.
If any RVP fails, the release is blocked.

---

## 2. RVP Versioning

RVPs are versioned by number and scope. Each RVP validates a specific layer
of EmberSim. They are NOT cumulative — a new RVP does not replace an old one.

```
RVP-001 — Baseline HAL Validation
  Peripherals: GPIO, UART, TIM, SPI, NVIC
  Purpose:    Prove that HAL interception, mock generation, deterministic
              trace, and regression detection work for the core peripheral set.

RVP-002 — Future: DMA + ADC
  Peripherals: ADC, DMA, interrupt nesting
  Purpose:    Validate DMA transfer modeling and analog input simulation.

RVP-003 — Future: Multi-peripheral stress
  Peripherals: I2C, EEPROM, Watchdog, multiple timer instances
  Purpose:    Validate concurrent peripheral operation and resource contention.
```

New RVPs are created when EmberSim adds support for a new peripheral class.
Existing RVPs are never modified to add peripherals — they validate the
layer they were designed for.

---

## 3. RVP Invariants

Every RVP must satisfy these invariants at all times:

### 3.1 Architecture invariants

- **No RTOS.** RVPs are bare-metal STM32 HAL firmware.
- **No DMA.** DMA is validated in a separate RVP.
- **No USB, Ethernet, CAN.** These are out of scope until explicitly added.
- **No randomness.** Every execution must be deterministic.
- **No timing-dependent behavior.** The scheduler controls time, not wall clocks.
- **No direct register access.** All peripheral access must go through HAL APIs.
- **No inline assembly.** All code must be portable C11.
- **Hardware addresses are symbolic only.** Register base addresses (e.g.,
  `#define GPIOA ((GPIO_TypeDef *)0x40020000U)`) are realistic STM32-style
  definitions required by the HAL. Host execution must never dereference
  these addresses. Simulator mocks own all hardware state. This preserves
  the embedded abstraction boundary while maintaining architectural realism.

### 3.2 Build invariants

- **Zero warnings** under `-Wall -Wextra -Wpedantic -Werror` with both GCC and Clang.
- **Zero sanitizer errors** under AddressSanitizer and UndefinedBehaviorSanitizer.
- **Zero static analysis errors** under cppcheck (`--enable=all`).
- **Native build succeeds** via `make` before EmberSim is involved.

### 3.3 Trace invariants

- **100 consecutive runs** produce **100 identical traces**.
- Trace must contain events from every peripheral in the RVP's scope.
- Trace must contain at least one error-recovery path.
- Trace size must be > 1000 events (non-trivial execution).

### 3.4 Regression invariants

- Removing any HAL call in the primary execution path must produce a
  detectable regression (trace divergence).
- The regression must be reported as `Result: FAIL` with a specific
  divergence line number and expected/observed function names.

---

## 4. RVP Lifecycle

### Creation

A new RVP is created when:
- A new peripheral class is added to EmberSim (e.g., ADC, DMA).
- An existing peripheral's mock is substantially rewritten.
- A new architectural pattern needs validation (e.g., interrupt nesting).

### Modification

An existing RVP may be modified ONLY to:
- Fix a bug in the firmware (not in EmberSim).
- Add a deterministic error path that exercises existing peripherals.
- Improve trace coverage of already-supported HAL functions.
- Update the golden baseline after an intentional firmware change.

An existing RVP must NOT be modified to:
- Add new peripheral classes (create a new RVP instead).
- Add features that don't improve generator validation.
- Circumvent a failing test by weakening assertions.

### Freeze

When an RVP is certified (Phase A complete), it enters a **frozen** state.

**Semantic versioning policy for RVP modifications:**

| Change | Version action |
|--------|---------------|
| Documentation clarifications, comment fixes | No version bump |
| Acceptance gate wording, error message updates | Patch (1.0.0 → 1.0.1) |
| Firmware logic change (same HAL surface, different behavior) | Minor (1.0.0 → 1.1.0) |
| HAL surface change (add/remove HAL functions exercised) | Major (1.0.0 → 2.0.0) |
| Peripheral additions | New RVP (RVP-002) |
| Trace schema dependency change | New certification required |

**Allowed after freeze:**
- Documentation clarifications that do not change firmware behavior.
- Build-system maintenance required by newer toolchains (e.g., Makefile flag updates).
- Compatibility fixes required by newer compiler versions (same behavior, same trace).

**Not allowed after freeze without creating a new RVP version:**
- New peripherals.
- New HAL API calls in the primary execution path.
- Behavioral changes that alter the trace.
- Changes to the command protocol.
- Changes to the golden baseline (except toolchain-compatibility re-baselines with documented justification).

Frozen RVPs protect the integrity of the baseline. If an RVP must evolve,
create a new version (e.g., RVP-001 → RVP-001 v1.1) with a new certification
cycle, or create a new RVP for the new scope.

### Deprecation

An RVP is deprecated when:
- The peripheral model it validates is removed from EmberSim.
- A newer RVP subsumes its validation scope with equivalent or better coverage.

Deprecated RVPs are moved to `test-fixtures/archive/` and removed from CI.

---

## 5. CI Integration

Every RVP must have a corresponding CI job:

```yaml
rvp-001:
  runs-on: ubuntu-latest
  steps:
    - name: Native build (strict)
      run: make -C test-fixtures/projects/pilot-reference
    - name: Static analysis
      run: make -C test-fixtures/projects/pilot-reference analyze
    - name: EmberSim init
      run: |
        cargo run -p embersim-cli -- init \
          -f test-fixtures/projects/pilot-reference/firmware/Drivers/.../stm32f4xx_hal.h \
          -I test-fixtures/projects/pilot-reference/firmware/Core/Inc \
          -I test-fixtures/projects/pilot-reference/firmware/Drivers/.../Inc \
          -o test-fixtures/projects/pilot-reference/ember_sim
    - name: EmberSim check
      run: cargo run -p embersim-cli -- check -o test-fixtures/projects/pilot-reference/ember_sim
    - name: EmberSim run
      run: cargo run -p embersim-cli -- run -o test-fixtures/projects/pilot-reference/ember_sim
    - name: Compare baseline
      run: |
        cargo run -p embersim-cli -- compare \
          -g test-fixtures/projects/pilot-reference/baseline/trace.jsonl \
          -c test-fixtures/projects/pilot-reference/trace.jsonl
```

If the RVP CI job fails, the PR must not merge.

---

## 6. Golden Baseline Policy

- The golden baseline (`baseline/trace.jsonl`) is committed to the repository.
- It is updated ONLY when firmware behavior intentionally changes.
- Every baseline update commit must explain WHY the behavior changed.
- The baseline is never auto-updated by CI.

Baseline metadata (documented in the baseline directory or trace header):

| Field | Example |
|-------|---------|
| RVP version | RVP-001 v1.0.0 |
| EmberSim version | (git describe) |
| Generator version | (git describe) |
| Schema version | trace.jsonl v1 |
| Git commit | (full SHA) |
| Date generated | 2026-07-21 |

---

## 7. Adding a New RVP

To propose a new RVP:

1. Write a spec document (`docs/RVP_NNN_SPEC.md`) defining:
   - Scope (which peripherals, which HAL functions)
   - Acceptance criteria
   - Forbidden dependencies
   - Expected trace coverage

2. Implement the firmware under `test-fixtures/projects/rvp-NNN/`.

3. Run the full acceptance checklist.

4. Add the CI job.

5. Freeze the golden baseline.

The new RVP becomes active immediately upon merge.

---

## 8. Enforcement

This policy is enforced by:
- The RVP acceptance checklist (per-RVP).
- The CI pipeline (automatic on every PR).
- Code review (manual, for RVP modifications).
- The EmberSim release process (all RVPs must pass before tagging).

Exceptions require written justification and approval from the project maintainer.
