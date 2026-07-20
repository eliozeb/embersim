# ADR Validation Report

**Date:** 2026-07-19  
**Slice:** 0 → 1 transition  

## ADR-001: IRQ-to-Peripheral Ownership

**Decision:** Replace kernel hardcoded IRQ lookup with peripheral list scan (Option A).

**Implementation:**
```c
// BEFORE: kernel knew about specific peripherals
static uint32_t periph_to_irq(uint32_t base) {
    switch (base) {
        case 0x40000400: return 28; // TIM2
        case 0x40004400: return 38; // USART2
        default:         return 0;
    }
}

// AFTER: kernel discovers IRQ from registered peripherals
static uint32_t periph_to_irq(uint32_t base) {
    for (int i = 0; i < peripheral_count; i++) {
        if (peripherals[i]->base_address == base && peripherals[i]->irq_number) {
            return peripherals[i]->irq_number;
        }
    }
    return 0;
}
```

**Evidence:**
- TIM runtime: 6/6 PASS
- Kernel diff: 6 lines, no API surface change
- Peripherals changed: 0 — each already declares `irq_number`

## ADR-002: Event System Ownership

**Decision:** Delete dead `ember_event_bus.h/.c`. Keep two-tier: `KernelEvent` (scheduler) + `BusEvent` (peripheral-to-bus). Document ownership boundaries.

**Implementation:**
- Deleted: `crates/embersim-core/src/templates/ember_event_bus.h`
- Deleted: `crates/embersim-core/src/templates/ember_event_bus.c`
- Documented: `docs/adr/ADR-002-clarification.md`
- Rust references: 0 (files were not in `templates.rs` or `project.rs`)
- C references: 0 (files were never `#include`d by any peripheral)

**Evidence:**
- TIM runtime: 6/6 PASS (unchanged)
- `cargo build --workspace`: no errors
- `embersim init`: workspace created, no missing files
- C compilation: 9/9 mocks compile

## Determinism Proof

| Metric | Before ADR | After ADR |
|---|---|---|
| Golden trace lines | 181 | 181 |
| Golden trace SHA256 | `E113449E9408B3838BD90B3DA47F81FA86881A523B874E1137BFF0AE8B48FF4B` | `E113449E9408B3838BD90B3DA47F81FA86881A523B874E1137BFF0AE8B48FF4B` |
| Deterministic across runs | ✅ Identical | ✅ Identical |
| TIM tests | 6/6 PASS | 6/6 PASS |
| Cargo test | 5/5 PASS | 5/5 PASS |

## Result

**Architecture changed without observable regression.** The kernel no longer depends on individual peripherals for IRQ mapping. Dead event infrastructure removed. Deterministic behavior preserved bit-for-bit.

## Why This Matters

Six months from now, when a contributor adds a CAN peripheral and the kernel doesn't need a `case 0x40006400: return ...` — this report is the proof that the architecture was intentionally changed to support that growth. The ADR documents why. The hash proves nothing broke.
