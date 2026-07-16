# EmberSim Project State Snapshot
**Last Session:** Day 6 Integration Complete  
**Date:** 2026-07-17  
**Next:** Day 7 (I2C Virtual Device) or Day 8 (CMake Build Pipeline)

---

## What Exists Now

### Rust Workspace (`C:\embersim\embersim`)
```
embersim/
├── Cargo.toml
├── crates/
│   ├── embersim-core/
│   │   ├── Cargo.toml          (deps: serde, serde_json, regex, anyhow)
│   │   └── src/
│   │       ├── lib.rs           (pub mod: clean, model, parser, pipeline, preprocess, stubgen, templates, project)
│   │       ├── model.rs         (HalFunction, Parameter, Project structs)
│   │       ├── preprocess.rs    (gcc/clang -E execution)
│   │       ├── clean.rs         (strip preprocessor noise)
│   │       ├── parser.rs        (regex extraction + structured params)
│   │       ├── pipeline.rs      (orchestration: preprocess→clean→parse→dedup)
│   │       ├── stubgen.rs       (generates mock_hal.h + mock_hal.c with weak stubs)
│   │       ├── templates.rs     (include_bytes! for embedded C templates)
│   │       └── project.rs       (Init command: writes full ember_sim/ workspace)
│   └── embersim-cli/
│       ├── Cargo.toml
│       └── src/
│           └── main.rs          (Commands: Generate, Init)
├── test-fixtures/
│   └── minimal_hal.h            (16 HAL functions: GPIO + UART)
└── test-embersim/               (generated workspace from `embersim init`)
    ├── mocks/
    │   ├── mock_hal.h           (auto-generated types + forward declarations)
    │   ├── mock_hal.c           (weak stubs with trace_log calls)
    │   ├── trace_log.h          (trace_log_init/close/log declarations)
    │   ├── trace_log.c          (append-only JSON Lines writer)
    │   ├── mock_state.c         (stateful GPIO: WritePin/ReadPin/TogglePin)
    │   └── mock_uart.c          (professional UART: blocking, IT, DMA, errors)
    └── host_main.c              (minimal host runner, calls app_init/app_run)
```

### C Templates (`crates/embersim-core/templates/`)
- `trace_log.h` / `trace_log.c`
- `mock_state.c`
- `mock_uart.c`
- `host_main.c`

### Verified Working
- [x] Day 2: Parser extracts 16 HAL functions from preprocessed headers
- [x] Day 3: Stub generator produces compilable mock_hal.h + mock_hal.c
- [x] Day 4: Stateful GPIO mocks with port-based lookup (weak override)
- [x] Day 5: Trace logger writes JSON Lines with timestamps
- [x] Day 6: Professional UART mock (blocking, IT, DMA, error injection, callbacks)
- [x] Day 6 Integration: `embersim init` generates full workspace from templates

### Architecture Decisions Locked In
1. **Weak symbol pattern**: Auto-generated stubs are `__attribute__((weak))`. Hand-written templates in `mock_state.c` / `mock_uart.c` provide strong overrides.
2. **Trace format**: JSON Lines (`trace.jsonl`), flat JSON objects, one per HAL call.
3. **Instance mapping**: Dummy base addresses on x86 (GPIOA=0x10000000, USART1=0x40011000).
4. **Tick-driven async**: `ember_sim_uart_tick()` must be called from host runner for IT/DMA completion.
5. **Template embedding**: C files embedded in Rust binary via `include_bytes!` in `templates.rs`.

---

## How to Resume in a New Session

Paste the following into your first message:

> I am continuing EmberSim, a host-native firmware test harness. Current state: Day 6 integration is complete. The Rust CLI can `init` a workspace with embedded C templates. Parser, stub generator, trace logger, GPIO mocks, and professional UART mock (blocking/IT/DMA/error injection) are all working. I need to continue to [Day 7 / Day 8 / specific feature]. My repo is at `C:\embersim\embersim`.

### Quick Verification Commands
```powershell
cd C:\embersim\embersim
cargo test --workspace
cargo run -p embersim-cli -- init --hal test-fixtures/minimal_hal.h -o test-embersim
cd test-embersim
gcc -I mocks -c mocks/mock_hal.c -o mocks/mock_hal.o
gcc -I mocks -c mocks/trace_log.c -o mocks/trace_log.o
gcc -I mocks -c mocks/mock_state.c -o mocks/mock_state.o
gcc -I mocks -c mocks/mock_uart.c -o mocks/mock_uart.o
```

---

## Next Steps (Choose One)

### Option A: Day 7 — I2C Virtual Device
Build a register-map-based I2C mock:
- `mock_i2c.c` with virtual device table (address → registers → bytes)
- `HAL_I2C_Master_Transmit` selects device + register
- `HAL_I2C_Master_Receive` returns pre-configured register data
- Test: firmware reads temperature sensor at 0x48, reg 0x00, gets [0x1A, 0x3B]

### Option B: Day 8 — CMake Build Pipeline
Automate the manual gcc commands:
- Generate `host_CMakeLists.txt` from `embersim.json` config
- `embersim build` command runs CMake
- `embersim test` runs the compiled host binary
- `embersim run` does init → build → test in one shot

### Option C: Day 9 — Project Config + Discovery
- `embersim.json` schema: source_files, exclude_files, hal_path
- Auto-discover STM32Cube project structure
- `embersim init` reads existing CubeMX project, generates config

---

## Critical Files to Preserve
Do NOT delete or modify without discussion:
1. `crates/embersim-core/src/parser.rs` — regex is finely tuned
2. `crates/embersim-core/templates/mock_uart.c` — professional mock with state machine
3. `crates/embersim-core/src/templates.rs` — paths must match template directory
4. `test-fixtures/minimal_hal.h` — used for CI/regression

---

## Known Issues / Technical Debt
1. `mock_uart.c` tick function stores handle references incompletely (commented section for ISR handle lookup)
2. `stubgen.rs` type definitions are hardcoded; needs dynamic generation from CMSIS headers
3. No `HAL_UART_STATE_*` constants in generated `mock_hal.h` for non-UART peripherals
4. Windows-only path separators in some template paths
5. `project.rs` does not yet validate `embersim.json`
