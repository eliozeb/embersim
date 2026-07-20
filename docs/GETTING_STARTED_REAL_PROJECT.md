# Getting Started: Add EmberSim to Your Firmware Project

This guide assumes you have an existing STM32 firmware project and want to add deterministic regression testing without hardware.

**Target time to first trace: under 1 hour.**

## Prerequisites

- gcc on PATH (`embersim doctor` checks this)
- Rust toolchain (`embersim doctor` checks this)
- Your STM32 firmware project (CubeMX, Makefile, or CMake-based)

## Step 1: Initialize the simulation workspace

From your firmware project root:

```bash
embersim init -f Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h \
              -I Core/Inc \
              -I Drivers/CMSIS/Include \
              -o ember_sim
```

This generates:
```
ember_sim/
├── mocks/                  # HAL stubs + simulation kernel
│   ├── mock_hal.h/.c       # Auto-generated weak stubs
│   ├── trace_log.h/.c      # JSON Lines trace logger
│   ├── mock_state.c        # Stateful GPIO mock
│   ├── mock_uart.c         # UART peripheral model
│   ├── mock_tim.c/.h       # TIM peripheral model
│   ├── ember_sim_kernel.h/.c  # Simulation kernel
│   └── ember_regs.h/.c     # Register model
├── host_main.c             # Host runner template
└── embersim.toml           # Project configuration
```

## Step 2: Configure your project

Edit `embersim.toml`:

```toml
[project]
name = "my-firmware"
description = "Motor controller firmware"

[build]
sources = ["Core/Src/main.c", "Core/Src/motor.c", "host_main.c"]

[simulation]
entry = "app_init"
trace = "trace.jsonl"
duration_ms = 100
```

If your firmware uses `main()` instead of `app_init()`/`app_run()`, adapt `host_main.c` to call your entry points.

## Step 3: Refactor firmware entry point (if needed)

EmberSim uses a two-function entry model:

```c
// Your firmware provides:
void app_init(void);   // Called once at startup
void app_run(void);    // Called in simulation loop
```

If your firmware uses a standard `main()` that calls `HAL_Init()` + `SystemClock_Config()` + infinite loop, extract the loop body into `app_run()` and call it from `host_main.c`.

Example refactor:

```c
// Before (your main.c):
int main(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_USART2_Init();

    while (1) {
        // application loop
    }
}

// After:
void app_init(void) {
    HAL_Init();
    SystemClock_Config();
    MX_GPIO_Init();
    MX_TIM2_Init();
    MX_USART2_Init();
}

void app_run(void) {
    // application loop body
}
```

## Step 4: Handle hardware-specific code

Code that accesses hardware registers directly (not through HAL) may need stubs. Common cases:

| Pattern | Solution |
|---|---|
| `__disable_irq()` / `__enable_irq()` | Stub as empty macros |
| Direct register access (`GPIOA->ODR = ...`) | Not supported — use HAL API |
| Inline assembly | Not supported — use C equivalents |
| `SystemInit()` | Already stubbed by `host_main.c` |
| `HardFault_Handler()` | Already provided as weak default |

## Step 5: Create your first baseline

```bash
embersim run
```

First run creates a build of your firmware against the simulation mocks, executes it, and generates a trace. Then:

```bash
embersim baseline create
```

This saves the trace as the known-good reference.

## Step 6: Add to CI

Example GitHub Actions workflow:

```yaml
name: Firmware Regression
on: [push, pull_request]
jobs:
  regression:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v4
      - name: Install EmberSim
        run: cargo install embersim-cli
      - name: Init workspace
        run: embersim init -f $HAL_HEADER -I Core/Inc -o ember_sim
      - name: Run regression
        run: embersim run -o ember_sim
```

## What to expect

On first attempt, you may encounter:

1. **Missing HAL function in generated mocks.** Add it to `embersim.toml` `[build.defines]` or include the header. Re-run `embersim init`.

2. **Undefined symbol at link time.** Your firmware calls a function not in the HAL. Either stub it or exclude the file.

3. **Simulation runs but behaviour differs.** Your firmware may depend on hardware timing or uninitialized peripheral state. Adjust the simulation duration or initialize peripheral state in `app_init()`.

4. **Trace too large.** Increase `duration_ms` carefully — every µs of simulation produces trace events. Start with 50ms and increase as needed.

## Success

When `embersim run` prints:

```
Result: PASS
Firmware behaviour matches baseline.
```

Your firmware regression test runs in CI without hardware.
