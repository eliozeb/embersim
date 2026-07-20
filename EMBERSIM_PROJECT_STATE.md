# EmberSim Project State Snapshot
**Last Session:** Day 6 Integration Complete  
**Date:** 2026-07-17  
**Next:** Day 7 (I2C Virtual Device) or Day 8 (CMake Build Pipeline)

---

## What Exists Now

### Rust Workspace (`C:\embersim\embersim`)
```
embersim/
├── Cargo.toml
├── crates/
│   ├── embersim-core/
│   │   ├── Cargo.toml          (deps: serde, serde_json, regex, anyhow)
│   │   └── src/
│   │       ├── lib.rs           (pub mod: clean, model, parser, pipeline, preprocess, stubgen, templates, project)
│   │       ├── model.rs         (HalFunction, Parameter, Project structs)
│   │       ├── preprocess.rs    (gcc/clang -E execution)
│   │       ├── clean.rs         (strip preprocessor noise)
│   │       ├── parser.rs        (regex extraction + structured params)
│   │       ├── pipeline.rs      (orchestration: preprocess→clean→parse→dedup)
│   │       ├── stubgen.rs       (generates mock_hal.h + mock_hal.c with weak stubs)
│   │       ├── templates.rs     (include_bytes! for embedded C templates)
│   │       └── project.rs       (Init command: writes full ember_sim/ workspace)
│   └── embersim-cli/
│       ├── Cargo.toml
│       └── src/
│           └── main.rs          (Commands: Generate, Init)
├── test-fixtures/
│   └── minimal_hal.h            (16 HAL functions: GPIO + UART)
└── test-embersim/               (generated workspace from `embersim init`)
    ├── mocks/
    │   ├── mock_hal.h           (auto-generated types + forward declarations)
    │   ├── mock_hal.c           (weak stubs with trace_log calls)
    │   ├── trace_log.h          (trace_log_init/close/log declarations)
    │   ├── trace_log.c          (append-only JSON Lines writer)
    │   ├── mock_state.c         (stateful GPIO: WritePin/ReadPin/TogglePin)
    │   └── mock_uart.c          (professional UART: blocking, IT, DMA, errors)
    └── host_main.c              (minimal host runner, calls app_init/app_run)
```

### C Templates (`crates/embersim-core/templates/`)
- `trace_log.h` / `trace_log.c`
- `mock_state.c`
- `mock_uart.c`
- `host_main.c`

### Verified Working
- [x] Day 2: Parser extracts 16 HAL functions from preprocessed headers
- [x] Day 3: Stub generator produces compilable mock_hal.h + mock_hal.c
- [x] Day 4: Stateful GPIO mocks with port-based lookup (weak override)
- [x] Day 5: Trace logger writes JSON Lines with timestamps
- [x] Day 6: Professional UART mock (blocking, IT, DMA, error injection, callbacks)
- [x] Day 6 Integration: `embersim init` generates full workspace from templates

### Architecture Decisions Locked In
1. **Weak symbol pattern**: Auto-generated stubs are `__attribute__((weak))`. Hand-written templates in `mock_state.c` / `mock_uart.c` provide strong overrides.
2. **Trace format**: JSON Lines (`trace.jsonl`), flat JSON objects, one per HAL call.
3. **Instance mapping**: Dummy base addresses on x86 (GPIOA=0x10000000, USART1=0x40011000).
4. **Tick-driven async**: `ember_sim_uart_tick()` must be called from host runner for IT/DMA completion.
5. **Template embedding**: C files embedded in Rust binary via `include_bytes!` in `templates.rs`.

---

## How to Resume in a New Session

Paste the following into your first message:

> I am continuing EmberSim, a host-native firmware test harness. Current state: Day 6 integration is complete. The Rust CLI can `init` a workspace with embedded C templates. Parser, stub generator, trace logger, GPIO mocks, and professional UART mock (blocking/IT/DMA/error injection) are all working. I need to continue to [Day 7 / Day 8 / specific feature]. My repo is at `C:\embersim\embersim`.

### Quick Verification Commands
```powershell
cd C:\embersim\embersim
cargo test --workspace
cargo run -p embersim-cli -- init --hal test-fixtures/minimal_hal.h -o test-embersim
cd test-embersim
gcc -I mocks -c mocks/mock_hal.c -o mocks/mock_hal.o
gcc -I mocks -c mocks/trace_log.c -o mocks/trace_log.o
gcc -I mocks -c mocks/mock_state.c -o mocks/mock_state.o
gcc -I mocks -c mocks/mock_uart.c -o mocks/mock_uart.o
```

---

## Active Roadmap

> **Living synthesis: `embersim-final-synthesis.md`** — self-critique, corrected pillars, reordered slices, commercial plan.  
> **Full plan: `embersim-architector-revised-plan.md`** — architecturally-validated 90-day plan.  
> **Original plan: `embersim-industry-standard-plan.md`** — retained for reference (CI matrix, testing pyramid, security).  
> **Review 1 (architecture): `providing_architector.md`** — freeze kernel, prove extensibility.  
> **Review 2 (product/execution): `feedback.md`** — five lenses, four pillars, vertical slices, customer KPIs.  
> **Review 3 (strategic/commercial): `revised.md`** — compatibility pillar, trace from day one, customer validation at week 4, company thesis.

### Immediate: Phase 0 — Stabilize & Verify (Week 1)

The project is at an inflection point: the simulation kernel works (TIM runtime passes), but it needs to be frozen and proven before expanding.

1. **P0.1** — Verify full C compilation chain (all mocks + kernel + regs compile and link)
2. **P0.2** — Run existing test files: `test_uart_runtime.c`, `test_i2c.c`, `test_spi.c`, `test_tim_runtime.c`
3. **P0.3** — Add Rust tests for untested modules (clean, preprocess, model, project, discovery, stubgen, templates)
4. **P0.4** — Fix critical TD: ISR handle lookup (TD1), state constants (TD3), path separators (TD4), duplicate event bus (TD14)
5. **P0.5** — Add `embersim doctor` command
6. **P0.6** — Add `--version --verbose`

### After Phase 0: Phase 1 — Freeze the Kernel (Week 1-2)

Define stable interfaces: `EmberPeripheral`, `EmberRuntime`, `BusEvent`, `NvicController`. No new features. No new peripherals. Make the kernel impossible to break before anything else is added.

### Product Positioning (Revised)

| Before | After |
|---|---|
| "STM32 simulator generating HAL stubs" | "Event-driven firmware execution engine with pluggable MCU peripheral models" |
| "Supports 8 peripherals" | "Any peripheral as an independent plugin — zero kernel changes" |

### Where We Are vs. the 30-Day Plan
- **Day 6** (verified): Parser, stub gen, trace log, GPIO mock, UART mock, kernel + TIM runtime
- **Day 7–8** (code written, unverified): I2C mock, SPI mock, TIM mock, CMake pipeline, discovery
- **Day 9+** (not started): Everything beyond — now reorganized per the revised plan

---

## Critical Files to Preserve
Do NOT delete or modify without discussion:
1. `crates/embersim-core/src/parser.rs` — regex is finely tuned
2. `crates/embersim-core/templates/mock_uart.c` — professional mock with state machine
3. `crates/embersim-core/src/templates.rs` — paths must match template directory
4. `test-fixtures/minimal_hal.h` — used for CI/regression

---

## Known Issues / Technical Debt
1. `mock_uart.c` tick function stores handle references incompletely (commented section for ISR handle lookup)
2. `stubgen.rs` type definitions are hardcoded; needs dynamic generation from CMSIS headers
3. No `HAL_UART_STATE_*` constants in generated `mock_hal.h` for non-UART peripherals
4. Windows-only path separators in some template paths
5. `project.rs` does not yet validate `embersim.json`


I think you've reached an important inflection point. If I were reviewing EmberSim as a Principal Embedded Software Architect or CTO before approving funding, I would **change the next 90-day plan significantly** from the previous version.

The biggest change is this:

> **You should stop thinking about "adding peripherals" and start thinking about "proving the architecture."**

Those are very different goals.

---

# Current Assessment

From everything you've shown over the last several weeks:

* ✓ Timer runtime works
* ✓ Register model works
* ✓ Event bus works
* ✓ NVIC dispatch works
* ✓ Runtime scheduler works
* ✓ Plugin registration works
* ✓ Trace generation works
* ✓ HAL compatibility works
* ✓ Tests pass

This is no longer a prototype.

This is the **first version of an embedded simulation kernel.**

That changes the roadmap.

---

# What most embedded simulators get wrong

Most simulators become this:

```
GPIO
ADC
SPI
UART
CAN
USB
DMA
RTC
DAC
...
```

Eventually:

```
250k lines
Everything coupled
Impossible to maintain
```

I would reject this architecture.

---

# Instead build the kernel first

Think of Linux.

Linux was not valuable because it had drivers.

Linux became valuable because the architecture made drivers easy.

EmberSim should follow the same philosophy.

Your selling point should become:

> "Any STM32 peripheral can be implemented as an independent runtime plugin."

That is far more valuable than:

> "Supports UART."

---

# New 90 Day Objective

Not

> Support UART

Instead

> Prove EmberSim is a scalable simulation platform.

Huge difference.

---

# Phase 1 (Weeks 1–3)

## Freeze the Kernel

No new features.

Instead define stable interfaces.

Example

```
Peripheral

↓

Tick()

↓

Publish Hardware Event

↓

Event Bus

↓

NVIC

↓

IRQ

↓

HAL

↓

User callback
```

That becomes permanent.

Every future peripheral must obey it.

---

## Define Peripheral API

Instead of every peripheral inventing itself

Create

```
PeripheralOps

register()

unregister()

reset()

tick()

read()

write()

irq()

shutdown()
```

Then UART

TIM

ADC

SPI

DMA

all implement

```
PeripheralOps
```

Now everything is uniform.

---

# Phase 2 (Weeks 3–5)

Build UART

NOT because UART matters.

Because UART stresses completely different behaviour.

Timer stresses

time

UART stresses

state machine

FIFO

interrupt latency

blocking

non-blocking

DMA

errors

If UART fits cleanly...

...your architecture is validated.

---

# Phase 3 (Weeks 5–6)

Introduce DMA

Not because customers need DMA first.

Because DMA validates:

shared peripherals

ownership

multiple interrupts

memory transactions

bus interactions

DMA exposes architectural weaknesses faster than almost anything else.

---

# Phase 4 (Weeks 6–7)

Documentation

Most engineers skip this.

Don't.

Document:

```
How a peripheral works

Lifecycle

Registration

Interrupt flow

Tick flow

Event flow

```

Why?

Because contributors can now add peripherals without asking you.

That's enormous.

---

# Phase 5 (Weeks 7–8)

Plugin SDK

This is the biggest improvement I'd make.

Instead of:

```
Peripheral code
```

Build

```
Peripheral SDK
```

Like

```
EmberPeripheral

EmberRegister

EmberRuntime

EmberInterrupt

EmberEvent
```

Someone should be able to create

```
FakeSensor

```

in

100 lines.

That's powerful.

---

# Phase 6 (Weeks 8–10)

Trace Viewer

Right now

You have

```
JSON
JSON
JSON
JSON
JSON
```

Developers won't read this.

Instead

```
Tick

↓

Peripheral

↓

IRQ

↓

HAL

↓

Callback

↓

Application
```

Like

```
TIME

0

TIM2 CNT++

1

TIM2 Overflow

2

UIF Set

3

IRQ Pending

4

IRQ Enter

5

HAL_TIM_IRQHandler

6

PeriodElapsedCallback

```

This becomes one of EmberSim's strongest selling points.

---

# Phase 7 (Weeks 10–12)

Developer Experience

Now build things people actually pay for.

Examples

Auto peripheral registration

```
EMBER_REGISTER(TIM2)
```

instead of

```
registerPeripheral(...)
```

Automatic trace names

Assertion helpers

```
EXPECT_REGISTER(TIM2,SR)
EXPECT_IRQ(TIM2)
EXPECT_EVENT(...)
```

Test fixtures

Mock builders

Virtual clocks

This reduces friction enormously.

---

# Things I would deliberately NOT build

Many projects die because they chase completeness.

Avoid these for now:

❌ USB

❌ Ethernet

❌ SDMMC

❌ File systems

❌ Graphics

❌ FreeRTOS simulation

❌ CMSIS RTOS

❌ Network stacks

These are all second-order features.

---

# What I WOULD build

In order

```
Kernel

↓

Plugin API

↓

UART

↓

DMA

↓

SPI

↓

I2C

↓

ADC

↓

GPIO

↓

EXTI
```

Notice

GPIO comes later.

Because GPIO is trivial.

DMA is architecturally important.

---

# What Customers Actually Buy

This is where many engineers misjudge the market.

People don't buy:

> "Supports 18 peripherals."

They buy:

> "I can run 5,000 firmware tests in CI without hardware."

or

> "I can debug interrupt timing visually."

or

> "My junior engineer can reproduce a race condition."

or

> "Every register write is traceable."

Those are business outcomes.

---

# A Better Product Vision

I would stop describing EmberSim as:

> STM32 simulator

Instead describe it as:

> **An event-driven firmware execution engine with pluggable MCU peripheral models.**

That's a fundamentally stronger position. It suggests the architecture can eventually support other MCU families, even if STM32 remains the initial focus.

---

# My Optimized 90-Day Deliverables

If I were leading this project, the success criteria at Day 90 would be:

1. **Stable kernel API** — No breaking changes to the core runtime.
2. **Peripheral SDK** — A documented, reusable interface for adding new peripherals.
3. **TIM and UART fully migrated** — Demonstrating that two fundamentally different peripherals work through the same architecture.
4. **DMA integrated** — Validating shared-resource and interrupt orchestration.
5. **Architecture documentation** — Clear diagrams of event flow, interrupt dispatch, lifecycle, and extension points.
6. **Trace visualization** — A timeline view that makes firmware execution understandable at a glance.
7. **CI-ready test framework** — Deterministic execution with reusable assertions and fixtures.
8. **Plugin example** — A complete "Hello Peripheral" that proves third parties can extend EmberSim without touching the kernel.

---

## Overall Assessment

Given the progress you've demonstrated, I would **not** spend the next three months racing to support dozens of peripherals. That would increase code size without increasing strategic value.

Instead, I would invest in making EmberSim **the platform** for peripheral simulation. A small number of well-designed peripherals built on a rock-solid SDK is far more compelling than a large collection of tightly coupled implementations.

If you execute this plan well, EmberSim will no longer look like "another STM32 simulator." It will look like the beginnings of a reusable embedded simulation framework—something that can support multiple microcontroller families over time while remaining maintainable and attractive to contributors and commercial users.
