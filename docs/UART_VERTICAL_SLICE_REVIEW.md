# UART Vertical Slice Review

**Date:** 2026-07-19  
**Slice:** 1A — Architecture Validation  

## 1. Did UART require kernel changes?

**No.** Zero lines changed in `ember_sim_kernel.c` or `ember_sim_kernel.h` to support UART.

UART uses the same kernel interfaces as TIM:
- `kernel_register_peripheral()` — self-registration
- `ember_bus_publish()` — bus event publication
- `ember_reg_read/write/set_bits/clear_bits()` — register model
- `nvic_register_handler()` — IRQ handler registration
- `trace_log()` — trace output

The ADR-001 `periph_to_irq()` peripheral scan discovers UART's IRQ (38) from `uart_peripheral.irq_number` — the same mechanism TIM uses. No switch case added.

## 2. Did UART require scheduler changes?

**No.** Zero lines changed in the scheduler. `kernel_advance_ticks()` iterates all registered peripherals and calls their `tick()` — UART's tick works identically to TIM's.

## 3. Did TIM golden trace remain identical?

**Yes.** SHA256: `E113449E9408B3838BD90B3DA47F81FA86881A523B874E1137BFF0AE8B48FF4B` — unchanged from Slice 0 baseline.

## 4. What files changed to make UART work?

### Added (all pre-existing, validated now)
| File | Role |
|---|---|
| `mock_uart.c` | UART peripheral — EmberPeripheral implementation |
| `test_uart_runtime.c` | UART runtime tests |

### Modified
| File | Change | Reason |
|---|---|---|
| `mock_uart.c:51` | `SR.writable_mask: 0x0000 → 0xFFFF` | Peripheral bug: mock couldn't write hardware status flags |
| `mock_uart.c:174` | `rx_pos++` on RXNE | Peripheral bug: DR read wasn't consuming received byte |
| `test-fixtures/minimal_hal.h` | Added `HAL_UART_Init` declaration | Pipeline needs to parse it for mock_hal.h generation |
| `test_uart_runtime.c` | Fixed `kernel_run_until` deadlines (monotonic) | Test bug: absolute-time API used with relative-time values |

### NOT modified
```
ember_sim_kernel.c      — 0 lines changed for UART
ember_sim_kernel.h      — 0 lines changed
mock_tim.c              — 0 lines changed
ember_regs.c            — 0 lines changed
ember_regs.h            — 0 lines changed
trace_log.c             — 0 lines changed
```

## 5. Architecture evidence

### Before this slice
```
Kernel ─── periph_to_irq() ─── switch(TIM2, USART2)  ← hardcoded
```

### After this slice
```
Kernel ─── periph_to_irq() ─── peripheral list scan  ← discovers any peripheral
  │                                │
  │                           TIM2: irq_number=28
  │                           USART2: irq_number=38
  │                           (future): irq_number=...
```

Adding a third peripheral (SPI, I2C, ADC) requires: set `irq_number` in its `EmberPeripheral` struct, implement tick/init, publish bus events. Zero kernel changes.

## 6. What customer capability is now proven?

**"Two fundamentally different peripherals — timer-driven (TIM) and state-machine-driven (UART) — execute on the same kernel without kernel modification."**

This is the architecture validation milestone. TIM validates tick-driven peripherals (timers, counters). UART validates state-machine peripherals (serial, FIFO, interrupt-driven). Together they prove the kernel interface is general enough to support both categories.

## 7. UART bugs found (Case A — peripheral model issues)

| Bug | Symptom | Root Cause | Fix |
|---|---|---|---|
| SR not writable | IT callbacks never fired | `SR.writable_mask = 0x0000` blocked mock hardware writes | Set to `0xFFFF` |
| RXNE over-fire | Callback fired 10 times for 1 byte | `rx_pos` not advanced on DR read | Increment `rx_pos` in IRQ handler |

Both are peripheral model correctness issues — not architecture failures. The kernel, bus, NVIC, and trace systems worked correctly on the first attempt.

## 8. Verdict

**The current peripheral abstraction is sufficient for at least two different peripheral classes (timer-driven and state-machine-driven) without requiring kernel changes.**

TIM and UART provide strong evidence that the `EmberPeripheral` interface generalizes across independent peripheral categories. They share characteristics: interrupt-driven, register-based, single-device ownership. DMA will validate cross-peripheral interaction — the remaining uncertainty in the abstraction.

The UART bugs were the best possible kind: peripheral semantics (writable mask, RX state machine). The architecture absorbed the new peripheral and exposed flaws where they belong: inside the peripheral model.
