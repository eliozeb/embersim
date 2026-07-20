# Product Evidence Log

**Purpose:** Track whether EmberSim is solving real customer problems — separate from architecture validation. Architecture success ≠ product success.

**Last updated:** 2026-07-19

---

## Product Claims

| Claim | Evidence | Status |
|---|---|---|
| Can run firmware without hardware | `real-firmware/` — production-like TIM+UART workload | ✅ |
| Produces deterministic traces | SHA256 match across runs | ✅ |
| Trace explains firmware behavior | UART transmits visible in trace | ✅ |
| Saves debugging time | Customer timing comparison | ⏳ |
| Integrates into CI | GitHub Actions workflow | ⏳ |
| Explains failures | Engineer feedback on trace usefulness | ⏳ |
| Worth paying for | Pilot customer | ⏳ |

---

## G5 Evidence: Real Firmware Project

**Project:** `real-firmware/` — production-style STM32 firmware using TIM2 (periodic interrupt) and USART2 (data transmit in callback).

**Workflow:**
```
1. Firmware source: app.c (TIM init + UART init + callback)
2. embersim init: generates mocks from HAL header
3. gcc compile: app.c + host_main.c + 9 mocks → firmware.exe
4. Execute: 50ms simulation
5. Trace: 150,176 lines, deterministic (SHA256 identical across runs)
```

**Evidence in trace:**
- TIM interrupts: `NVIC_DISPATCH irq=28` visible (50+ occurrences)
- Callback chain: `SOFTWARE_EVENT component=TIM event=irq_handler`
- UART transmits: `HAL_UART_Transmit started` at 5 timestamps (30s, 60s, 90s, 120s, 150s ms)
- Register events: CNT increments, SR changes, CR1 writes — all observable

**What this proves:**
- A developer can write STM32 HAL firmware and run it on EmberSim
- Both TIM and UART peripherals work together in the same firmware
- The trace captures the full execution: hardware events → interrupts → callbacks → application code

**What remains to prove:**
- Can a developer debug a real failure using the trace?
- Can this run in CI?
- Would an embedded engineer pay for this?

---

## Next Product Milestone

1. Find an embedded engineer with a real STM32 project
2. Run their firmware on EmberSim
3. Ask: "Does this trace help you understand what your firmware did?"
4. If yes: product validated. If no: learn what's missing.
