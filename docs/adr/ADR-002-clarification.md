# ADR-002: Event System Ownership Clarification

**Status:** Clarified (pre-implementation)  
**Date:** 2026-07-19  

## Three Systems Identified

| System | Enum | Struct | Location | Embedded by templates.rs? | Written by project.rs? | Used by any peripheral? |
|---|---|---|---|---|---|---|
| Scheduler events | `KernelEventType` (11 values) | `KernelEvent` | `ember_sim_kernel.h/.c` | Yes | Yes | TIM (handle_event) |
| Bus events | `BusEventType` (6 values) | `BusEvent` | `ember_sim_kernel.h/.c` | Yes | Yes | TIM, UART (ember_bus_publish) |
| Ember events | `EmberEventType` (8 values) | `EmberEvent` | `ember_event_bus.h/.c` | No | No | None |

## Ownership

### KernelEvent — Owned by the kernel scheduler

**Producer:** `kernel_schedule_event()` — kernel-internal scheduling  
**Consumer:** Peripheral `handle_event()` callbacks  
**Trace visibility:** None (not traced; internal mechanism)  
**Purpose:** "Wake me up in N microseconds to do hardware work"  
**Example:** Timer overflow schedules `KERN_EVT_TIM_UPDATE` for immediate dispatch  
**Who extends it:** Kernel only. Peripherals never add types here.

### BusEvent — Owned by peripherals, dispatched by kernel

**Producer:** Peripherals via `ember_bus_publish()`  
**Consumer:** `nvic_bus_handler` (converts to IRQ pending), `trace_bus_handler` (converts to JSON Lines)  
**Trace visibility:** Full — every BusEvent becomes a trace line  
**Purpose:** "Something observable happened in hardware"  
**Example:** TIM publishes `BUS_EVT_TIMER_UPDATE` → NVIC sets IRQ28 pending → trace records it  
**Who extends it:** Peripherals add types here. No kernel changes needed — the bus dispatch is type-agnostic.

### EmberEvent (ember_event_bus.h/.c) — Dead code

**Status:** Not embedded, not written, not used. Appears to be an early prototype of the bus system that was superseded by the kernel's built-in `BusEvent`/`ember_bus_publish` mechanism. Has its own publish/subscribe with type-filtered callbacks (`ember_bus_subscribe(type, cb)`) that differs from the kernel's priority-based subscriber model.

## Decision

### Immediate (Slice 0/1): Delete `ember_event_bus.h/.c`

These files are dead code — not embedded, not written during `init`, not used by any peripheral. Deleting them is cleanup, not architecture change.

Files to delete:
- `crates/embersim-core/src/templates/ember_event_bus.h`
- `crates/embersim-core/src/templates/ember_event_bus.c`

No Rust code references them. No C code includes them (except the self-include in `ember_event_bus.c`). Zero risk.

### Deferred: Unification of KernelEvent + BusEvent

The remaining two systems (`KernelEvent` and `BusEvent`) serve different purposes and have different consumers. They should NOT be unified now. Revisit after 3+ peripherals have exercised both systems.

## Implementation

Delete two files. No other changes. Validated by: `cargo build`, `embersim init`, all 9 mocks compile, TIM tests pass deterministically.
