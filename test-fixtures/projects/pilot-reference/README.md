# Pilot 0 — Reference Firmware

Industrial-style STM32F407 firmware for EmberSim Stage 7 internal validation.

**Simulates:** Motor controller + temperature sensor node
**Peripherals:** GPIO (LED + fault input), UART (command interface), TIM2 (PWM + 10 ms tick), SPI1 (temperature sensor)

## What this is

This is NOT a real product. It is a controlled validation instrument designed to
answer one question:

> Can a normal embedded workflow be represented deterministically by EmberSim
> without special treatment?

If this firmware produces a deterministic trace and detects regressions,
external Pilot 1 has a proven success path.

## Quickstart — Native Build

```bash
# Build natively with gcc (no EmberSim)
cd test-fixtures/projects/pilot-reference
make clean && make
# → build/pilot-reference
```

This confirms the firmware compiles without EmberSim.

## Quickstart — EmberSim

```bash
# From the pilot-reference/ directory:

# 1. Generate simulation workspace
embersim init \
  -f firmware/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h \
  -I firmware/Core/Inc \
  -I firmware/Drivers/STM32F4xx_HAL_Driver/Inc \
  -o ember_sim

# 2. Check HAL coverage
embersim check -o ember_sim

# 3. Build, execute, generate trace
embersim run -o ember_sim

# 4. Create golden baseline
embersim baseline create -t trace.jsonl

# 5. Verify determinism
embersim run -o ember_sim
embersim compare -g baseline/trace.jsonl -c trace.jsonl
# Expected: Result: PASS

# 6. Prove regression detection
# Comment out HAL_UART_Transmit in firmware/Core/Src/uart_service.c
embersim run -o ember_sim
# Expected: Result: FAIL — firmware regression detected
```

## Firmware Architecture

```
main.c
  ├── SystemClock_Config()
  ├── MX_GPIO_Init()      → PA5 (LED out), PA0 (fault in)
  ├── MX_USART2_UART_Init() → 115200-8N1
  ├── MX_TIM2_Init()       → 10 ms tick + PWM CH1
  ├── MX_SPI1_Init()       → SPI master
  └── app_init() → app_run() loop

app.c                     — central state machine, command dispatch
uart_service.c            — UART RX/TX, command parsing
motor_control.c           — TIM2 PWM motor start/stop
sensor.c                  — SPI temperature sensor (fixed 25.0°C)
```

## Commands

Send via UART (in simulation, inject via uart_service_inject_rx):

| Command       | Response             |
|---------------|----------------------|
| `STATUS`      | `OK:READY` or `OK:MOTOR_RUNNING` |
| `MOTOR ON`    | `OK:MOTOR_RUNNING`   |
| `MOTOR OFF`   | `OK:MOTOR_STOPPED`   |
| `TEMP`        | `OK:TEMP=25.0C`      |
| anything else | `ERR:UNKNOWN_CMD`    |

## Acceptance Criteria

See `docs/STAGE7_PILOT0_REFERENCE_SPEC.md` §10 for the full list.

1. `make` → exit 0, `build/pilot-reference` exists
2. `embersim init` → exit 0, `ember_sim/mocks/` exists
3. `embersim check` → 0 issues
4. `embersim run` → trace generated, deterministic across runs
5. `embersim baseline create` → exit 0
6. Regression: comment out `HAL_UART_Transmit` → `embersim run` reports FAIL

## References

- [Stage 7 Pilot 0 Spec](../../docs/STAGE7_PILOT0_REFERENCE_SPEC.md)
- [B4 Dry-Run Report Template](../../docs/STAGE7_B4_DRY_RUN_REPORT.md)
- [B4 Pilot Selection](../../docs/STAGE7_B4_PILOT_SELECTION.md)
