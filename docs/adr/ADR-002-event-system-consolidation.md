# ADR-002: Canonical Event System

**Status:** Proposed  
**Date:** 2026-07-19  
**Affects:** `ember_sim_kernel.h`, `ember_sim_kernel.c`, `ember_event_bus.h`, `ember_event_bus.c`, `templates.rs`, all peripheral implementations  

## Context

EmberSim currently has three overlapping event type systems, each with its own enum, struct, and dispatch mechanism:

| System | Enum | Struct | Location | Used By |
|---|---|---|---|---|
| Kernel events | `KernelEventType` (11 values) | `KernelEvent` | `ember_sim_kernel.h` | Kernel internal scheduler (`kernel_schedule_event`) |
| Bus events | `BusEventType` (6 values) | `BusEvent` | `ember_sim_kernel.h` | Peripherals → NVIC/trace dispatch (`ember_bus_publish`) |
| Ember events | `EmberEventType` (8 values) | `EmberEvent` | `ember_event_bus.h` | Standalone event bus (unclear if used) |

Additionally, `ember_event_bus.c` implements a separate publish/subscribe system (`ember_bus_subscribe` with type-filtered callbacks) that differs from the kernel's priority-based `ember_bus_subscribe`.

This violates the constitution:

> *"Important system behavior must be visible. The system should make it possible to understand what happened, when it happened, and why execution diverged."* — §4 Architecture Invariants

Three systems make it harder to understand event flow, not easier.

## Problem

When a developer adds a new peripheral, they must decide:
- Which event system to use?
- Which enum to extend?
- Which dispatch mechanism to hook into?

The current answer is "it depends" — and the answer is different for TIM vs. UART vs. what `ember_event_bus.h` was designed for. This is friction that will compound with every new peripheral.

Additionally, the trace output mixes event types from different systems, making trace format inconsistent and replay harder to implement.

## Options Considered

### Option A: Consolidate into a single `BusEvent` system

**Change:** Delete `KernelEventType` and `EmberEventType`. Extend `BusEventType` to cover all event categories. Kernel-internal scheduling uses `BusEvent` instead of `KernelEvent`. Delete `ember_event_bus.h/.c`.

```c
// Single event enum — covers hardware, software, and kernel-internal events
typedef enum {
    // Hardware events (peripheral → bus)
    BUS_EVT_TIMER_UPDATE,
    BUS_EVT_UART_TX_DONE,
    BUS_EVT_UART_RX_DONE,
    BUS_EVT_SPI_TX_DONE,
    BUS_EVT_SPI_RX_DONE,
    BUS_EVT_I2C_MASTER_TX_DONE,
    BUS_EVT_I2C_MASTER_RX_DONE,
    BUS_EVT_DMA_DONE,
    BUS_EVT_GPIO_EDGE,
    BUS_EVT_REGISTER_CHANGED,
    
    // Kernel-internal (scheduler)
    BUS_EVT_KERNEL_TIMER_FIRE,
    BUS_EVT_KERNEL_DMA_COMPLETE,
    
    BUS_EVT_COUNT
} BusEventType;

// Single event struct
typedef struct BusEvent {
    uint32_t     event_id;
    uint32_t     parent_id;
    uint32_t     timestamp_us;
    BusEventType type;
    uint32_t     source;       // peripheral base address
    uint32_t     param;        // channel, irq, etc.
    union {
        BusRegChangePayload reg;
    } data;
    struct BusEvent *next;     // queue linkage
} BusEvent;
```

| Pro | Con |
|---|---|
| One system to learn and extend | Touches every peripheral and the kernel |
| Consistent trace format | Risk of regression in working TIM code |
| Simpler debugging (one event stream) | Large diff (~200+ lines across 5+ files) |
| Enables unified replay (one event log) | Scheduler events now visible in bus — coupling risk |

### Option B: Keep two-tier: BusEvent for inter-peripheral, KernelEvent for scheduler

**Change:** Keep `KernelEvent` for internal scheduling. Merge `EmberEventType` into `BusEventType`. Delete `ember_event_bus.h/.c`. The kernel uses `KernelEvent` for its own scheduling; peripherals use `BusEvent` for all externally visible communication.

| Pro | Con |
|---|---|
| Smaller change than Option A | Still two systems to understand |
| Clear boundary: kernel-internal vs. peripheral-visible | Scheduler events not visible in bus trace |
| Preserves existing kernel scheduling logic | Replay engine must handle two event streams |
| Lower regression risk | |

### Option C: Keep all three, document the boundaries

**Change:** No code changes. Document when to use each system. Apply consistent naming.

| Pro | Con |
|---|---|
| Zero risk | Does not solve the problem |
| No diff | Confusion persists; each new peripheral re-litigates the decision |
| | Violates constitution observability requirement |
| | Replay remains difficult with three event streams |

### Option D: Delete `ember_event_bus.h/.c` only — defer full consolidation

**Change:** Delete the clearly redundant `ember_event_bus.h/.c` and its `EmberEventType`. Keep `KernelEventType` (scheduler) and `BusEventType` (peripheral communication) as the two-tier system. Remove `ember_event_bus` entries from `templates.rs` and `project.rs`. This is the minimal change that eliminates the most obvious duplication.

| Pro | Con |
|---|---|
| Smallest safe change (~10 lines) | Still two systems to consolidate later |
| Removes clearly dead code | Does not unify event types |
| No impact on working TIM/UART code | |
| Can be done in Slice 0/1 without destabilizing anything | |

## Recommendation

**Option B: Two-tier — KernelEvent for scheduler, BusEvent for peripherals. Delete `ember_event_bus.h/.c`.**

Rationale:

1. **Option A (full unification) is the right long-term answer but too risky for Slice 0/1.** The kernel scheduler uses `KernelEvent` for its internal priority queue. Merging this with `BusEvent` would touch the scheduler, event pool, and all peripheral `handle_event` implementations. TIM works today. A full unification risks breaking it without a clear customer-facing benefit yet.

2. **The two-tier split has a defensible boundary.** `KernelEvent` is for the kernel talking to itself (scheduling future work). `BusEvent` is for peripherals publishing observable state changes. These are different concerns with different consumers. The scheduler does not need trace visibility; peripherals do.

3. **Deleting `ember_event_bus.h/.c` is uncontroversial.** It is not referenced by any working peripheral. `templates.rs` does not embed it. `project.rs` does not write it. It appears to be a prototype that was never connected. Removing it eliminates one-third of the confusion with zero risk.

4. **This sets up a clean migration to Option A later.** When we have 3+ peripherals and understand the event patterns, we can decide whether full unification is warranted. By then we'll have evidence, not speculation.

### Concrete changes for Option B

1. Delete `crates/embersim-core/src/templates/ember_event_bus.h`
2. Delete `crates/embersim-core/src/templates/ember_event_bus.c`
3. Document the boundary: `KernelEvent` = kernel-internal scheduling; `BusEvent` = peripheral-observable state
4. Add a comment in `ember_sim_kernel.h` explaining when to use each
5. No changes to TIM, UART, or kernel scheduling logic

### Rejection of other options

- **Option A** is architecturally ideal but premature. We need 2-3 peripherals of evidence before unifying. The risk of breaking deterministic TIM behavior during Slice 0 is not acceptable.
- **Option C** violates the constitution. Three undocumented event systems is not observability.
- **Option D** is Option B without the documentation. We should document the boundary while we're here.

### Migration path

1. Delete `ember_event_bus.h/.c` (Slice 0)
2. Document the two-tier boundary (Slice 0)
3. Add UART using `BusEventType` (Slice 2)
4. After 3+ peripherals (Slice 4), evaluate whether Option A full unification is warranted based on real pain points
