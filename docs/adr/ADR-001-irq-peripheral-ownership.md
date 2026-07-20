# ADR-001: IRQ-to-Peripheral Ownership

**Status:** Proposed  
**Date:** 2026-07-19  
**Affects:** `ember_sim_kernel.c`, `ember_sim_kernel.h`, all peripheral implementations  

## Context

The kernel currently contains a hardcoded lookup table mapping peripheral base addresses to IRQ numbers:

```c
// ember_sim_kernel.c:261-267
static uint32_t periph_to_irq(uint32_t base) {
    switch (base) {
        case 0x40000400: return 28; // TIM2
        case 0x40004400: return 38; // USART2
        default:         return 0;
    }
}
```

This function is called by `nvic_bus_handler()` whenever a bus event arrives, to determine which IRQ to set pending. It violates the constitution:

> *"The EmberSim kernel must not depend on individual peripherals."* — §4 Architecture Invariants  
> *"Adding a peripheral must not require kernel modification."* — §4 Architecture Invariants

Every new peripheral (SPI, I2C, ADC, ...) adds another `case` to this switch. This is the precise failure mode the architecture was designed to prevent.

## Problem

The kernel does not know which IRQ belongs to which peripheral, yet the NVIC bus handler needs this mapping to convert bus events into IRQ pending states. The data exists (each `EmberPeripheral` has an `irq_number` field), but the kernel does not use it during bus dispatch.

## Options Considered

### Option A: Look up peripheral by base address at dispatch time

**Change:** Replace `periph_to_irq()` with a search over the registered peripheral list.

```c
static uint32_t periph_to_irq(uint32_t base) {
    for (int i = 0; i < peripheral_count; i++) {
        if (peripherals[i]->base_address == base) {
            return peripherals[i]->irq_number;
        }
    }
    return 0;
}
```

| Pro | Con |
|---|---|
| Zero kernel API changes | O(n) scan on every bus event |
| Uses existing `irq_number` field | Peripheral list must be populated before bus events arrive |
| Peripherals self-declare their IRQ | If two peripherals share a base address, first match wins |
| Minimal diff (~4 lines changed) | |

### Option B: Register IRQ handlers at peripheral init time

**Change:** Each peripheral's `init()` calls `nvic_register_handler(irq, handler)` directly. The kernel's `nvic_bus_handler` is replaced by per-peripheral bus subscriptions.

```c
// In TIM init:
nvic_register_handler(28, tim_irq_handler);
ember_bus_subscribe(10, tim_bus_handler);

// tim_bus_handler checks if the event is for TIM and if so sets NVIC pending
static void tim_bus_handler(const BusEvent *ev) {
    if (ev->source == TIM2_BASE) {
        nvic_set_pending(28);
    }
}
```

| Pro | Con |
|---|---|
| Peripheral owns its entire IRQ path | Every peripheral must duplicate bus→NVIC logic |
| No kernel IRQ mapping at all | Bus subscriber slots are limited (MAX_BUS_SUBSCRIBERS=8) |
| Aligns with plugin model | More boilerplate per peripheral |

### Option C: Kernel publishes all bus events; NVIC subscriber checks a registered IRQ map

**Change:** Keep a dedicated IRQ lookup table in the kernel, but populated by peripherals at registration time rather than hardcoded.

```c
// New kernel API
void kernel_register_irq(uint32_t base_address, uint32_t irq_number);

// Peripheral init calls this:
kernel_register_irq(0x40000400, 28);  // TIM2

// nvic_bus_handler uses the table:
static uint32_t periph_to_irq(uint32_t base) {
    return irq_map[base];  // hash or array lookup
}
```

| Pro | Con |
|---|---|
| O(1) lookup | Adds kernel API surface (`kernel_register_irq`) |
| Peripherals self-declare | Requires a data structure for the map (array or hash) |
| Clean separation: kernel owns the map, peripherals populate it | More code than Option A |
| Scales to many peripherals | Base addresses as keys may collide if not careful |

### Option D: Bus events carry IRQ number directly

**Change:** Peripherals include their IRQ number when publishing bus events. The `nvic_bus_handler` reads it from the event.

```c
// TIM tick publishes with IRQ embedded:
ember_bus_publish(BUS_EVT_TIMER_UPDATE, p->base_address, p->irq_number, NULL);

// nvic_bus_handler uses event.param:
static void nvic_bus_handler(const BusEvent *ev) {
    if (ev->param) nvic_set_pending(ev->param);
}
```

| Pro | Con |
|---|---|
| Simplest possible change | `param` field becomes overloaded (IRQ for some events, channel for others) |
| Zero kernel data structures | Loss of type safety — param means different things per event type |
| Self-contained in each peripheral | Makes bus events harder to understand and debug |
| No new kernel API | |

## Recommendation

**Option A: Look up peripheral by base address at dispatch time.**

Rationale:

1. **Minimal change.** The kernel diff is 4 lines. The function body changes from a switch to a loop. No new API, no new data structures, no changes to peripheral code.

2. **Uses existing infrastructure.** The `EmberPeripheral` struct already has `irq_number`. Peripherals already set it. The kernel already stores registered peripherals in an array. Option A simply connects these two existing facts.

3. **Correct for Slice 0/1 scale.** With MAX_PERIPHERALS=8, an O(n) scan costs at most 8 pointer dereferences. This is not a performance concern. If we ever have 100+ peripherals, we can revisit — but that's a good problem to have.

4. **No peripheral code changes.** TIM and UART already set `irq_number` in their `EmberPeripheral` struct. The fix is entirely in the kernel.

5. **Preserves determinism.** The search is deterministic (same peripheral list order every run). The same events produce the same IRQ mapping.

### Rejection of other options

- **Option B** is the most architecturally pure but adds boilerplate to every peripheral. Premature for 2-3 peripherals.
- **Option C** adds kernel API surface (`kernel_register_irq`) for a problem solved more simply by Option A.
- **Option D** overloads the `param` field and makes events harder to reason about.

### Migration path

1. Replace `periph_to_irq()` body with peripheral list scan
2. Verify TIM tests still pass deterministically
3. When UART is added in Slice 2, verify no kernel changes needed for its IRQ
4. If performance becomes a concern (50+ peripherals), replace with a hash table — but do not do this prematurely
