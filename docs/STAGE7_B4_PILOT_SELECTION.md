# Stage 7 B4 — Pilot Selection

**Date:** 2026-07-21
**Status:** Selection framework established — candidates ranked, Pilot 0 recommended first
**Decision authority:** Elias Zebene

---

## 1. Purpose

B4 is not "find any STM32 repo and run EmberSim against it." The dry-run must produce evidence that:

1. A normal embedded engineer can onboard without assistance.
2. The HAL interception model works for real firmware patterns.
3. Regression detection creates business value (catches a change that matters).
4. The project profile resembles a future paying customer.

A poorly chosen candidate produces false negatives — failures caused by the project choice, not by EmberSim. That wastes time and erodes confidence in the product.

---

## 2. B4 Acceptance Criteria (Refined)

A candidate project must satisfy ALL of:

| # | Criterion | Rationale |
|---|-----------|-----------|
| C1 | Builds outside EmberSim | Must have a working baseline before simulation |
| C2 | Uses STM32 HAL APIs | EmberSim intercepts at the HAL boundary |
| C3 | Uses GPIO + UART + TIM (minimum) | Validates the three most-tested peripherals |
| C4 | Has deterministic startup | Trace must be reproducible run-to-run |
| C5 | No RTOS | Scheduler simulation is not in scope |
| C6 | No DMA dependency | DMA peripheral is not implemented |
| C7 | No direct peripheral register access | MMIO simulation is not in scope |
| C8 | GitHub-compatible | CI workflow must run without adaptation |
| C9 | Under 50 source files | Keeps onboarding time manageable |
| C10 | Licensed for derivative use | Must be able to modify and commit baseline |

### Automatic disqualifiers

| Condition | Why |
|-----------|-----|
| FreeRTOS or any RTOS | Not supported — failure is meaningless |
| USB, Ethernet, CAN, DAC, ADC peripherals | Not implemented — failure is predictable |
| Direct register manipulation (`GPIOA->ODR = …`) | Not supported — HAL API required |
| Inline assembly or CMSIS intrinsics as core logic | Not supported — `--repair` covers stubs only |
| Requires specific hardware to boot | Simulation must run headless |
| Proprietary/closed-source firmware | Cannot commit baseline or share results |

---

## 3. Candidate Ranking

### 🥇 Candidate 1 — STM32CubeF4 Nucleo Example (Recommended first external validation)

**Repository:** `https://github.com/STMicroelectronics/STM32CubeF4`
**Target path:** `Projects/STM32F4xx-Nucleo/Examples/`
**Suggested example:** `UART/UART_Printf` + `TIM/TIM_PWM` + `GPIO/GPIO_IOToggle` (combined or individually)

| Criterion | Result |
|-----------|--------|
| C1 — Builds outside EmberSim | ✅ Standard CubeMX/gcc |
| C2 — STM32 HAL APIs | ✅ Native HAL |
| C3 — GPIO + UART + TIM | ✅ All three covered |
| C4 — Deterministic startup | ✅ No dynamic init |
| C5 — No RTOS | ✅ Examples are bare-metal |
| C6 — No DMA | ✅ No DMA in these examples |
| C7 — No register access | ✅ Uses HAL exclusively |
| C8 — GitHub compatible | ✅ |
| C9 — Under 50 source files | ✅ Examples are small |
| C10 — Licensed | ✅ ST license (BSD-3-Clause) |

**Strengths:**
- Official ST code — industry credibility
- HAL-native — no adaptation needed
- Small and self-contained — fast onboarding
- STM32F4 — EmberSim's best-tested family

**Risks:**
- **Medium.** ST examples sometimes depend heavily on Cube-generated startup files (`system_stm32f4xx.c`, startup assembly). These need include path configuration but don't need simulation — they're compiled but their hardware effects are stubbed.
- Some examples use `BSP` (board support package) drivers on top of HAL. Prefer examples that call HAL directly.

**Best fit for Pilot 1.** Low risk, high credibility, fast execution.

---

### 🥈 Candidate 2 — STM32G0 Bootloader

**Repository:** `https://github.com/jonahswain/stm32g0-bootloader`

| Criterion | Result |
|-----------|--------|
| C1 — Builds outside EmberSim | ✅ Makefile-based |
| C2 — STM32 HAL APIs | ✅ STM32G0 HAL |
| C3 — GPIO + UART + TIM | ⚠️ UART and GPIO likely; TIM unconfirmed |
| C4 — Deterministic startup | ⚠️ Bootloaders often have conditional init paths |
| C5 — No RTOS | ✅ Bare-metal |
| C6 — No DMA | ⚠️ Bootloaders may use DMA for flash writes |
| C7 — No register access | ⚠️ May touch flash controller registers directly |
| C8 — GitHub compatible | ✅ |
| C9 — Under 50 source files | ✅ Small project |
| C10 — Licensed | ✅ MIT |

**Strengths:**
- Closer to a real product than an ST example
- Makefile build — simpler than CubeMX
- STM32G0 — validates second MCU family
- Bootloader + update mechanism = realistic use case

**Risks:**
- **Medium-high.** Bootloaders typically touch flash registers, vector table relocation, and memory maps — areas where EmberSim's architecture boundaries are sharp. A failure here is informative but not a fair first test.
- May use DMA for flash programming.
- Conditional initialization (checking boot pins, flash flags) may produce non-deterministic traces.

**Recommended as Pilot 2** — after Pilot 1 confirms the basic workflow works.

---

### 🥉 Candidate 3 — Controlled Internal Reference Firmware

**Not a repo — a to-be-created small firmware.**

Profile:

```
STM32F407
 ├── UART command interface (HAL_UART_Transmit/Receive)
 ├── TIM PWM motor output (HAL_TIM_PWM_Start/SetCompare)
 ├── GPIO fault LED (HAL_GPIO_WritePin/TogglePin)
 └── SPI sensor mock (HAL_SPI_Transmit/Receive)
```

Size: 10–20 C files.
Build: Makefile or simple gcc invocation.

| Criterion | Result |
|-----------|--------|
| C1 — Builds outside EmberSim | ✅ By design |
| C2 — STM32 HAL APIs | ✅ By design |
| C3 — GPIO + UART + TIM | ✅ Core peripherals |
| C4 — Deterministic startup | ✅ By design |
| C5 — No RTOS | ✅ By design |
| C6 — No DMA | ✅ By design |
| C7 — No register access | ✅ By design |
| C8 — GitHub compatible | ✅ By design |
| C9 — Under 50 source files | ✅ By design |
| C10 — Licensed | ✅ EmberSim-owned |

**Strengths:**
- Full control — no surprises from unknown code
- Matches ideal industrial customer profile (motor controller, sensor node, actuator)
- Becomes EmberSim's permanent "golden demo"
- Can be reused for every future Stage validation
- Eliminates project-selection risk from the first dry-run

**Risks:**
- Not "external" — doesn't prove external engineer onboarding
- Takes time to write (trade against hunting for a perfect external repo)

**Recommended as Pilot 0** — internal validation before external testing.

---

## 4. Projects Explicitly Rejected

| Project | Reason for rejection |
|---------|---------------------|
| FreeRTOS-based firmware | RTOS not supported — failure is meaningless |
| Full STM32CubeF4 package (as-is) | Too large — engineer spends time navigating ST structure, not validating EmberSim |
| Flight controller firmware (Betaflight, etc.) | RTOS-like scheduler, DMA, IMU drivers, USB, CAN — wrong customer profile |
| Any firmware using CMSIS-RTOS, ThreadX, Zephyr | RTOS not in scope |
| Any firmware requiring USB, Ethernet, CAN, ADC, DAC | Peripherals not implemented |
| Proprietary/closed-source firmware | Cannot commit baseline or publish results |

---

## 5. Recommended Execution Sequence

```
Pilot 0 (internal) ──────────────────────────────────────────
│  Create controlled STM32F407 reference firmware
│  Goal: prove the full workflow with zero surprises
│  Time: 1–2 days
│  Output: golden demo + filled B4 report
│
├── Pilot 1 (external evidence) ─────────────────────────────
│   │  STM32CubeF4 Nucleo UART/TIM/GPIO example
│   │  Goal: engineer < 30 min, no assistance, CI regression detected
│   │  Time: 1 day (after Pilot 0)
│   │  Output: filled B4 report
│   │
│   └── Pilot 2 (architecture stress) ──────────────────────
│        STM32G0 bootloader
│        Goal: push HAL boundary, discover real gaps
│        Time: 1–2 days
│        Output: filled B4 report + gap list
```

Each pilot uses the same report template (`docs/STAGE7_B4_DRY_RUN_REPORT.md`). Each produces its own GO / CONDITIONAL GO / NO-GO recommendation.

---

## 6. Expected Blockers by Pilot

### Pilot 0 — Controlled Reference

| Blocker | Likelihood | Mitigation |
|---------|-----------|-----------|
| CMSIS macro stubs needed | High | `--repair` handles this |
| Include path configuration friction | Medium | Document exact `-I` paths in the reference README |
| `app_init`/`app_run` refactor needed | Certain | Design firmware with the two-function model from the start |

### Pilot 1 — STM32CubeF4 Nucleo

| Blocker | Likelihood | Mitigation |
|---------|-----------|-----------|
| Cube-generated startup files cause build errors | Medium | Add startup files to `[build.sources]`; stub `SystemInit` |
| BSP layer wraps HAL — extra include paths needed | Medium | Prefer examples that call HAL directly; document BSP include paths |
| Example uses `main()` not `app_init()`/`app_run()` | Certain | Refactor per HAL_BOUNDARY.md guide |
| CMSIS core headers needed | High | Add `-I Drivers/CMSIS/Include` and `-I Drivers/CMSIS/Device/ST/STM32F4xx/Include` |

### Pilot 2 — STM32G0 Bootloader

| Blocker | Likelihood | Mitigation |
|---------|-----------|-----------|
| Flash controller register access | High | May need manual stubs or exclusion of flash write code |
| Vector table relocation | Medium | Not simulatable — exclude or stub |
| DMA for flash programming | Medium | Exclude DMA paths; test only UART command path |
| Conditional boot paths (boot pin check) | Medium | Force a single path via `app_init()` |

---

## 7. Decision Log

| Date | Decision | Rationale |
|------|----------|-----------|
| 2026-07-21 | B4 selection framework created | Strategic review: not all STM32 projects are valid B4 candidates |
| 2026-07-21 | Pilot sequence: 0 → 1 → 2 | De-risk: internal first, external second, stress third |
| 2026-07-21 | Rejected: RTOS, complex OSS, hobby drone firmware | Would produce false negatives — wrong first customer profile |

---

## 8. References

- [STAGE_7_PILOT_PLAN.md](STAGE_7_PILOT_PLAN.md) — §1 Ideal First Pilot Profile, §2 Onboarding Workflow
- [STAGE7_READINESS_REPORT.md](STAGE7_READINESS_REPORT.md) — §4 Pilot Blockers, §8 Go/No-Go Criteria
- [STAGE7_B4_DRY_RUN_REPORT.md](STAGE7_B4_DRY_RUN_REPORT.md) — Report template for each pilot execution

---

## 9. Next Action

**Proceed with Pilot 0:** Create the controlled internal reference firmware.

Next artifact: `test-fixtures/projects/pilot-reference/` containing the reference firmware project.

The selection framework does not require further approval — Pilot 0 is an internal artifact with no external dependencies. Pilot 1 and Pilot 2 selection is confirmed by this document unless overridden.
