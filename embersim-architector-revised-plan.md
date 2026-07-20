# EmberSim — Product-Validated Architecture & Execution Plan

**Status:** Revised after architectural review + product/execution challenge  
**Base plan:** `embersim-industry-standard-plan.md`  
**Architecture challenge:** `providing_architector.md`  
**Product/execution challenge:** `feedback.md`  
**Revised:** 2026-07-19  

---

## The Five-Lens Framework

This plan is structured to satisfy five independent evaluation lenses simultaneously:

```
                Vision
                  │
      Product ────┼──── Technology
                  │
          Engineering
                  │
            Execution
```

Most engineering plans optimize only Technology. This one optimizes all five.

---

## Product Vision (Revised)

| Before | After |
|---|---|
| "STM32 simulator generating HAL stubs" | **"The fastest way to validate embedded firmware before hardware exists."** |
| "Event-driven firmware execution engine..." | Architecture is an implementation detail. The customer benefit is the headline. |

### Competitive Positioning

EmberSim competes against **testing and CI tools**, not hardware simulators:

| Wrong comparison set | Right comparison set |
|---|---|
| QEMU, Renode | GitHub Actions, GitLab CI, Jenkins |
| Other simulators | GoogleTest, Catch2, CTest |
| "Simulating hardware" | "Testing firmware" |

This changes the market, the messaging, and the feature priorities.

### Four Pillars — Every Feature Must Strengthen At Least One

1. **Correctness** — Every register behaves like hardware.
2. **Determinism** — Every run produces identical results.
3. **Observability** — Every state transition is visible, queryable, replayable, and explainable.
4. **Developer Productivity** — Every feature must reduce firmware development time, debugging time, or CI cost.

### The One-Question Architectural Filter

> **Will this allow an embedded engineer to find a firmware bug faster, automate more validation, or eliminate dependence on physical hardware?**

If yes → on the roadmap. If no → infrastructure that should wait, or a feature whose value hasn't been articulated yet.

---

## Execution Model: Vertical Slices, Not Horizontal Layers

The architectural review correctly identified that the kernel must be frozen and peripherals must be plugins. But the execution model needs adjustment: **vertical slices that each produce customer value**, not horizontal infrastructure phases.

```
❌ Horizontal (builds infrastructure, delivers no value until the end):
  Freeze kernel → SDK → Replay → Viewer → Scheduler → Peripherals → [value]

✅ Vertical (each slice delivers a testable customer outcome):
  Slice 1: Kernel + TIM → Run a CubeMX project with a timer → Release
  Slice 2: + UART → Validate UART firmware → Release
  Slice 3: + DMA → Cross-peripheral scenarios → Release
  ...
```

Each vertical slice is: **architecture work → peripheral implementation → real firmware validation → customer KPI → release.**

---

## What Changed and Why

The original 30-day plan and my first industry-standard plan were organized around **feature breadth**: add GPIO, then UART, then I2C, then SPI, then TIM, then expand to more MCU families. This is the natural instinct — and the one that kills most embedded simulation projects.

The architectural review identified the core problem:

> Most simulators become 250k lines of tightly coupled peripheral code. The architecture collapses under its own weight. The right approach is: freeze the kernel, define stable interfaces, then prove any peripheral fits without kernel changes.

The revised plan inverts the priority stack:

| | Old Priority | New Priority |
|---|---|---|
| **Primary goal** | Support more peripherals and MCUs | Prove the architecture is extensible |
| **Success metric** | "18 peripherals supported" | "Any peripheral can be added as an independent plugin with zero kernel changes" |
| **Peripheral order** | GPIO → UART → I2C → SPI → TIM → ... | TIM (golden) → UART (state machine) → DMA (cross-peripheral) → SPI → I2C → ADC → GPIO → EXTI |
| **Kernel treatment** | Add features as needed | Freeze contracts first. No kernel changes for new peripherals = architectural gate. |
| **SVD/MCU expansion** | Phase B (weeks 2-6) | After kernel proven (week 12+) |
| **Plugin SDK** | Milestone 5 (weeks 16-20) | Weeks 5-6 (before peripheral expansion) |
| **Product positioning** | "STM32 simulator" | "Event-driven firmware execution engine with pluggable MCU peripheral models" |

### What I Retained From My Original Plan

These elements from `embersim-industry-standard-plan.md` remain correct and are incorporated:

1. **Structured diagnostics system** (Section 2.3) — still essential, moved into kernel freeze phase
2. **CI/CD matrix across 3 OS × 2 compilers** (Section 4.2) — retained for Milestone 3
3. **Performance benchmarks in CI** (Section 4.4) — retained, with added architectural regression tests
4. **Security audit points** (Section 4.5) — retained for pre-release
5. **Documentation suite structure** (Section 4.6) — retained, reordered to prioritize architecture docs
6. **Semantic versioning and release engineering** (Section 4.7) — retained, deferred to post-beta
7. **Technical debt catalog** (Section 1.2) — retained as pre-requisite work
8. **Testing pyramid targets** (Section 4.1) — retained, with new architectural invariant tests added

### What I Dropped or Deferred

These elements were premature given where the architecture needs to be proven first:

1. **Multi-MCU family expansion (SVD parsing)** — moves from weeks 2-6 to post-week-12. Freeze the kernel before making it multi-architecture.
2. **RTOS simulation** — dropped entirely from the 90-day scope. Architectural review explicitly flags this as a second-order feature.
3. **VS Code extension** — moves to post-beta. The plugin SDK and documentation matter more for now.
4. **Package manager distribution** — moves to post-beta. GitHub releases suffice until the architecture is proven.
5. **Scenario engine and fault injection** — deferred to post-kernel-validation. These depend on the typed event system being solid.
6. **Pro-tier licensing system** — deferred. Focus on proving the free tier's architecture creates more value than gating features.
7. **"Weeks of peripheral expansion" as a primary phase** — replaced with architecture-gated expansion where each new peripheral must pass the "zero kernel diff" test.

---

## Technology: What to Freeze and When

The architectural review said "freeze the kernel early." The product review refined this: **freeze only what multiple peripherals have proven.**

| Freeze after 1 peripheral (TIM) | Defer until 2+ peripherals prove it |
|---|---|
| `EmberPeripheral` lifecycle (init, reset, tick, shutdown) | Exact `tick()` signature — UART may need different timing |
| `EmberRegMap` / `EmberRegister` API | `BusEvent` payload structure — DMA may need fields TIM doesn't |
| NVIC pending/clear/dispatch | NVIC priority/preemption — may need per-MCU customization |
| `EMBER_REGISTER_PERIPHERAL` macro | Trace event hierarchy — will evolve with more peripherals |

**Principle:** If UART needs one additional runtime callback, evolve the API rather than hacking UART forever. Mature projects freeze after 2-3 implementations, not 1.

---

## Revised 90-Day Plan: Vertical Slices with Customer KPIs

Each slice produces a **customer-visible outcome**, not just infrastructure.

---

### Slice 0 — Stabilize: Make Everything Work (Week 1)

**Customer KPI:** A developer can clone the repo and run `embersim init && embersim build` on a fixture project without errors.

| # | Task | Description |
|---|---|---|
| P0.1 | Verify C compilation chain | Every `.c` mock compiles with gcc: mock_hal, trace_log, mock_state, mock_uart, mock_i2c, mock_spi, mock_tim, ember_sim_kernel, ember_regs |
| P0.2 | Run existing test files | `test_uart_runtime.c`, `test_i2c.c`, `test_spi.c`, `test_tim_runtime.c` — all must pass |
| P0.3 | Rust test coverage | Add tests for `clean.rs`, `preprocess.rs`, `model.rs`, `project.rs`, `stubgen.rs`, `templates.rs`, `discovery.rs` |
| P0.4 | Fix critical technical debt | TD1 (ISR handle lookup), TD3 (state constants), TD4 (path separators), TD14 (duplicate event bus) |
| P0.5 | `embersim doctor` | Environment check: gcc/clang/cmake on PATH, version checks, fix suggestions |
| P0.6 | `--version --verbose` | Full dependency version dump for bug reports |

**Quality gates:**

| Gate | Requirement |
|---|---|
| Tests | ≥60% Rust line coverage, all C tests pass |
| Memory | No leaks in kernel init → shutdown cycle (valgrind/drmemory) |
| Performance | `embersim init` on minimal_hal.h completes in <2 seconds |
| Regression | All previously passing tests still pass |

---

### Slice 1 — Kernel + TIM: Prove the Core Loop (Weeks 1-3)

**Customer KPI:** Run a real CubeMX firmware project that uses a timer and see the timer overflow → IRQ → callback chain in trace output.

---

### Phase 1 — Freeze the Kernel (Weeks 1-2)

**Goal:** Define stable interfaces. Make the kernel impossible to break.

This is the most important phase. No new peripherals. No new features. Only contract definition.

#### 1.1 Define the Peripheral Contract (C level)

Every peripheral must implement a uniform lifecycle. Currently `ember_sim_kernel.h` defines `EmberPeripheral` with init/reset/tick/shutdown. This needs to become the **only** way a peripheral interacts with the kernel.

```c
// ember_sim_kernel.h — frozen interface
typedef struct EmberPeripheral {
    const char *name;                    // Unique identifier
    uint32_t    base_address;            // Dummy base address on x86

    // Lifecycle — called by kernel, never by user code
    void (*init)    (struct EmberPeripheral *self);
    void (*reset)   (struct EmberPeripheral *self);
    void (*tick)    (struct EmberPeripheral *self, uint64_t time_ns);
    void (*shutdown)(struct EmberPeripheral *self);

    // Interrupt interface
    uint32_t (*irq_pending)(struct EmberPeripheral *self);  // Returns IRQ bitmap
    void     (*irq_ack)    (struct EmberPeripheral *self, uint32_t irq_n);

    // Trace — called by peripheral, consumed by kernel
    void (*trace_event)(struct EmberPeripheral *self, uint32_t event_type, const char *payload);

    // Private data — owned by the peripheral implementation
    void *priv;
} EmberPeripheral;
```

**Key decisions:**
- `EmberPeripheral` is the **only** struct the kernel knows about
- No peripheral-specific fields in the kernel
- No `#include "uart.h"` in kernel code — ever
- Peripherals self-register via `kernel_register_peripheral(EmberPeripheral *p)`

#### 1.2 Define the Runtime Contract

```c
// Runtime — frozen interface
typedef struct EmberRuntime {
    void (*register_peripheral)(EmberPeripheral *p);
    void (*unregister_peripheral)(const char *name);
    void (*publish_bus_event)(BusEvent *evt);
    void (*schedule_callback)(uint64_t at_time_ns, void (*cb)(void *), void *ctx);
    uint64_t (*now_ns)(void);
    void (*log_trace)(uint32_t category, const char *format, ...);
} EmberRuntime;
```

#### 1.3 Define the Bus Event Contract

Move from free-form JSON strings to typed events:

```c
// Bus event types — frozen enum
typedef enum {
    BUS_EVT_REGISTER_WRITE,      // Peripheral wrote a register
    BUS_EVT_REGISTER_READ,       // Peripheral read a register
    BUS_EVT_INTERRUPT_RAISED,    // Peripheral raised an IRQ
    BUS_EVT_INTERRUPT_CLEARED,   // IRQ acknowledged
    BUS_EVT_PERIPHERAL_STARTED,  // Peripheral began operation
    BUS_EVT_PERIPHERAL_STOPPED,  // Peripheral completed/stopped
    BUS_EVT_CALLBACK_INVOKED,    // HAL callback fired
    BUS_EVT_DMA_REQUEST,         // DMA transfer requested
    BUS_EVT_DMA_COMPLETE,        // DMA transfer finished
    BUS_EVT_EXTERNAL_STIMULUS,   // External input (button, sensor)
} BusEventType;

typedef struct BusEvent {
    uint64_t    timestamp_ns;
    BusEventType type;
    const char *source_peripheral;  // "TIM2", "USART1"
    const char *detail_json;        // Structured payload — JSON for now, schema later
} BusEvent;
```

The trace logger converts `BusEvent` → JSON Lines. This is the bridge between the C runtime and the Rust analysis tools.

#### 1.4 Define the NVIC Contract

```c
// NVIC — frozen interface
typedef struct NvicController {
    void (*enable_irq)  (uint32_t irq_n);
    void (*disable_irq) (uint32_t irq_n);
    void (*set_pending) (uint32_t irq_n);
    void (*clear_pending)(uint32_t irq_n);
    uint32_t (*get_pending)(void);
    uint32_t (*find_highest_priority)(void);  // Returns IRQn or -1
    void (*dispatch)    (uint32_t irq_n);      // Calls registered handler

    // Priority — Cortex-M style, 0 = highest
    void (*set_priority)(uint32_t irq_n, uint32_t priority);
    uint32_t (*get_priority)(uint32_t irq_n);
} NvicController;
```

#### 1.5 Architectural Gates for Phase 1

Before advancing to Phase 2, these must all be true:

| Gate | Requirement |
|---|---|
| **G1** | `EmberPeripheral` is the only peripheral-kernel interface |
| **G2** | No kernel file `#include`s any peripheral header |
| **G3** | All bus events use the `BusEvent` typed enum — no untyped strings |
| **G4** | NVIC dispatch path is identical for every peripheral |
| **G5** | Each frozen interface has a documented contract (header comment block) |
| **G6** | TIM implementation is ported to the frozen interfaces and passes all tests |

**Exit criteria:**
- `ember_sim_kernel.h` frozen — no further changes without architectural review
- `ember_regs.h` frozen
- All 6 contract interfaces documented with lifecycle diagrams
- TIM migrated to frozen interfaces and passing `test_tim_runtime.c`

---

### Phase 2 — TIM as Golden Peripheral (Week 2)

**Goal:** Make TIM the reference implementation every future peripheral copies.

TIM is the right starting point because it validates: tick-driven updates, register writes → hardware change, overflow → interrupt → NVIC → HAL handler → user callback. That's the full pipeline.

#### 2.1 Restructure TIM to the Canonical Layout

```
peripherals/tim/
├── tim_peripheral.h         # Public API: ember_tim_init(), ember_tim_set_period()
├── tim_peripheral.c         # EmberPeripheral implementation (init/reset/tick/shutdown/irq/trace)
├── tim_registers.c          # Register map: CR1, SR, CNT, PSC, ARR, DIER, CCR1-4
├── tim_registers.h
├── tim_irq.c                # IRQ handling: TIM_IRQHandler → dispatch to callbacks
├── tim_irq.h
├── tim_trace.c              # Trace events specific to TIM
├── tim_trace.h
├── tim_hal_bridge.c         # HAL_TIM_* → EmberPeripheral bridge
├── tim_hal_bridge.h
└── tim_tests.c              # Unit + integration tests
```

Every future peripheral mirrors this structure. SPI author opens `peripherals/tim/`, copies the layout, fills in the blanks.

#### 2.2 TIM Architecture Diagram (Must Be Documented)

```
Tick (1 μs)
  │
  ▼
tim_peripheral.tick()
  ├── CNT++ (register write via ember_reg_write)
  ├── CNT == ARR? → Overflow
  │     ├── Set UIF in SR
  │     ├── Publish BUS_EVT_REGISTER_WRITE (SR.UIF)
  │     └── Publish BUS_EVT_INTERRUPT_RAISED (TIM2_IRQn)
  │
  ▼
NVIC.set_pending(TIM2_IRQn)
  │
  ▼
NVIC.dispatch(TIM2_IRQn)
  │
  ▼
HAL_TIM_IRQHandler(&htim2)
  │
  ▼
HAL_TIM_PeriodElapsedCallback(&htim2)
  │
  ▼
User callback
```

**Exit criteria:**
- TIM directory matches canonical layout
- All TIM tests pass with frozen kernel
- Architecture diagram committed alongside code
- Kernel diff from Phase 1: **0 lines**

---

### Phase 3 — Typed Event Bus Formalization (Week 2-3)

**Goal:** Every bus event is typed, filterable, and replayable.

#### 3.1 Event Type Hierarchy

```
BusEvent (base)
├── HardwareEvent
│   ├── RegisterWriteEvent   { peripheral, register, old_value, new_value }
│   ├── RegisterReadEvent    { peripheral, register, value }
│   └── PeripheralStateEvent { peripheral, old_state, new_state }
├── InterruptEvent
│   ├── IrqRaisedEvent       { peripheral, irq_n, priority }
│   ├── IrqClearedEvent      { peripheral, irq_n }
│   └── IrqDispatchEvent     { peripheral, irq_n, handler_name }
├── SoftwareEvent
│   ├── CallbackInvokedEvent { peripheral, callback_name }
│   ├── DmaRequestEvent      { source, destination, size }
│   └── DmaCompleteEvent     { source, destination }
└── ExternalEvent
    ├── GpioEdgeEvent         { port, pin, edge }
    └── StimulusEvent         { peripheral, data }
```

#### 3.2 Trace Exporter

The trace logger receives typed events and converts to JSON Lines. This is a **separate concern** — the kernel doesn't know about JSON.

```c
// trace_log.c — consumes BusEvent*, writes JSON
void trace_event_to_jsonl(FILE *out, const BusEvent *evt);
```

#### 3.3 Enable Replay

Every `BusEvent` contains enough information to reconstruct the simulation state. A replay engine reads the trace and feeds events back into the kernel — this becomes a powerful debugging tool (impossible on real hardware).

**Exit criteria:**
- All existing peripherals use typed BusEvent subtypes
- Trace output unchanged (JSON Lines format preserved)
- Kernel diff: **0 lines**

---

### Phase 4 — NVIC Completion (Week 3)

**Goal:** Behavior-accurate interrupt controller.

The current NVIC likely handles raise/dispatch. Complete it to Cortex-M behavior:

```c
// NVIC additions
typedef struct NvicController {
    // ... existing interface ...

    // Priority — 0 = highest, 255 = lowest (Cortex-M style)
    void (*set_priority)(uint32_t irq_n, uint32_t priority);
    uint32_t (*get_priority)(uint32_t irq_n);

    // Preemption — higher-priority IRQ preempts lower
    bool (*can_preempt)(uint32_t new_irq_n);

    // Tail chaining — back-to-back IRQs without state restore
    bool (*can_tail_chain)(uint32_t next_irq_n);

    // Nested interrupt tracking
    uint32_t (*nesting_depth)(void);

    // Vector table — maps IRQn → handler function pointer
    void (*register_handler)(uint32_t irq_n, void (*handler)(void));
} NvicController;
```

**Not cycle-accurate. Behavior-accurate.** The goal is correct preemption and priority behavior, not cycle counts.

**Exit criteria:**
- Priority-based preemption works (high-priority IRQ preempts low-priority)
- Tail chaining works (back-to-back IRQs skip state save/restore)
- Nested interrupts up to depth 4 tested
- NVIC diff from Phase 1: only additions to the struct — no breaking changes

---

### Phase 5 — UART Migration (Weeks 3-4)

**Goal:** Prove the architecture with a fundamentally different peripheral.

UART is the architectural validation peripheral because it stresses: **state machines, FIFOs, blocking vs. non-blocking, interrupt modes, DMA, errors, callbacks** — all behaviors TIM does not exercise.

#### 5.1 Architecture Gate

```
Kernel diff after UART: 0 lines
Runtime diff after UART: 0 lines
Scheduler diff after UART: 0 lines
Bus diff after UART: 0 lines
NVIC diff after UART: 0 lines

Only these files change:
  peripherals/uart/*
```

If any kernel file must change to support UART, **stop and fix the kernel interface first.**

#### 5.2 UART Directory Structure (Mirrors TIM)

```
peripherals/uart/
├── uart_peripheral.h         # Public API
├── uart_peripheral.c         # EmberPeripheral implementation
├── uart_registers.c          # SR, DR, CR1, BRR register map
├── uart_registers.h
├── uart_fifo.c               # TX/RX circular buffers
├── uart_fifo.h
├── uart_irq.c                # USART1_IRQHandler → TxCplt/RxCplt/Error
├── uart_irq.h
├── uart_trace.c              # UART-specific trace events
├── uart_trace.h
├── uart_hal_bridge.c         # HAL_UART_* → EmberPeripheral bridge
├── uart_hal_bridge.h
└── uart_tests.c
```

#### 5.3 UART Behavior Checklist

| Mode | Description | Architecture Validates |
|---|---|---|
| Blocking TX | Copy to TX buffer, return HAL_OK | Basic register write |
| Blocking RX | Read from RX buffer, return HAL_TIMEOUT if empty | FIFO read |
| IT TX | Enable TXEIE, tick advances one byte/tick, TxCpltCallback on empty | State machine, IRQ timing |
| IT RX | Enable RXNEIE, callback when buffer fills | State machine, callback chain |
| DMA TX | DMA request → DMA engine → memory → IRQ → callback | Cross-peripheral (Phase 6) |
| DMA RX | Inverse of DMA TX | Cross-peripheral (Phase 6) |
| Error injection | Set PE/NE/FE/ORE, verify ErrorCallback fires | Fault handling |
| Overrun | RX buffer full, new byte arrives, ORE set | Edge case handling |

**Exit criteria:**
- All 8 behavior modes tested
- Kernel diff: **0 lines**
- `test_uart_runtime.c` passes on frozen kernel
- UART directory mirrors TIM directory exactly in structure

---

### Phase 6 — DMA (Weeks 4-5)

**Goal:** Prove cross-peripheral communication. DMA is architecturally more important than SPI or I2C.

#### 6.1 Why DMA Matters

DMA validates: **shared peripherals, ownership, multiple interrupts, memory transactions, bus interactions.** These are all cross-cutting concerns that expose architectural weaknesses.

#### 6.2 DMA Flow

```
UART TX Empty (TXE)
  │
  ▼
DMA Request (USART1_TX)
  │
  ▼
DMA Engine
  ├── Read from memory (source address)
  ├── Write to UART.DR (destination address)
  ├── Increment source pointer
  ├── Decrement transfer count
  └── Count == 0?
        │
        ▼
      DMA_IT_TC (Transfer Complete IRQ)
        │
        ▼
      NVIC.dispatch(DMA1_Stream6_IRQn)
        │
        ▼
      HAL_DMA_IRQHandler(&hdma_usart1_tx)
        │
        ▼
      HAL_UART_TxCpltCallback(&huart1)
```

#### 6.3 DMA Architecture Gate

```
Kernel diff after DMA: 0 lines
```

DMA must be implemented as an `EmberPeripheral` — same interface as TIM and UART. If DMA requires kernel changes, the architecture is wrong.

**Exit criteria:**
- UART + DMA TX works end-to-end
- UART + DMA RX works end-to-end
- DMA engine handles: peripheral→memory, memory→peripheral, memory→memory
- Multiple DMA streams can be active simultaneously
- Kernel diff: **0 lines**

---

### Phase 7 — Plugin SDK (Weeks 5-6)

**Goal:** Someone outside the project can write a new peripheral in ~100 lines with no kernel knowledge.

#### 7.1 SDK Surface

```c
// ember_sdk.h — the only header a plugin author needs
#include "ember_sim_kernel.h"   // EmberPeripheral, EmberRuntime
#include "ember_regs.h"         // EmberRegister, EmberRegMap
#include "ember_event_bus.h"    // BusEvent, BusEventType
#include "ember_nvic.h"         // NvicController
#include "ember_trace.h"        // Trace helpers
#include "ember_hal_bridge.h"   // HAL → peripheral glue macros
```

#### 7.2 Plugin Registration Macro

```c
// Self-registering — no main() changes needed
EMBER_REGISTER_PERIPHERAL(my_sensor, &my_sensor_ops, 0x40005000);
```

Expands to a constructor function that calls `kernel_register_peripheral()` before `main()`.

#### 7.3 Hello Peripheral (100-line example)

```c
// my_temperature_sensor.c — complete fake peripheral in ~100 lines
#include "ember_sdk.h"

static EmberRegMap regs;
static uint16_t temperature = 0x1A3B;  // 25.0°C in 12-bit

static void sensor_tick(EmberPeripheral *self, uint64_t time_ns) {
    // Every 100ms, update temperature register
    static uint64_t last_update = 0;
    if (time_ns - last_update > 100000000) {
        ember_reg_write(&regs, 0x00, temperature);
        last_update = time_ns;
    }
}

static void sensor_init(EmberPeripheral *self) {
    ember_reg_init(&regs, "TEMP_SENSOR", 4);
    ember_reg_define(&regs, 0x00, "TEMP_OUT", 0x0000);  // Temperature register
    ember_reg_define(&regs, 0x02, "CONFIG",   0x0000);  // Configuration
}

static EmberPeripheral temp_sensor = {
    .name = "TEMP_SENSOR",
    .base_address = 0x40005000,
    .init = sensor_init,
    .tick = sensor_tick,
    .reset = NULL,
    .shutdown = NULL,
    .irq_pending = NULL,  // No interrupts
    .irq_ack = NULL,
    .trace_event = NULL,
};

EMBER_REGISTER_PERIPHERAL(temp_sensor, &temp_sensor);
```

**Exit criteria:**
- Hello Peripheral example compiles, registers, and produces trace events
- SDK documentation complete: lifecycle, registration, events, HAL bridging
- External developer can write a peripheral without reading kernel source

---

### Phase 8 — Architecture Documentation Freeze (Week 6)

**Goal:** Contributors can add peripherals without asking the core team.

#### 8.1 Required Architecture Documents

```
docs/architecture/
├── 01-runtime-lifecycle.md       # Kernel init → peripheral registration → tick loop → shutdown
│                                 # Diagram: initialization sequence
│                                 # Diagram: main loop (kernel_step → tick_all → nvic_dispatch → repeat)
├── 02-peripheral-lifecycle.md    # init → reset → tick → irq → trace → shutdown
│                                 # Diagram: peripheral state machine
│                                 # Template: peripheral directory structure
├── 03-event-flow.md              # Hardware event → bus → NVIC → IRQ → HAL → callback
│                                 # Diagram: full event trace with timestamps
├── 04-interrupt-flow.md          # Pending → priority → preemption → dispatch → tail-chain
│                                 # Diagram: NVIC decision tree
├── 05-register-model.md          # EmberRegister, EmberRegMap, read/write/set_bits
│                                 # Diagram: register change → bus event → trace
├── 06-bus-system.md              # Publish/subscribe, event types, filtering
│                                 # Diagram: bus topology
├── 07-plugin-sdk.md              # Getting started, Hello Peripheral, HAL bridging
│                                 # Template: empty peripheral skeleton
├── 08-adding-a-peripheral.md     # Step-by-step: copy TIM → rename → implement ops → test
│                                 # Checklist: 10 steps to a working peripheral
└── 09-trace-format.md            # JSON Lines schema, event type reference
```

**Exit criteria:**
- All 9 documents written with diagrams
- "Adding a peripheral" guide tested by implementing a fake peripheral in <2 hours
- Documentation lives in repo, rendered as mdBook or similar

---

### Phase 9 — Scheduler Evolution: Discrete-Event Simulation (Weeks 6-7)

**Goal:** Replace tick-loop with priority-queue-based discrete-event simulation.

#### 9.1 Why

Current approach: `kernel_step()` advances 1 μs and ticks every peripheral. A 1-second simulation = 1,000,000 kernel steps.

Discrete-event approach: jump directly to the next event. A 1-second simulation with 10 events = 10 kernel steps.

#### 9.2 Architecture

```c
// Scheduler v2
typedef struct EmberScheduler {
    // Priority queue of timed events — sorted by fire_time_ns
    void (*schedule)(uint64_t fire_time_ns, void (*callback)(void *), void *ctx);
    void (*cancel)(void *event_handle);

    // Advance clock to next event and execute it
    void (*step)(void);       // Process one event
    void (*run_until)(uint64_t deadline_ns);  // Process events until deadline

    // Query
    uint64_t (*now_ns)(void);
    uint64_t (*next_event_ns)(void);  // Time of next scheduled event
} EmberScheduler;
```

#### 9.3 Migration Path

Peripherals currently call `kernel_step()` in a loop. After migration:

```c
// Before: tick loop
for (int i = 0; i < 1000000; i++) {
    kernel_step();  // 1 μs per step
}

// After: discrete-event
kernel_run_until(1000000);  // Jump to next event each iteration
```

Peripheral `tick()` functions become optional — they only need to schedule future events, not be called every microsecond.

**Exit criteria:**
- All existing peripherals (TIM, UART, DMA) work with discrete-event scheduler
- 1000× speedup on idle periods (e.g., 1ms between timer overflows)
- Deterministic: same initial state + same events = same trace

---

### Phase 10 — Trace Visualization (Weeks 7-9)

**Goal:** A timeline view that makes firmware execution understandable at a glance.

#### 10.1 Trace Engine 2.0

Current trace: flat JSON Lines. Future: hierarchical trace with parent/child relationships.

```
Clock → Peripheral → Register → Interrupt → HAL → Callback → User code
```

Trace output becomes:

```
0.000000000  TIM2        CNT++          (0→1)
0.000001000  TIM2        CNT++          (1→2)
...
0.000100000  TIM2        CNT == ARR     Overflow
0.000100000  TIM2        SR.UIF         Set
0.000100000  NVIC        Pending        TIM2_IRQn
0.000100000  NVIC        Dispatch       TIM2_IRQn
0.000100000  HAL         IRQHandler     HAL_TIM_IRQHandler
0.000100001  HAL         Callback       HAL_TIM_PeriodElapsedCallback
0.000100001  User        Callback       my_timer_callback()
```

#### 10.2 Web-Based Trace Viewer

- `embersim serve` — starts local HTTP + WebSocket server
- Trace events stream in real-time during host execution
- Timeline: horizontal lanes per peripheral (TIM, UART, DMA, etc.)
- Click event → expand to see: register values, call stack, timing
- Filter by peripheral, event type, time range
- Export filtered trace as JSON

#### 10.3 SVG Static Export

- `embersim report --format svg` — generates standalone SVG timeline
- Embeddable in CI job summaries, PR comments, documentation

**Exit criteria:**
- `embersim serve` shows live trace for TIM + UART + DMA scenario
- User can click an event and see register state at that moment
- SVG export works and renders in GitHub PR comments

---

### Phase 11 — Replay Engine (Week 9)

**Goal:** Record and replay firmware execution. Impossible on real hardware — a key differentiator.

#### 11.1 Architecture

```
Execution → trace.jsonl (record)
trace.jsonl → ReplayEngine → kernel (replay)
```

The replay engine reads the trace and feeds events back into the kernel at the recorded timestamps. The kernel doesn't know it's being replayed.

#### 11.2 Debugging Use Cases

1. **"Show me what happened 50 μs before the crash."** — Jump to timestamp, replay forward
2. **"Run this exact sequence 1000 times with slight variations."** — Deterministic replay + parameter variation
3. **"Compare two runs side by side."** — Diff two traces, highlight divergence points
4. **"Share this bug reproduction."** — Send a trace file instead of hardware setup instructions

**Exit criteria:**
- Record a TIM + UART + DMA scenario to trace
- Replay produces identical trace (bit-for-bit)
- `embersim replay --input trace.jsonl --breakpoint 0.000500000` stops at exactly 500 μs

---

### Phase 12 — Scale: More Peripherals (Weeks 9-11)

**Only now** — after the kernel is frozen, the plugin SDK is documented, and the architecture is proven with TIM + UART + DMA — add more peripherals.

#### 12.1 Peripheral Addition Order

| Priority | Peripheral | Why Now | Architecture Validates |
|---|---|---|---|
| 1 | **SPI** | Mirrors UART pattern (TX/RX, IT, DMA) | Multi-byte transactions, chip select |
| 2 | **I2C** | Adds addressing, START/STOP conditions | Multi-byte protocol, ACK/NACK |
| 3 | **ADC** | Adds analog modeling, continuous conversion | Regular sampling, injected channels |
| 4 | **GPIO** | External stimulus entry point | Edge detection, EXTI bridge |
| 5 | **EXTI** | GPIO → NVIC bridge | External event → IRQ path |

#### 12.2 Architectural Gate for Each Peripheral

Before each peripheral is considered "done":

| Checkpoint | Requirement |
|---|---|
| **Kernel stability** | No kernel code changes required — **kernel diff = 0 lines** |
| **Plugin model** | Peripheral self-registers via `EMBER_REGISTER_PERIPHERAL` |
| **Event model** | Uses typed `BusEvent` subtypes — no ad-hoc strings |
| **Interrupt model** | IRQs flow through NVIC without peripheral-specific code paths |
| **Trace quality** | Every externally visible action appears in structured trace |
| **Test quality** | Unit tests for registers + integration tests for HAL path |
| **Structure** | Directory mirrors `peripherals/tim/` canonical layout |
| **Documentation** | 1-page peripheral doc: registers, events, HAL bridge, test scenarios |

**If any checkpoint fails, fix the architecture before proceeding.**

**Exit criteria:**
- 5 additional peripherals (SPI, I2C, ADC, GPIO, EXTI) all pass architectural gates
- Any peripheral can be added by following the "Adding a Peripheral" guide
- Total peripheral count: 8 (TIM + UART + DMA + SPI + I2C + ADC + GPIO + EXTI)

---

### Phase 13 — Public Beta & Feedback (Weeks 11-12)

**Goal:** External users validate the architecture by adding real peripherals.

#### 13.1 Beta Deliverables

1. **Stable kernel API** — No breaking changes since Phase 1
2. **Plugin SDK** — Documented, with Hello Peripheral example
3. **8 validated peripherals** — TIM, UART, DMA, SPI, I2C, ADC, GPIO, EXTI
4. **CI-ready test framework** — Deterministic execution with reusable assertions
5. **Trace visualization** — Timeline viewer for debugging
6. **Replay engine** — Record/replay for bug reproduction
7. **Full architecture documentation** — 9 docs with diagrams

#### 13.2 Beta Success Metrics

- 1 external contributor adds a peripheral without core team help
- 1 real CubeMX firmware project runs on EmberSim and finds a bug before hardware
- Zero kernel API breaks reported by beta users

---

## New 90-Day Timeline Summary

| Week | Phase | What |
|---|---|---|
| 0-1 | **P0** | Stabilize: tests, compile chain, `embersim doctor` |
| 1-2 | **P1** | Freeze kernel: `EmberPeripheral`, `EmberRuntime`, `BusEvent`, `NvicController` |
| 2 | **P2** | TIM as golden peripheral — canonical directory layout |
| 2-3 | **P3** | Typed event bus — `BusEventType` enum, structured subtypes |
| 3 | **P4** | NVIC completion — priority, preemption, tail-chaining |
| 3-4 | **P5** | UART migration — zero kernel diff gate |
| 4-5 | **P6** | DMA — cross-peripheral communication, zero kernel diff gate |
| 5-6 | **P7** | Plugin SDK — Hello Peripheral in 100 lines |
| 6 | **P8** | Architecture documentation freeze — 9 docs |
| 6-7 | **P9** | Scheduler v2 — discrete-event simulation |
| 7-9 | **P10** | Trace visualization — live WebSocket + SVG export |
| 9 | **P11** | Replay engine — record/replay/diff |
| 9-11 | **P12** | Scale: SPI, I2C, ADC, GPIO, EXTI — each gated by checkpoints |
| 11-12 | **P13** | Public beta |

---

## What This Plan Deliberately Excludes (for now)

These are explicitly deferred past the 90-day window. They are not "no" — they are "not yet."

| Deferred | Reason |
|---|---|
| Multi-MCU families (F7, H7, G0, L4) | Freeze one architecture before generalizing |
| CMSIS-SVD parsing | Depends on frozen kernel + proven peripheral model |
| RTOS simulation (FreeRTOS) | Second-order feature; prove bare-metal first |
| USB, Ethernet, SDMMC, CAN | Complex peripherals that don't validate architecture |
| VS Code extension | Plugin SDK + docs matter more for adoption |
| Package managers (brew, choco) | GitHub releases suffice for beta |
| Pro-tier licensing | Prove free tier value first |
| Scenario engine | Depends on typed event bus + replay being solid |
| Fault injection | Depends on scenario engine |
| HTML report generator | Superseded by live trace viewer |
| Code coverage integration | Important, but post-beta |

---

## Product Positioning Change

| | Before | After |
|---|---|---|
| **One-line description** | "STM32 simulator that generates HAL stubs" | "Event-driven firmware execution engine with pluggable MCU peripheral models" |
| **Primary value prop** | "Compile and test firmware on your PC" | "Any STM32 peripheral can be implemented as an independent runtime plugin" |
| **Customer buys** | "Supports 8 peripherals" | "I can run 5,000 firmware tests in CI without hardware" |
| **Architecture claim** | "Weak-symbol HAL stubs" | "Zero kernel changes to add a peripheral" |

---

## Current State → Phase 0 Immediate Actions

Based on the codebase audit, here is the exact sequence for the next session:

1. **Run full C compilation chain** (user's workflow — already shown working but needs gcc on PATH for `init`)
2. **Run existing test files** — `test_uart_runtime.c`, `test_i2c.c`, `test_spi.c`, `test_tim_runtime.c`
3. **Add Rust tests** for untested core modules
4. **Fix TD1, TD3, TD4, TD14** — the critical technical debt items
5. **Consolidate `ember_event_bus`** — appears to have drifted from kernel bus (TD14)
6. **Add `embersim doctor`** — environment validation command
7. **Add `--version --verbose`** — for bug reports

After Phase 0 is complete, the real architectural work begins with Phase 1: freezing the kernel contracts.

---

*This plan replaces `embersim-industry-standard-plan.md` as the active roadmap. The original plan is retained for reference — its CI matrix, testing pyramid, security, and documentation sections remain valid and should be incorporated when those phases are reached.*
