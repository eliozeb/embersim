# Stage 7 B4 — External STM32 Dry-Run Report

**Status:** Pending execution
**Date:** (fill after dry-run)
**Tester:** (fill after dry-run)
**EmberSim commit:** (fill after dry-run)

---

## 1. Project Details

| Field | Value |
|-------|-------|
| Project name | (fill) |
| Repository URL | (fill) |
| MCU family | STM32F4 / STM32G0 / other: ___ |
| Build system | Makefile / simple gcc / CMake / other: ___ |
| HAL header path | (fill — e.g. Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h) |
| Peripherals used | TIM / UART / GPIO / SPI / I2C / other: ___ |
| RTOS? | Yes / No |
| USB/Ethernet/CAN? | Yes / No |
| Number of .c source files | (fill) |
| GitHub Actions CI? | Yes / No / Willing to add |

### Project profile match vs ideal pilot

| Attribute | Target | Actual | Match? |
|-----------|--------|--------|--------|
| MCU family | STM32F4 or STM32G0 | | |
| Build system | Makefile or simple gcc | | |
| HAL usage | 3–5 peripherals (TIM+UART+GPIO min) | | |
| RTOS | No | | |
| USB/Ethernet/CAN | No | | |
| CI | GitHub or willing | | |

---

## 2. Environment

| Field | Value |
|-------|-------|
| OS | (fill — Windows / macOS / Linux) |
| rustc version | (fill) |
| gcc/clang version | (fill) |
| EmberSim install method | (fill — cargo install --git ... / local build) |
| embersim doctor result | (fill — all PASS / failures listed below) |

### Doctor failures (if any)

```
(paste embersim doctor output)
```

---

## 3. Onboarding Execution Log

### Step 1: Install prerequisites

| Measure | Value |
|---------|-------|
| Rust already installed? | Yes / No |
| gcc already installed? | Yes / No |
| Time to install missing tools | ___ minutes |
| Blockers encountered | (none / describe) |

### Step 2: Initialize simulation workspace

**Command run:**

```bash
embersim init -f <hal-header> -I Core/Inc -I Drivers/CMSIS/Include -o ember_sim
```

| Measure | Value |
|---------|-------|
| Time to complete | ___ seconds |
| Command succeeded first try? | Yes / No |
| Error messages seen | (none / paste) |
| --auto attempted? | Yes / No — result: ___ |
| Were -I flags guessed correctly? | Yes / No — adjustments needed: ___ |

**init output summary:**

```
(paste relevant output)
```

### Step 3: Check project readiness

```bash
embersim check -o ember_sim
```

| Measure | Value |
|---------|-------|
| Time to complete | ___ seconds |
| HAL functions: OK | ___ |
| HAL functions: MISSING | ___ |
| HAL types: OK | ___ |
| HAL types: MISSING | ___ |
| CMSIS macros: STUBBED | ___ |
| CMSIS macros: MISSING | ___ |
| Total issues before simulation | ___ |

**MISSING items that required action:**

| Symbol | Kind | Resolution |
|--------|------|-----------|
| (fill) | (fill) | (fill) |

### Step 4: Run first simulation

```bash
embersim run -o ember_sim
```

| Measure | Value |
|---------|-------|
| Time to complete | ___ seconds |
| Build succeeded first try? | Yes / No |
| Trace generated? | Yes / No |
| Trace event count | ___ |
| No baseline found message? | Yes / No |

**Build errors (if any):**

```
(paste)
```

### Step 5: Create golden baseline

```bash
embersim baseline create -t trace.jsonl
```

| Measure | Value |
|---------|-------|
| Time to complete | ___ seconds |
| Baseline created? | Yes / No |
| Baseline event count | ___ |

### Step 6: Generate CI workflow

```bash
embersim ci-init
```

| Measure | Value |
|---------|-------|
| Time to complete | ___ seconds |
| Workflow generated? | Yes / No |
| Already exists warning? | Yes / No |

### Step 7: Configure GitHub

| Measure | Value |
|---------|-------|
| HAL_HEADER variable set? | Yes / No |
| EMBERSIM_REPO variable set? | Yes / No / Not needed |
| Time to configure | ___ minutes |
| Confusion points | (none / describe) |

### Step 8: Commit and push

| Measure | Value |
|---------|-------|
| Files committed | (list) |
| Push succeeded? | Yes / No |

### Step 9: CI run verification

| Measure | Value |
|---------|-------|
| CI triggered automatically? | Yes / No |
| All steps passed first run? | Yes / No |
| Total CI runtime | ___ minutes |
| Failed step (if any) | (fill) |

**CI run URL:** (fill)

---

## 4. Aggregated Measurements

| Metric | Target | Actual | Pass? |
|--------|--------|--------|-------|
| Time to first trace | < 15 minutes | ___ minutes | |
| Time to green CI | < 30 minutes | ___ minutes | |
| Unassisted steps | ≥ 80% | ___ / 9 steps | |
| Steps needing help | — | (list step numbers) | |
| CLI commands that failed first try | 0 | (list) | |
| HAL coverage | ≥ 90% | ___% | |
| CI runtime | < 5 minutes | ___ minutes | |

---

## 5. HAL Coverage Findings

### Functions used by firmware vs generated

| HAL Function | Generated? | Firmware calls it? | Status |
|---|---|---|---|
| (fill) | (fill) | (fill) | (fill) |

### Types used by firmware vs generated

| HAL Type | Available? | Firmware uses it? | Status |
|---|---|---|---|
| (fill) | (fill) | (fill) | (fill) |

### Summary

| Category | Count |
|---|---|
| HAL functions used by firmware | ___ |
| HAL functions covered by mocks | ___ |
| HAL functions MISSING | ___ |
| HAL types used by firmware | ___ |
| HAL types covered | ___ |
| HAL types MISSING | ___ |
| Coverage % | ___% |

---

## 6. Documentation Issues Found

During the dry-run, record every place where the provided documentation was:

- Incorrect (wrong command, wrong flag, wrong path)
- Missing (needed information not present)
- Confusing (correct but hard to follow)

| # | Document | Section | Issue type | Description |
|---|----------|---------|-----------|-------------|
| 1 | (fill) | (fill) | Incorrect / Missing / Confusing | (fill) |

---

## 7. CLI Friction Points

Record every CLI behavior that caused confusion or extra work:

| # | Command | What happened | Expected | Friction level |
|---|---------|--------------|----------|---------------|
| 1 | (fill) | (fill) | (fill) | Low / Medium / High |

Friction levels:
- **Low:** Understood after reading output, but could be clearer
- **Medium:** Had to read docs or --help to understand
- **High:** Had to read source code or ask for help

---

## 8. Regression Detection Test

### Intentional break introduced

**Change made:**

```
(describe the intentional change — e.g., commented out HAL_UART_Transmit call, changed timer period)
```

### Local regression detection

```bash
embersim run -o ember_sim
```

| Measure | Value |
|---------|-------|
| Result: FAIL? | Yes / No |
| First divergence line | ___ |
| Expected function | ___ |
| Observed function | ___ |
| Evidence strength | strong / weak |
| User understood output? | Yes / No |

### CI regression detection

| Measure | Value |
|---------|-------|
| CI run triggered on branch/PR? | Yes / No |
| CI reported FAIL? | Yes / No |
| CI divergence matched local? | Yes / No |
| User trusted the result? | Yes / No — why: ___ |

### Baseline update after intentional change

```bash
embersim baseline create -t trace.jsonl
git add baseline/trace.jsonl
git commit -m "Update golden baseline for <reason>"
git push
```

| Measure | Value |
|---------|-------|
| Baseline updated? | Yes / No |
| CI returned to green? | Yes / No |

---

## 9. Classification of All Issues

Apply the Stage 7 classification framework to every issue encountered:

| # | Issue | Classification | Action |
|---|-------|---------------|--------|
| 1 | (fill) | Documentation gap / UX friction / Missing HAL coverage / Missing peripheral / Product gap / Architecture limitation | (fill) |

**Classification definitions:**

| Classification | Definition | Action |
|---------------|-----------|--------|
| Documentation gap | Product does it, user couldn't find how | Improve docs |
| UX friction | Product does it, workflow is awkward | Improve CLI output, errors, defaults |
| Missing HAL coverage | HAL function has no stub | Add to stub generation |
| Missing peripheral | Peripheral not simulated | Record for roadmap; do NOT add |
| Product gap | Product cannot do something it promises | Fix code or narrow claim |
| Architecture limitation | Architecture fundamentally prevents something | Record for architecture review |

---

## 10. Final Recommendation

### Evidence summary

- [ ] Time to first trace < 15 minutes
- [ ] Time to green CI < 30 minutes
- [ ] Unassisted steps ≥ 80%
- [ ] Regression detection confirmed
- [ ] User trusted CI result
- [ ] Zero kernel/scheduler/trace changes required
- [ ] Fewer than 3 architecture-level issues

### Recommendation

**GO** / **CONDITIONAL GO** / **NO-GO**

(circle one)

### Rationale

(fill — 2–4 sentences explaining the recommendation)

### If CONDITIONAL GO — specific conditions

| # | Condition | Owner | Deadline |
|---|-----------|-------|----------|
| 1 | (fill) | (fill) | (fill) |

### If NO-GO — blocking issues

| # | Blocker | Proposed resolution |
|---|---------|-------------------|
| 1 | (fill) | (fill) |

---

## 11. Tester Notes

Free-form observations from the engineer who ran the dry-run:

```
(fill — anything not captured above: surprises, pleasant moments,
 frustrating moments, ideas for improvement, things they wish existed)
```

---

## Appendix A: Full Command Log

```
(paste every command run and its full output)
```

## Appendix B: Generated Files

```
(paste or attach: embersim.toml, .github/workflows/embersim.yml)
```
