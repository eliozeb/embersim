# Stage 7 — External Pilot Plan

**Status:** Planning
**Prerequisite:** S6.4 complete — hosted CI regression detection proven
**Objective:** Validate EmberSim with a real external firmware team

---

## 1. Ideal First Pilot Profile

The first pilot customer should match this profile, in priority order:

| Attribute | Target | Why |
|-----------|--------|-----|
| Project type | STM32F4 or STM32G0 firmware project | EmberSim's best-supported HAL families |
| Size | Single developer or 2–3 person team | Low coordination overhead |
| Build system | Makefile or simple gcc compilation | Avoids CMake/CubeIDE complexity on first pilot |
| HAL usage | Moderate — 3–5 peripherals (TIM + UART + GPIO minimum) | Enough to stress HAL coverage, not enough to hit gaps |
| Testing pain | Currently uses hardware for regression testing, or has no automated testing | Motivation to adopt is real |
| Toolchain | Already has GCC, comfortable with CLI | Reduces onboarding friction |
| CI | Uses GitHub or is willing to add it | Aligns with S6.4 proven workflow |

### Anti-profile — do NOT target for first pilot

- FreeRTOS or RTOS-based firmware
- Complex CubeMX-generated projects with 50+ source files
- Teams without CI experience
- Projects requiring USB, Ethernet, or CAN peripherals
- Safety-critical or medical device firmware

---

## 2. Onboarding Workflow

The pilot should follow the exact path validated in S6.4:

```
Pilot engineer receives:
  └── EmberSim README + pilot guide
        │
        v
  1. git clone <their-firmware-repo>
        │
        v
  2. embersim init -f <their-hal-header> -I <their-includes> -o ember_sim
        │
        v
  3. embersim check -o ember_sim
        │  └── resolve any MISSING items (add includes, use mock_hal.h boundary)
        │
        v
  4. embersim run -o ember_sim
        │  └── first trace generated
        │
        v
  5. embersim baseline create -t trace.jsonl
        │  └── commit baseline/trace.jsonl
        │
        v
  6. embersim ci-init
        │  └── commit .github/workflows/embersim.yml
        │
        v
  7. git push
        │
        v
  8. GitHub Actions: green run ✅
```

### Time target

Each step should take under 2 minutes for an engineer who has never seen EmberSim. Total: **clone → green CI in under 15 minutes.**

---

## 3. Required Documentation

Before the pilot begins, these documents must exist:

| Document | Contents | Audience |
|----------|----------|----------|
| `GETTING_STARTED.md` | Step-by-step from zero to green CI, with exact commands and expected output | First-time user |
| `HAL_BOUNDARY.md` | How to replace `#include "stm32f4xx_hal.h"` with `#include "mock_hal.h"` for simulation builds, and how to add `mock_tim.h` etc. | Firmware engineer |
| `CI_SETUP.md` | How `embersim ci-init` works, what variables to set, how the workflow acquires EmberSim | DevOps / team lead |
| `TROUBLESHOOTING.md` | Common errors and their fixes: "Header not found", "MISSING type", "No baseline found", "gcc not found" | First-time user |

These should be in the EmberSim repository, not in the pilot's project. The pilot should only need to read them once.

---

## 4. Success Metrics

### Primary (must achieve)

| Metric | Target | How measured |
|--------|--------|-------------|
| Time to first trace | < 15 minutes | Stopwatch from `git clone` to first `trace.jsonl` |
| Time to green CI | < 30 minutes total | Stopwatch from `git clone` to green Actions run |
| Unassisted steps | ≥ 80% | Count of steps completed without asking for help |
| Regression detection | Confirmed | Intentional break → CI catches it |
| Kernel unchanged | Confirmed | Diff against frozen S6.4 kernel |

### Secondary (nice to have)

| Metric | Target | How measured |
|--------|--------|-------------|
| HAL coverage | ≥ 90% of functions used by firmware | `embersim check` output |
| CI runtime | < 5 minutes | Actions log |
| Pilot NPS | ≥ 7/10 | Post-pilot survey |
| Bugs found by pilot | Documented | Issue tracker |

---

## 5. Pilot Timeline

```
Week 1 — Preparation
  ├── Write GETTING_STARTED.md, HAL_BOUNDARY.md, CI_SETUP.md, TROUBLESHOOTING.md
  ├── Identify pilot candidate (match profile above)
  ├── Verify embersim-cli builds and installs on their OS
  └── Dry-run the onboarding with a fresh checkout of their repo

Week 2 — Onboarding
  ├── Pilot engineer clones EmberSim and their firmware repo
  ├── 30-minute guided session (screen share or in-person)
  │     └── embersim init → check → run → baseline → ci-init → push
  ├── Pilot runs independently for remainder of week
  └── Daily 5-minute check-in: "What broke? What was confusing?"

Week 3 — Observation
  ├── Pilot uses EmberSim as part of their normal workflow
  ├── No scheduled guidance (they reach out if stuck)
  ├── Collect every friction point, error message, and workaround
  └── Mid-pilot survey: "What would make you keep using this?"

Week 4 — Retrospective
  ├── Final survey
  ├── 30-minute interview: "What worked? What didn't? Would you pay for this?"
  ├── Categorize all feedback:
  │     ├── Documentation gap (fix docs)
  │     ├── Bug (fix code)
  │     ├── Missing feature (evaluate for roadmap)
  │     └── Architecture limitation (discuss carefully — never rush)
  └── Write Stage 7 closure report
```

---

## 6. Feedback Collection

### Daily (lightweight)

```
- What did you try to do?
- Did it work the first time?
- If not, what error did you see?
- How did you resolve it?
```

### Weekly (structured)

```
- On a scale of 1–10, how likely are you to recommend EmberSim?
- What is the single biggest friction point?
- What one feature would make you use EmberSim daily?
- Did you trust the CI result? Why or why not?
```

### Exit interview

```
- If EmberSim disappeared tomorrow, what would you miss?
- What did EmberSim not do that you expected it to?
- Would you add EmberSim to your next project without being asked?
```

---

## 7. Architecture Areas That Remain Frozen

During the pilot, these must not change regardless of feedback:

| Area | Freeze reason |
|------|--------------|
| Kernel API (`EmberPeripheral`, event bus, NVIC dispatch) | Proven across G0–G5e |
| Scheduler (tick-driven execution) | Determinism depends on it |
| Trace format (JSON Lines) | Baseline compatibility depends on it |
| Plugin registration interface | Peripheral independence depends on it |
| Register model | All peripherals depend on it |

Pilot feedback may reveal that documentation or error messages are insufficient, that the CLI workflow has friction, or that a specific HAL function is missing — those are fixable within the frozen architecture. If feedback suggests an architecture change, it should be recorded for a future architecture review, not acted on during the pilot.

---

## 8. Criteria for Product Gaps vs. User Errors

Not all pilot feedback is a product gap. Classify every issue:

| Classification | Definition | Action |
|---------------|-----------|--------|
| **Documentation gap** | The product does it, but the user couldn't find how | Improve docs, add examples |
| **UX friction** | The product does it, but the workflow is awkward | Improve CLI output, error messages, defaults |
| **Missing HAL coverage** | A HAL function the firmware uses has no stub | Add to stub generation (allowed: peripheral boundary not crossed) |
| **Missing peripheral** | A peripheral the firmware uses is not simulated | Record for future roadmap; do NOT add during pilot |
| **Product gap** | The product cannot do something it promises | Fix the code or narrow the claim |
| **Architecture limitation** | The product's architecture fundamentally prevents something | Record for architecture review; do NOT work around |

The most common pilot feedback will likely be documentation gaps and UX friction. That's expected and healthy — it means the architecture is sound but the surface is rough.

---

## 9. Go/No-Go Criteria After Pilot

### Go — proceed to Stage 8 (scale to 3–5 pilots)

- Pilot achieved green CI in under 30 minutes
- Regression detection confirmed by pilot
- Pilot NPS ≥ 6/10
- Fewer than 3 architecture-level issues raised
- Zero kernel/scheduler changes required

### No-Go — revisit architecture or scope

- Pilot could not achieve green CI without hands-on help
- Multiple missing peripherals blocked the workflow
- Architecture limitation prevented a core use case
- Pilot NPS < 4/10
- Pilot reported that CI results were not trustworthy

### Conditional Go — fix then re-pilot

- Documentation gaps caused > 50% of friction → fix docs, try again
- One specific missing HAL function blocked progress → add it, try again
- OS-specific issue (Windows path separator, etc.) → fix, try again

---

## 10. What NOT to Do During the Pilot

- Do not add new peripherals
- Do not redesign the workflow
- Do not optimize CI speed
- Do not add features the pilot didn't ask for
- Do not argue with pilot feedback
- Do not preemptively fix things the pilot hasn't encountered yet
- Do not touch the kernel, scheduler, or trace engine

The pilot's job is to reveal what's actually broken, not what we think might be broken.

---

## Summary

S6.4 proved EmberSim works in CI. Stage 7 proves it works for someone else.

The success criterion:

> An external firmware engineer, using only the provided documentation, can integrate EmberSim into their existing STM32 project and detect a firmware regression in CI — without asking for help.
