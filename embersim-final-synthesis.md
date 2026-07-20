# EmberSim — Strategic-Validated Plan: Final Synthesis

**Status:** Third iteration — strategic/commercial review incorporated  
**Architecture review:** `providing_architector.md`  
**Product/execution review:** `feedback.md`  
**Strategic review:** `revised.md`  
**Active plan:** `embersim-architector-revised-plan.md` (base document)  
**This file:** `embersim-final-synthesis.md` — the living synthesis of all review rounds  

---

## The Three Documents and What Each Contributed

| Document | Main Concern | Key Contribution |
|---|---|---|
| `embersim-industry-standard-plan.md` (my original) | Feature breadth, CI, security, testing | Established the target: industry-standard quality. Good CI matrix, testing pyramid, security points. Too horizontal. |
| `providing_architector.md` | Architecture | **"Stop adding peripherals. Prove the architecture."** Kernel freeze, peripheral contract, zero-diff gates, canonical layout, plugin model. The right architecture foundation. |
| `feedback.md` | Product & Execution | **"You're still building infrastructure."** Five lenses, four pillars, vertical slices, customer KPIs, competitive positioning against CI tools not simulators. The right execution model. |

---

## The Synthesis: What I Changed

### 1. Vision (Most Important Change)

| Before | After |
|---|---|
| "STM32 simulator generating HAL stubs" | **"The fastest way to validate embedded firmware before hardware exists."** |
| "Event-driven firmware execution engine with pluggable MCU peripheral models" | Architecture is an implementation detail. Customer benefit is the headline. |

### 2. Competitive Positioning

EmberSim competes against **testing/CI tools**, not hardware simulators:

| Wrong comparison set | Right comparison set |
|---|---|
| QEMU, Renode | GitHub Actions, GitLab CI, GoogleTest, Catch2 |
| "Simulating hardware" | "Testing firmware before hardware exists" |

### 3. Execution Model: Horizontal → Vertical Slices

The architectural review correctly identified that the kernel must be frozen and peripherals must be plugins. But the execution model was still horizontal (layers of infrastructure). The product review corrected this:

```
❌ Horizontal: Freeze kernel → SDK → Replay → Viewer → Scheduler → Peripherals → [value at the end]
✅ Vertical:   [Kernel+TIM → Real firmware → Release] → [+UART → Real firmware → Release] → ...
```

Each slice = architecture work + peripheral implementation + real firmware validation + customer KPI + release.

### 4. Technology: Freeze What's Proven, Not Everything

The architectural review said "freeze early." The product review refined: **freeze only what multiple peripherals have proven.**

| Freeze after TIM (1 peripheral) | Defer until TIM+UART+DMA (3 peripherals) |
|---|---|
| `EmberPeripheral` lifecycle (init, reset, tick, shutdown) | Exact `tick()` signature — UART may need different timing |
| `EmberRegMap` / `EmberRegister` API | `BusEvent` payload structure |
| NVIC pending/clear/dispatch | NVIC priority/preemption details |
| `EMBER_REGISTER_PERIPHERAL` macro | Trace event hierarchy |

**Principle:** If UART needs one additional runtime callback, evolve the API — don't hack UART. Freeze permanently after Slice 2 (TIM + UART both proven).

### 5. Four Pillars — Every Feature Evaluation

Every proposed feature must answer: **Which pillar does this strengthen?**

1. **Correctness** — Every register behaves like hardware.
2. **Determinism** — Every run produces identical results.
3. **Observability** — Every state transition is visible, queryable, replayable, explainable.
4. **Developer Productivity** — Reduces firmware dev time, debugging time, or CI cost.

If a proposed feature strengthens none of these, it doesn't make the 90-day plan.

### 6. The One-Question Filter (Before Every Feature)

> **Will this allow an embedded engineer to find a firmware bug faster, automate more validation, or eliminate dependence on physical hardware?**

If yes → on the roadmap. If no → infrastructure that should wait.

### 7. Customer KPIs — One Per Slice

| Slice | Technical Deliverable | Customer KPI |
|---|---|---|
| S0 — Stabilize | Tests pass, `embersim doctor` | Clone → init → build works |
| S1 — Kernel + TIM | Contracts, golden peripheral, typed events, NVIC | Real firmware TIM project produces IRQ→callback trace |
| S2 — UART | Pure plugin, canonical layout | "I can validate UART firmware without hardware" |
| S3 — DMA | Cross-peripheral communication | "UART+DMA firmware validates end-to-end" |
| S4 — SDK + Docs + CI | Plugin SDK, architecture docs, test framework | "I can model my custom ASIC in one afternoon" |
| S5 — Observability | DES scheduler, trace viewer, replay engine | "I can replay crashes" / "5000 tests in <2 min CI" |
| S6 — Scale | 5 more peripherals (SPI, I2C, ADC, GPIO, EXTI) | "I can validate multi-peripheral firmware" |

### 8. Quality Gates — Measurable, Not Aspirational

Every slice exit requires measurable gates:

| Gate | Threshold |
|---|---|
| Kernel isolation | No kernel file `#include`s any peripheral header |
| Kernel diff | **0 lines** for each new peripheral |
| Test coverage | ≥90% line coverage per peripheral |
| Memory | No leaks across init→run→shutdown cycle |
| Determinism | Same state + same inputs = bit-for-bit identical trace |
| Documentation | 1-page doc per peripheral (registers, events, HAL bridge, tests) |
| Structure | Directory mirrors `peripherals/tim/` canonical layout |

### 9. Product Description: Feature → Outcome Language

| Infrastructure language (avoid) | Customer language (use) |
|---|---|
| "We implemented typed events" | "You can now replay crashes" |
| "Plugin SDK complete" | "You can model your custom ASIC in one afternoon" |
| "Scheduler V2 ships" | "CI now runs 5000 firmware tests in under two minutes" |
| "Zero kernel diff gate" | "Add a peripheral without touching the kernel" |

### 10. What's Deliberately Excluded

These are not "no" — they are "not until the architecture and product-market fit are proven":

- Multi-MCU families (SVD parsing) — post-90-day
- RTOS simulation — dropped entirely
- USB, Ethernet, CAN — complex peripherals that don't validate architecture
- VS Code extension — post-beta
- Package managers — post-beta
- Pro-tier licensing — post-beta
- Scenario engine / fault injection — post-kernel-validation (Slate 6+)

---

## Self-Critique: What the Strategic Review Exposed

After reading `revised.md`, I need to be honest about where my thinking was still wrong:

### Where I Was Still Engineering-Led

1. **I put observability at Slice 5.** A simulator without observability is a slow debugger. Trace is the moat — the thing that makes EmberSim different from running firmware on hardware. It should be built in from Slice 1, not bolted on at week 8.

2. **I put SDK before DMA pain.** I was thinking "build the SDK so others can help." But you cannot design a good SDK until you've suffered through 3 implementations (TIM, UART, DMA) and know where the pain points are. The SDK should come AFTER, not before.

3. **I missed Compatibility as a pillar.** I had four pillars: Correctness, Determinism, Observability, Productivity. But an embedded engineer's first question is "will my existing firmware run unchanged?" That's the adoption gate — and I didn't call it out as a pillar.

4. **I had no customer validation milestone.** I was building toward a hypothetical "public beta" at week 12. The right answer: find 10 embedded engineers at week 4 and ask "would this save you time?" — not "do you like it?"

5. **My commercial thinking was 6.5/10.** I never identified who pays, what their pain is, or how to test willingness-to-pay before building everything. The plan was technically excellent but market-naive.

### The Company Thesis — Corrected

| Before | After |
|---|---|
| "STM32 simulator" | **"Firmware CI platform that executes embedded firmware without hardware, detects regressions earlier, and provides deterministic replay of failures."** |
| "Event-driven firmware execution engine" | Implementation detail. The product is: **move firmware validation from hardware labs into CI.** |

The competitor is not QEMU, not Renode, not GitHub Actions. The competitor is **the absence of a fast feedback loop** — the weeks lost waiting for hardware boards, flashing firmware, and manually debugging.

### What Changed — Three Immediate Corrections

**Change 1: Five Pillars (added Compatibility)**

| Pillar | Definition |
|---|---|
| **Correctness** | Every register behaves like hardware |
| **Determinism** | Every run produces identical results |
| **Observability** | Every state transition is visible, queryable, replayable |
| **Productivity** | Reduces firmware dev time, debugging time, or CI cost |
| **Compatibility** | **Existing firmware runs unchanged — STM32 HAL, bare-metal, custom drivers** |

Compatibility is the moat. If a customer must rewrite their firmware to use EmberSim, they won't adopt it. The acceptance criterion: "EmberSim runs unmodified firmware source builds produced for target hardware."

**Change 2: Trace from Slice 1 (not Slice 5)**

Lightweight structured trace — `event_id | timestamp | source | payload` — built into the kernel from day one. Not the full viewer. Just the data. Because:

- A simulator without trace is a slow debugger
- Trace enables the replay engine (the killer feature)
- Every customer demo shows: "Here is your firmware. Here is every interrupt it generated. Here is exactly where it crashed."

**Change 3: Customer Validation at Week 4**

Before building SDKs, before more peripherals, before the viewer:

1. Find 10 embedded engineers
2. Show them: STM32 firmware + EmberSim running + interrupt trace
3. Ask: **"Would this save you time?"** — not "Do you like it?"
4. If yes → continue. If no → pivot before investing in SDK.

### What "Real Firmware" Means — Three Levels

| Level | Definition | Example | For Customers? |
|---|---|---|---|
| **Level 1** | Mock firmware written for the simulator | `timer_init(); assert(callback_called);` | No — internal testing only |
| **Level 2** | HAL firmware using STM32 HAL API | `HAL_TIM_Base_Start_IT(&htim);` | Minimum for demos |
| **Level 3** | Production firmware — same startup code, interrupts, drivers, application logic as the target build | Unmodified `.c`/`.h` files from a CubeMX project, compiled for host | **This is the goal** |

The acceptance criterion for every slice: **Level 3 firmware runs and produces a trace.** A customer should be able to take their CubeMX project, point `embersim` at it, and get a trace without modifying their source code.

### Target Customer

Not hobbyists. Not individual developers.

| Tier | Who | Pain | Willingness to Pay |
|---|---|---|---|
| **Primary** | Embedded startup (10-50 engineers), firmware must start before silicon | Hardware delays = missed funding milestones | High — survival depends on it |
| **Secondary** | Semiconductor companies | Need reference firmware validated across MCU variants | High — selling chips depends on it |
| **Tertiary** | Automotive/industrial with CI compliance requirements | Must prove firmware quality in audit | Very high — regulatory requirement |

**First customer profile:** Embedded startup building hardware where firmware development must begin before silicon availability. That pain is existential, not cosmetic.

### Revised Slice Order

| Week | Slice | Goal | Customer KPI | Revenue Check |
|---|---|---|---|---|
| 1 | **S0** — Stabilize | Clean slate: tests, compile, doctor | Clone → init → build works | — |
| 1-3 | **S1** — Kernel + TIM + Trace | Frozen contracts, golden peripheral, lightweight trace from day one | **Level 3 firmware TIM project produces IRQ→callback trace** | — |
| 4-6 | **S2** — UART | Pure plugin, canonical layout, trace enhanced | **Level 3 firmware UART driver runs without hardware** | **← Week 4: show 10 engineers, ask "would this save you time?"** |
| 7-9 | **S3** — DMA + Trace Viewer | Cross-peripheral, live trace viewer, replay prototype | "I can debug UART+DMA race conditions in trace" | **← Week 8: first paid pilot customer** |
| 10-12 | **S4** — SDK + Docs | Plugin SDK, architecture docs, test framework — **AFTER suffering through 3 peripherals** | "I can model my custom ASIC in one afternoon" | **← Week 12: second paid pilot** |

### Why SDK Comes Last

The original plan had SDK at weeks 6-8. The strategic review correctly identified this as premature. You cannot design a good plugin API until you've implemented TIM (tick-driven), UART (state machine + FIFO), and DMA (cross-peripheral + ownership). Each stresses different parts of the interface. The SDK emerges from the pain, not from the design.

### Scoring Myself (Honest Assessment)

| Area | My Score | Reviewer's Score | Gap |
|---|---|---|---|
| Strategic direction | 7/10 | 9/10 | I was still thinking "simulator" not "CI platform." Corrected. |
| Architecture | 9/10 | 8.5/10 | Strong. Slightly over-engineered in places. |
| Execution model | 8/10 | 9/10 | Vertical slices were right. Order was wrong. Corrected. |
| Product thinking | 7/10 | 8/10 | Good pillars. Missing Compatibility. Corrected. |
| Commercial readiness | 4/10 | 6.5/10 | I had no customer, no pricing, no validation. Added. |
| Risk management | 7/10 | 8/10 | Good gates. Missing market risk mitigations. Added. |
| **Overall** | **7.0/10** | **8.3/10** | **Still technically-led. Correcting now.** |

### The Billion-Dollar Question

> "What is the smallest EmberSim version that an embedded company would pay for within 30 days?"

Answer: **Level 3 firmware with UART running on EmberSim, producing a structured trace that shows exactly where and when interrupts fired.** That's the minimum viable product — not a feature list, a customer outcome.

If we can show an embedded startup: "Your firmware. No hardware. Every interrupt visible. Deterministic replay of that crash you spent 3 days debugging" — they will pay. Not because the architecture is elegant. Because it solves a problem that currently costs them weeks.

---

## Immediate Next Action: Slice 0

Per `EMBERSIM_PROJECT_STATE.md`, the immediate work is:

1. Verify full C compilation chain (gcc on PATH needed for `embersim init`)
2. Run existing test files (`test_uart_runtime.c`, `test_i2c.c`, `test_spi.c`, `test_tim_runtime.c`)
3. Add Rust tests for untested modules
4. Fix critical TD items (ISR handle lookup, state constants, path separators, duplicate event bus)
5. Add `embersim doctor` command
6. Add `--version --verbose`

**Exit:** Clone → `embersim init` → `embersim build` → all tests pass → trace output.
