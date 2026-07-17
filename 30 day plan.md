Revised 30-Day Execution Plan
Week 1: The Foundation — Parse Everything, Stub Everything
Day 1 — Repo & Toolchain
Create github.com/YOURNAME/embersim. MIT license.
Rust workspace: crates/embersim-core (lib), crates/embersim-cli (bin).
Add deps: serde, serde_json, clap, anyhow, chrono, regex, handlebars (for HTML templating).
Install gcc, cmake, clang (any version).
Deliverable: cargo build succeeds. README states: "EmberSim generates host-native HAL stubs from your STM32 project so you can compile and test firmware logic on your PC."
Day 2 — The Preprocessor Pipeline
Write a Rust function that shells out to gcc -E -DEMBERSIM_MOCK <includes> stm32f4xx_hal.h.
The preprocessor expands all macros and includes.
Pipe output through a regex: ^([A-Za-z0-9_\s]+)\s+(HAL_[A-Za-z0-9_]+)\s*\(([^)]*)\)\s*;
Deliverable: CLI prints a JSON array of every HAL_* function signature found in the preprocessed output.
Day 3 — Stub Generator
For every extracted function, generate a C stub:
Return type HAL_StatusTypeDef → return HAL_OK;
Return type void → empty body.
Return type pointer → return NULL;
Return type uint32_t → return 0;
Generate mock_stm32f4xx_hal.h with all declarations and minimal type definitions (HAL_StatusTypeDef as enum, GPIO_TypeDef as opaque struct, etc.).
Deliverable: Running embersim generate --hal /path/to/stm32f4xx_hal.h --output ember_sim/mocks/ produces mock_hal.c and mock_hal.h that compile with gcc -c on x86.
Day 4 — CMSIS Shim Layer
Create ember_sim/cmsis_shim/stm32f4xx.h with:
Dummy peripheral base addresses (GPIOA_BASE = 0x10000000, etc.)
Minimal struct definitions: GPIO_TypeDef, USART_TypeDef, I2C_TypeDef, SPI_TypeDef, TIM_TypeDef (just the first field or empty).
Core IRQn enum values.
__IO macro defined as volatile.
Deliverable: A non-trivial HAL header (stm32f4xx_hal_gpio.h) can be included on x86 without errors.
Day 5 — GPIO Mock (Instance-Based)
Implement stateful GPIO mock:
Lookup table: static uint16_t gpio_pin_states[11]; indexed by port base address (GPIOA_BASE → 0, GPIOB_BASE → 1, etc.).
HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, ...) → gpio_pin_states[idx] |= pin;
HAL_GPIO_ReadPin(...) → returns configured state.
Deliverable: A test C file that calls HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_SET) and reads it back works on x86.
Day 6 — UART Mock (Instance-Based)
Implement stateful UART mock with circular buffers:
Lookup by huart->Instance.
HAL_UART_Transmit() copies to TX buffer.
HAL_UART_Receive() reads from RX buffer pre-loaded from embersim.json.
Deliverable: UART loopback test passes on x86.
Day 7 — I2C + SPI + TIM Stubs
Generate stateful stubs for I2C, SPI, TIM that return HAL_OK and log to trace.
Full stateful behavior (virtual devices, timers) is Pro-tier and comes later.
Deliverable: Any project using these peripherals compiles on x86.
Week 2: Host Runner & Real Project Integration
Day 8 — host_main.c & Missing Symbols
Write ember_sim/host_main.c:
int main(int argc, char **argv) for x86.
Weak void Default_Handler(void) {}.
Stub void SystemInit(void) {}.
Calls user-provided void app_init(void) and void app_run(void) (or loops app_tick()).
Deliverable: A minimal user app.c with app_init() and app_tick() links and runs.
Day 9 — embersim init & Build Generation
CLI command: embersim init reads embersim.json and generates:
ember_sim/host_CMakeLists.txt
ember_sim/cmsis_shim/ (copied from template)
ember_sim/mocks/ (generated stubs)
embersim.json schema:
JSON
{
  "project": {
    "name": "my-firmware",
    "source_files": ["src/app.c", "src/sensor.c"],
    "exclude_files": ["src/startup_stm32f407xx.s", "src/main.c"],
    "hal_path": "Drivers/STM32F4xx_HAL_Driver/Inc",
    "hal_conf": "Core/Inc/stm32f4xx_hal_conf.h"
  }
}
Deliverable: Running embersim init in a sample directory creates a buildable host project.
Day 10 — Sample Project: Real CubeMX Code
Generate a real STM32CubeMX project (LED blinky + UART printf).
Refactor slightly: move logic from main.c to app.c with app_init() and app_tick().
Keep main.c for ARM target. Exclude it from host build.
Deliverable: embersim init && embersim build compiles the project on x86.
Day 11 — Trace Logger (JSON Lines)
C library: trace_log(const char *func, const char *args_json).
Writes to ember_sim/out/trace.jsonl.
Format: {"ts_ms": 1234, "peripheral": "GPIO", "func": "HAL_GPIO_WritePin", "args": {"port": "A", "pin": 5, "state": 1}}.
Deliverable: Running the sample project produces a valid .jsonl file.
Day 12 — End-to-End Integration Test
Shell script test_e2e.sh:
embersim init
embersim generate
embersim build
embersim run
Assert trace.jsonl contains expected events.
Deliverable: CI-ready test that fails if mocks break.
Day 13 — Error Handling & Polish
Add anyhow context to all CLI commands.
Colored output for build status (green = pass, red = fail).
embersim run --trace shows live event count.
Deliverable: CLI feels like a professional tool.
Day 14 — Documentation: "Your First Host Test"
Write docs/getting-started.md:
How to refactor CubeMX main.c into app.c.
How to write embersim.json.
How to run embersim init && embersim build && embersim run.
Deliverable: A new user can follow the guide and succeed in 10 minutes.
Week 3: HTML Report & Pro Tier Mocks
Day 15 — HTML Report Generator (Rust)
Use handlebars or string templating to generate a single HTML file.
Sections: Header (project name, pass/fail, duration), Trace Timeline, Peripheral State Inspector.
Embed CSS and JS (no external dependencies).
Deliverable: embersim report --input trace.jsonl --output report.html works.
Day 16 — SVG Timeline (Static)
Vanilla JS in the HTML: parse trace.jsonl, group events by peripheral.
SVG with one row per peripheral (GPIO, UART, I2C, etc.).
X-axis: time in ms. Events as vertical lines or colored rectangles.
Deliverable: HTML report shows a readable timeline.
Day 17 — Peripheral State Inspector (HTML)
Table showing current state after the last trace event:
GPIO: Port A Pin 5 = HIGH, Port B Pin 0 = LOW.
UART: TX buffer = "Hello\n", RX queue length = 3.
Deliverable: Users can inspect "what would the hardware look like?"
Day 18 — Pro Tier: Advanced UART Mock
Configurable via embersim.json:
JSON
"mocks": {
  "uart": {
    "USART1": { "rx_bytes": [0x48, 0x49] }
  }
}
HAL_UART_Receive() dequeues these bytes.
Deliverable: Virtual UART input works.
Day 19 — Pro Tier: Advanced GPIO Mock
Configurable input states in embersim.json.
Simulated interrupt: ember_sim_trigger_irq(EXTI15_10_IRQn) calls the user's ISR callback.
Deliverable: Can test button-press logic without hardware.
Day 20 — Pro Tier: I2C Virtual Device
Configurable register map:
JSON
"i2c": {
  "I2C1": { "devices": { "0x48": { "registers": { "0x00": [0x1A, 0x3B] } } } }
}
HAL_I2C_Master_Transmit() selects device/register. HAL_I2C_Master_Receive() returns data.
Deliverable: Can test sensor reading logic.
Day 21 — Pro Tier: TIM (Timer) Mock
Configurable period and callback.
HAL_TIM_PeriodElapsedCallback() fires after N mock ticks.
Deliverable: Can test timer-based state machines.
Day 22 — License System (Ed25519)
Rust: verify license file signature against hardcoded public key.
Trial mode: if no license, Pro features work for 30 days from first run (tracked in ~/.embersim/first_run).
After trial: Pro mocks return HAL_OK with a warning log.
Deliverable: scripts/generate_license.py creates signed keys for customers.
Week 4: Launch
Day 23 — Feature Gating
Pro mocks check license_is_valid() at runtime.
If invalid: stateful behavior disabled, falls back to stub (returns HAL_OK, basic trace).
CLI: embersim run --pro enables Pro mocks if licensed.
Deliverable: Free tier always compiles. Pro tier unlocks stateful behavior.
Day 24 — Sample Project #2: I2C Sensor
Full demo: firmware reads a virtual temperature sensor (I2C), averages 4 samples, toggles LED if threshold exceeded.
embersim.json configures the virtual sensor.
Deliverable: A compelling 2-minute demo video script.
Day 25 — Screen Recording & Assets
Record: VS Code terminal showing embersim init → build → run → report.html.
Open report.html in browser. Show timeline and state inspector.
Convert to GIF.
Deliverable: assets/demo.gif and demo.mp4.
Day 26 — Blog Post Draft
Title: "I Stopped Waiting for Hardware: How I Test STM32 Firmware on My Laptop"
Structure:
Pain: 2-minute flash cycles, hardware delays.
Insight: Don't emulate the CPU. Stub the HAL.
Demo: 90-second GIF.
Architecture: Data-path mocks, instance-based state, HTML reports.
Call to action: GitHub stars, download, 30-day Pro trial.
Deliverable: docs/blog-post.md.
Day 27 — CI/CD Pipeline
.github/workflows/ci.yml: build Rust, run e2e test, lint.
.github/workflows/release.yml: build Linux/macOS/Windows binaries, package HTML template.
Deliverable: Green CI badge.
Day 28 — Release Packaging
cargo build --release (Linux). Use cross for Windows/macOS.
Tag v0.1.0.
GitHub Release with 3 binaries + html-template.zip.
Deliverable: Downloadable release.
Day 29 — Website / Landing Page
GitHub Pages single-page site.
Hero: GIF demo. Three bullets: Auto-mocks, Host-native, HTML reports.
Pricing: Free (compile on-host), Pro $299/yr (stateful mocks, advanced reports).
Deliverable: embersim.github.io live.
Day 30 — Launch
Post to r/embedded: "Show r/embedded: EmberSim — compile and test STM32 firmware on your PC without writing mocks. Auto-generates HAL stubs, outputs interactive HTML trace reports."
Post to Hacker News.
Monitor comments for 4 hours. Reply to every question.
Deliverable: Public launch. First users. First feedback.