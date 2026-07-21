# Stage 7 Pilot 0 — Reference Firmware Specification

**Status:** Spec frozen — implementation pending
**Date:** 2026-07-21
**Purpose:** Internal validation before external pilot (B4 Pilot 1)

---

## 1. Why This Exists

Pilot 0 is NOT a demo to prove EmberSim works. It is a controlled validation instrument designed to answer one question:

> Can a normal embedded workflow be represented deterministically by EmberSim without special treatment?

If the answer is "no" for a firmware we control completely, Pilot 1 (external) will certainly fail. If the answer is "yes," we have a reproducible success path and a permanent golden reference.

This fixture must be:
- **Representative** of a real industrial embedded project (motor controller, sensor node, actuator)
- **Honest** — no cheating to make EmberSim look better than it is
- **Minimal** — small enough to understand in one reading, complex enough to exercise boundaries
- **Reproducible** — anyone can clone, build, and run it

---

## 2. Simulated Hardware

The reference firmware targets an imaginary STM32F407 board. No physical hardware exists — all peripherals exist only as concepts the firmware addresses through HAL APIs.

| Peripheral | Instance | Purpose |
|-----------|----------|---------|
| GPIO | GPIOA Pin 5 | Status LED (output) |
| GPIO | GPIOA Pin 0 | Fault input (input, pull-up) |
| UART | USART2 | Command interface (115200-8N1) |
| TIM | TIM2 | 10 ms control loop tick |
| SPI | SPI1 | Temperature sensor (mock) |

No other peripherals. No DMA. No interrupts beyond TIM2 update.

---

## 3. Allowed Dependencies

| Dependency | Why |
|-----------|-----|
| STM32F4xx HAL (subset) | The HAL boundary EmberSim intercepts |
| CMSIS core headers | Required by HAL typedefs and macros |
| Standard C library (limited) | `snprintf`, `memset`, `strncmp` |

## 4. Forbidden Dependencies

| Dependency | Reason |
|-----------|--------|
| FreeRTOS or any RTOS | Separate milestone |
| DMA (any peripheral) | Not implemented |
| USB, CAN, Ethernet | Not implemented |
| Direct register access (`GPIOA->ODR = …`) | Violates HAL boundary |
| CubeMX-generated startup | Adds irrelevant complexity; startup is hand-written |
| Inline assembly | Not supported |
| CMSIS intrinsics in core logic | `--repair` provides stubs; avoid dependency on them |
| `HAL_Delay` or `SysTick` beyond init | Simulation uses tick-driven advance, not wall-clock delay |

---

## 5. Module Specifications

### 5.1 `main.c` — Entry Point

```c
// Responsibilities:
// - Call HAL_Init()
// - Initialize GPIO, UART, TIM, SPI via MX_* functions
// - Delegate to app_init() / app_run()

void SystemClock_Config(void);  // minimal: just sets APB prescalers in a struct

int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_TIM2_Init();
    MX_SPI1_Init();
    app_init();
    while (1) {
        app_run();
    }
}
```

EmberSim entry mapping (documented, not compiled into the firmware — the firmware builds natively with `main()`):

```c
// For EmberSim: extract to two-function model
void app_init(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_USART2_UART_Init();
    MX_TIM2_Init();
    MX_SPI1_Init();
}

void app_run(void) {
    // called repeatedly by host_main.c
}
```

### 5.2 `app.c` / `app.h` — Application Core

```c
// Responsibilities:
// - LED heartbeat toggle every 100 ms
// - Command dispatch from UART
// - Control loop tick counting

#define LED_PIN          GPIO_PIN_5
#define LED_PORT         GPIOA
#define HEARTBEAT_MS     100

typedef struct {
    uint32_t tick_count;
    uint8_t  motor_running;
    float    temperature;
} AppState;

void app_init(void);   // initialize app state
void app_run(void);    // called by main loop or host_main

// Internal:
// - On TIM2 update: increment tick_count
// - Every HEARTBEAT_MS: toggle LED
// - On UART RX complete: dispatch command
```

### 5.3 `uart_service.c` / `uart_service.h` — Command Interface

```c
// Responsibilities:
// - Receive commands via UART interrupt
// - Parse command strings
// - Transmit responses

#define UART_CMD_BUF_SIZE  64

typedef enum {
    CMD_NONE = 0,
    CMD_STATUS,
    CMD_MOTOR_ON,
    CMD_MOTOR_OFF,
    CMD_TEMPERATURE,
    CMD_UNKNOWN,
} Command;

void uart_service_init(void);
Command uart_service_poll(void);   // check for received command
void uart_service_respond(const char *response);

// Supported commands:
//   "STATUS"       → "OK:READY" or "OK:MOTOR_RUNNING"
//   "MOTOR ON"     → "OK:MOTOR_RUNNING"
//   "MOTOR OFF"    → "OK:MOTOR_STOPPED"
//   "TEMP"         → "OK:TEMP=25.0C"
//   anything else  → "ERR:UNKNOWN_CMD"
```

**Expected trace events:**
- UART RX complete → `HAL_UART_RxCpltCallback`
- Command dispatch → function call trace
- UART TX complete → `HAL_UART_TxCpltCallback`

### 5.4 `motor_control.c` / `motor_control.h` — TIM PWM Motor Output

```c
// Responsibilities:
// - Start/stop TIM2 CH1 PWM output
// - Set duty cycle based on command

#define MOTOR_PWM_CHANNEL  TIM_CHANNEL_1
#define MOTOR_DUTY_MAX     1000

typedef enum {
    MOTOR_STOPPED = 0,
    MOTOR_RUNNING = 1,
} MotorState;

void motor_init(void);
void motor_start(void);
void motor_stop(void);
MotorState motor_get_state(void);

// When running:
//   HAL_TIM_PWM_Start(&htim2, MOTOR_PWM_CHANNEL)
//   HAL_TIM_PWM_Stop(&htim2, MOTOR_PWM_CHANNEL) on stop
```

**Expected trace events:**
- `HAL_TIM_PWM_Start` called
- `HAL_TIM_PWM_Stop` called
- PWM callback on period elapsed

### 5.5 `sensor.c` / `sensor.h` — SPI Temperature Sensor

```c
// Responsibilities:
// - Read temperature via SPI (mock sensor)
// - Return fixed value (25.0°C) for deterministic trace

#define SPI_TEMP_CMD  0x00   // read temperature command
#define SPI_TEMP_FIXED 25.0f // fixed response — no real sensor

float sensor_read_temperature(void);

// Implementation:
//   HAL_SPI_TransmitReceive(&hspi1, tx_buf, rx_buf, 2, 100);
//   Parse rx_buf → always returns 25.0°C
```

**Expected trace events:**
- `HAL_SPI_TransmitReceive` called
- Fixed temperature value in trace output

---

## 6. HAL Functions Exercised

This is the HAL surface that EmberSim must cover:

| Module | HAL Function | Peripheral |
|--------|------------|-----------|
| GPIO | `HAL_GPIO_WritePin` | GPIO |
| GPIO | `HAL_GPIO_TogglePin` | GPIO |
| GPIO | `HAL_GPIO_ReadPin` | GPIO |
| GPIO | `HAL_GPIO_Init` | GPIO |
| UART | `HAL_UART_Init` | UART |
| UART | `HAL_UART_Transmit` | UART |
| UART | `HAL_UART_Receive_IT` | UART |
| TIM | `HAL_TIM_Base_Init` | TIM |
| TIM | `HAL_TIM_Base_Start_IT` | TIM |
| TIM | `HAL_TIM_PWM_Init` | TIM |
| TIM | `HAL_TIM_PWM_Start` | TIM |
| TIM | `HAL_TIM_PWM_Stop` | TIM |
| SPI | `HAL_SPI_Init` | SPI |
| SPI | `HAL_SPI_TransmitReceive` | SPI |

Total: 15 HAL functions across 4 peripherals.

---

## 7. HAL Types Required

| Type | Used By |
|------|---------|
| `GPIO_InitTypeDef` | GPIO init |
| `UART_HandleTypeDef` | UART init + TX/RX |
| `TIM_HandleTypeDef` | TIM init + PWM |
| `TIM_OC_InitTypeDef` | PWM channel config |
| `SPI_HandleTypeDef` | SPI init + transfer |

---

## 8. File Layout

```
test-fixtures/projects/pilot-reference/
├── firmware/
│   ├── Core/
│   │   ├── Src/
│   │   │   ├── main.c              # Entry point + MX_* init
│   │   │   ├── app.c               # Application core
│   │   │   ├── uart_service.c      # UART command handler
│   │   │   ├── motor_control.c     # TIM PWM motor
│   │   │   └── sensor.c            # SPI temperature sensor
│   │   └── Inc/
│   │       ├── app.h
│   │       ├── uart_service.h
│   │       ├── motor_control.h
│   │       └── sensor.h
│   └── Drivers/
│       └── STM32F4xx_HAL_Driver/
│           ├── Inc/                # HAL header subset
│           │   ├── stm32f4xx_hal.h
│           │   ├── stm32f4xx_hal_gpio.h
│           │   ├── stm32f4xx_hal_uart.h
│           │   ├── stm32f4xx_hal_tim.h
│           │   ├── stm32f4xx_hal_spi.h
│           │   ├── stm32f4xx_hal_rcc.h
│           │   └── stm32f4xx_hal_def.h
│           └── Src/                # HAL source stubs (not compiled by EmberSim)
│               └── stm32f4xx_hal.c
├── Makefile                        # Native gcc build
├── embersim.toml                   # EmberSim project config
├── README.md                       # Pilot 0 instructions
└── .gitignore
```

After `embersim init`:

```
test-fixtures/projects/pilot-reference/
├── ember_sim/                      # Generated workspace
│   ├── mocks/
│   ├── host_main.c
│   └── embersim.toml               # Skeleton (copy to project root)
└── baseline/
    └── trace.jsonl                 # After baseline create
```

---

## 9. Build Requirements

### Native build (gcc)

```bash
cd test-fixtures/projects/pilot-reference/firmware
make
# → build/pilot-reference.elf
```

The Makefile must:
- Compile all `.c` files in `Core/Src/`
- Include `Core/Inc/` and `Drivers/STM32F4xx_HAL_Driver/Inc/`
- Link with `-lm` if needed
- Produce `build/pilot-reference.elf`

### EmberSim build

```bash
cd test-fixtures/projects/pilot-reference
embersim init -f firmware/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h \
              -I firmware/Core/Inc \
              -I firmware/Drivers/STM32F4xx_HAL_Driver/Inc \
              -o ember_sim
embersim check -o ember_sim
embersim run -o ember_sim
```

---

## 10. Acceptance Criteria

Pilot 0 passes only if ALL of these hold:

### A1 — Native build

```bash
cd test-fixtures/projects/pilot-reference/firmware
make clean && make
# Exit code 0
# build/pilot-reference.elf exists
```

### A2 — EmberSim init succeeds

```bash
embersim init -f firmware/Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h \
              -I firmware/Core/Inc \
              -I firmware/Drivers/STM32F4xx_HAL_Driver/Inc \
              -o ember_sim
# Exit code 0
# ember_sim/mocks/ exists
# ember_sim/host_main.c exists
```

### A3 — Check reports zero issues (or only known-acceptable issues)

```bash
embersim check -o ember_sim
# Result: 0 issue(s) before simulation
#   (or ≤ 1 cosmetic issue with documented workaround)
```

### A4 — Run produces deterministic trace

```bash
embersim run -o ember_sim
# Exit code 0
# Trace generated (N events) — N > 0
```

### A5 — Trace is identical across repeated runs

```bash
embersim run -o ember_sim
cp trace.jsonl trace1.jsonl
embersim run -o ember_sim
cp trace.jsonl trace2.jsonl
diff trace1.jsonl trace2.jsonl
# No differences
```

### A6 — Baseline create succeeds

```bash
embersim baseline create -t trace.jsonl
# Exit code 0
# baseline/trace.jsonl exists
```

### A7 — Regression detection works

Intentional break: comment out `HAL_UART_Transmit` in `uart_service.c`.

```bash
embersim run -o ember_sim
# Result: FAIL — firmware regression detected
# Expected: UART TX event
# Observed: no UART TX event (or different event)
```

Revert the break. Confirm PASS returns.

### A8 — CI workflow generates

```bash
embersim ci-init
# Exit code 0
# .github/workflows/embersim.yml exists
```

---

## 11. What Pilot 0 Does NOT Validate

Intentionally out of scope:

| Concern | Why out of scope |
|---------|-----------------|
| External engineer onboarding | Pilot 1 validates this |
| Real hardware timing | Simulation time, not wall time |
| RTOS integration | Separate milestone |
| Complex CubeMX project structure | Pilot 1 may exercise this |
| Multi-file include path resolution edge cases | Pilot 2 (G0 bootloader) |
| Flash/EEPROM emulation | Not implemented |
| ADC/DAC | Not implemented |
| Watchdog | Not implemented |

---

## 12. Success / Failure Classification

### Pilot 0 is a SUCCESS if:

All A1–A8 pass. Any failures are documented, classified, and either fixed or deferred with rationale.

### Pilot 0 is CONDITIONAL if:

A1–A6 pass (core workflow works) but A7 (regression detection) reveals a gap in trace comparison that requires a code change.

### Pilot 0 is a FAILURE if:

A1 (native build) or A2 (ember init) fails and cannot be resolved without modifying `crates/embersim-core/` or `crates/embersim-cli/`. This would indicate a product defect that must be fixed before any external pilot.

---

## 13. References

- [STAGE7_B4_PILOT_SELECTION.md](STAGE7_B4_PILOT_SELECTION.md) — Selection framework and execution sequence
- [STAGE7_B4_DRY_RUN_REPORT.md](STAGE7_B4_DRY_RUN_REPORT.md) — Report template to fill after execution
- [GETTING_STARTED.md](GETTING_STARTED.md) — The onboarding guide this fixture validates
- [HAL_BOUNDARY.md](HAL_BOUNDARY.md) — HAL interception model
