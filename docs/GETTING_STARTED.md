# Getting Started — Zero to Green CI

This guide takes you from a fresh clone to a working CI pipeline that catches
firmware regressions automatically. **Target time: under 15 minutes.**

You need:
- An STM32 firmware project (any STM32F4 or STM32G0 project with a HAL header)
- `gcc` or `clang` on your PATH
- Git

## 1. Install prerequisites

### Rust toolchain

If you don't have Rust installed:

```bash
# Linux / macOS
curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh

# Windows: download and run https://rustup.rs
```

Restart your shell, then verify:

```bash
rustc --version
cargo --version
```

### EmberSim CLI

```bash
cargo install --git https://github.com/eliozeb/embersim embersim-cli
```

Verify:

```bash
embersim doctor
```

This also checks for a C preprocessor (gcc/clang) and CMake. Fix any FAIL lines
before continuing — the output tells you exactly what to install for your OS.

## 2. Initialize the simulation workspace

Run this from your firmware project root:

```bash
embersim init \
  -f Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h \
  -I Core/Inc \
  -I Drivers/CMSIS/Include \
  -o ember_sim
```

| Flag | What it is |
|------|-----------|
| `-f` | Path to your STM32 HAL master header — the file that includes all peripheral headers |
| `-I` | Include directory needed to resolve `#include` directives (repeatable) |
| `-o` | Where to create the workspace (default: `ember_sim`) |

The `-I` flags tell the preprocessor where to find headers. Common paths:

| Project type | `-f` example | `-I` examples |
|-------------|-------------|--------------|
| CubeMX F4 | `Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h` | `Core/Inc`, `Drivers/CMSIS/Include` |
| CubeMX G0 | `Drivers/STM32G0xx_HAL_Driver/Inc/stm32g0xx_hal.h` | `Core/Inc`, `Drivers/CMSIS/Include` |
| Custom layout | `lib/hal/stm32f4xx_hal.h` | `lib/hal`, `lib/cmsis` |

If you're unsure where your HAL header is:

```bash
find . -name "stm32*_hal.h"
```

Add `-I` for each directory that contains headers included by that file.

**Tip:** Standard CubeMX projects can skip the manual flags:

```bash
embersim init --auto
```

This auto-detects the HAL header and include paths.

### What init creates

```
ember_sim/
├── mocks/                          # HAL stubs + simulation kernel
│   ├── mock_hal.h / mock_hal.c     # Auto-generated weak stubs for every HAL function
│   ├── mock_tim.h / mock_tim.c     # Timer peripheral model
│   ├── mock_uart.c                 # UART peripheral model
│   ├── mock_spi.c / mock_spi.h     # SPI peripheral model
│   ├── mock_i2c.c                  # I2C peripheral model
│   ├── mock_state.c                # Stateful GPIO mock
│   ├── trace_log.h / trace_log.c   # JSON Lines trace logger
│   ├── ember_sim_kernel.h / .c     # Simulation kernel
│   └── ember_regs.h / .c           # Register model
├── host_main.c                     # Host runner template
└── embersim.toml                   # Skeleton config (copy to project root)
```

## 3. Create your project configuration

The active configuration lives at your **project root** (not inside `ember_sim/`).
The init command generates a skeleton at `ember_sim/embersim.toml` — use it as a
starting point:

```bash
cp ember_sim/embersim.toml ./embersim.toml
```

Now edit `embersim.toml` at your project root:

```toml
[project]
name = "my-firmware"
description = "Motor controller firmware"

[build]
sources = [
    "Core/Src/main.c",
    "Core/Src/motor.c",
    "ember_sim/host_main.c",       # ← required: the simulation host runner
]
includes = ["Core/Inc", "Drivers/CMSIS/Include"]

[simulation]
entry = "app_init"
trace = "trace.jsonl"
duration_ms = 100
```

**Important details:**

- **`[build.sources]`** must include `ember_sim/host_main.c`. This is the host
  runner that provides `main()` and calls your firmware's entry points.
- **`[build.includes]`** should list every directory your firmware's `#include`
  directives need to resolve.
- If your firmware uses `main()` instead of `app_init()`/`app_run()`, you'll need
  to customize `ember_sim/host_main.c`. See [HAL_BOUNDARY.md](HAL_BOUNDARY.md)
  for how to adapt it.

## 4. Check project readiness

```bash
embersim check -o ember_sim
```

This scans your firmware against the generated mocks and reports:

```
Project analysis

HAL functions:
  HAL_UART_Transmit                  OK
  HAL_TIM_Base_Init                  OK
  … (abbreviated)

HAL types:
  UART_HandleTypeDef                 OK
  TIM_HandleTypeDef                  MISSING
    → Replace '#include "stm32f4xx_hal.h"' with '#include "mock_hal.h"'

Result: 1 issue(s) before simulation.
```

**OK** means the symbol is covered. **MISSING** means your firmware uses something
the simulation doesn't provide yet. See [HAL_BOUNDARY.md](HAL_BOUNDARY.md) for how
to resolve these.

If you see CMSIS macro issues, run:

```bash
embersim init --repair
```

This generates compatibility shims without modifying your firmware source.

## 5. Run your first simulation

```bash
embersim run -o ember_sim
```

Expected output:

```
Building firmware...
Build successful
Running simulation...
Trace generated (150000+ events)
No baseline found. Run 'embersim baseline create' to set one.
```

Your firmware just executed on a simulated STM32 — no hardware required.

## 6. Create the golden baseline

```bash
embersim baseline create -t trace.jsonl
```

This copies the trace to `baseline/trace.jsonl`. Expected output:

```
Baseline created: baseline/trace.jsonl
  events: 150175
```

Do not commit yet — you'll commit everything together in Step 9.

## 7. Generate the CI workflow

```bash
embersim ci-init
```

This creates `.github/workflows/embersim.yml`. Do not commit yet.

## 8. Configure GitHub

Set one repository variable:

| Variable | Value |
|----------|-------|
| `HAL_HEADER` | `Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h` |

The value must match the `-f` argument you used in Step 2.

GitHub → Settings → Secrets and variables → Actions → Variables → New repository variable

That's the only required variable. Advanced users can also set `EMBERSIM_REPO`
to use a fork of EmberSim (defaults to `eliozeb/embersim`).

## 9. Commit and push

```bash
git add baseline/ .github/workflows/embersim.yml embersim.toml
git commit -m "Add EmberSim regression CI"
git push
```

Open your repository on GitHub → Actions. You should see **Firmware Regression**
run and all steps pass within ~2 minutes:

```
✓ Checkout repository
✓ Install Rust toolchain
✓ Install gcc
✓ Checkout EmberSim
✓ Cache Rust dependencies
✓ Build EmberSim CLI
✓ Initialize simulation workspace
✓ Check project readiness
✓ Run firmware regression
✓ Compare regression baseline
```

## 10. Prove it works (optional, recommended)

Make a small intentional change to your firmware — comment out a HAL call,
change a parameter. Push to a branch and open a PR. CI should fail with:

```
Result: FAIL — firmware regression detected

First divergence:
  line: 42

Expected:
  HAL_UART_Transmit

Observed:
  REGISTER_EVENT
```

Revert the change. CI returns to green. You now have proof that your CI catches
firmware regressions before merge.

---

## Next steps

- [HAL Boundary Guide](HAL_BOUNDARY.md) — understand how EmberSim replaces hardware HAL calls
- [CI Setup Guide](CI_SETUP.md) — detailed CI configuration and troubleshooting
- [Troubleshooting](TROUBLESHOOTING.md) — common errors and their fixes

## If you get stuck

1. Run `embersim doctor` first — most issues are missing tools
2. Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for your error message
3. The [HAL Boundary Guide](HAL_BOUNDARY.md) explains the most common conceptual hurdle
