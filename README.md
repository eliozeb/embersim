# EmberSim

EmberSim runs STM32 firmware on your PC — no hardware required. It generates a
deterministic execution trace from your HAL calls, so you can detect firmware
regressions in CI before they reach a physical board.

```
Commit firmware
      │
CI executes natively
      │
Deterministic trace
      │
Compare against baseline
      │
✅ PASS or ❌ regression detected
```

## Quickstart

```bash
# 1. Check your environment
embersim doctor

# 2. Generate simulation workspace from your HAL header
embersim init -f Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h \
              -I Core/Inc -I Drivers/CMSIS/Include

# 3. Check HAL coverage
embersim check

# 4. Build, execute, generate trace
embersim run

# 5. Save the golden baseline
embersim baseline create -t trace.jsonl

# 6. Generate CI workflow
embersim ci-init

# 7. Push — CI runs automatically on every push and PR
git add baseline/ .github/workflows/embersim.yml embersim.toml
git commit -m "Add EmberSim regression CI"
git push
```

**Target:** first trace in under 5 minutes, green CI in under 15.

## Supported peripherals

TIM · UART · SPI · I2C · GPIO · NVIC interrupt dispatch · Register read/write tracking

## Documentation

| Guide | What it covers |
|-------|---------------|
| [Getting Started](docs/GETTING_STARTED.md) | Step-by-step: clone → green CI in 15 minutes |
| [HAL Boundary Guide](docs/HAL_BOUNDARY.md) | How EmberSim replaces hardware HAL calls |
| [CI Setup Guide](docs/CI_SETUP.md) | Workflow configuration, variables, baseline management |
| [Troubleshooting](docs/TROUBLESHOOTING.md) | Common errors and their fixes |

## Requirements

- Rust toolchain ([rustup.rs](https://rustup.rs))
- `gcc` or `clang` on your PATH
- STM32 firmware project with a HAL header

## Install

```bash
cargo install --git https://github.com/eliozeb/embersim embersim-cli
```

## License

MIT
