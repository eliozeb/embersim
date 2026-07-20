# EmberSim — Industry-Standard Roadmap

**Document type:** Architecture & Quality Enhancement Plan  
**Created:** 2026-07-19  
**Status:** Planning — not yet prioritized or estimated  

> This document is a companion to `EMBERSIM_PROJECT_STATE.md` (which tracks day-to-day progress) and `30 day plan.md` (which is the original 30-day execution plan). It looks beyond the 30-day launch toward what a mature, production-grade firmware test harness demands.

---

## 1. Current State Audit

### 1.1 What Exists and Works (Verified)

| Layer | Component | Status |
|---|---|---|
| **Rust CLI** | `embersim-cli` — 5 subcommands (generate, init, build, test, run) | Compiles clean; 5 unit tests pass |
| **Rust CLI** | `--auto` discovery mode (parses `.ioc` CubeMX files via quick-xml) | Implemented, untested in CI |
| **Core pipeline** | `preprocess.rs` — gcc/clang `-E` shell-out | Works when gcc/clang on PATH |
| **Core pipeline** | `clean.rs/normalize.rs` — strip preprocessor noise, attributes, keywords | Working |
| **Core pipeline** | `parser.rs` — regex extraction of `HAL_*` signatures with structured params | **5 unit tests passing** |
| **Core pipeline** | `pipeline.rs` — orchestration + dedup | Working |
| **Stub gen** | `stubgen.rs` — generates `mock_hal.h`/`mock_hal.c` with weak symbols | Working for GPIO + UART; I2C/SPI/TIM hardcoded |
| **Templates** | `templates.rs` — `include_bytes!` embedding of all C templates | Working |
| **Project** | `project.rs` — init/build/test orchestration + CMake rendering | Compiles; untested end-to-end |
| **Discovery** | `discovery.rs` — auto-detect CubeMX project structure | Implemented, untested |
| **C mocks — GPIO** | `mock_state.c` — port-indexed state table, WritePin/ReadPin/TogglePin | Verified working |
| **C mocks — UART** | `mock_uart.c` — blocking, IT, DMA, error injection, callbacks | Verified working |
| **C mocks — I2C** | `mock_i2c.c` — virtual device table, register files, tick-driven | **Written, not verified** |
| **C mocks — SPI** | `mock_spi.c`/`mock_spi.h` — virtual device callbacks, W25Q32 test | **Written, not verified** |
| **C mocks — TIM** | `mock_tim.c`/`mock_tim.h` — period/PWM/IC, tick-driven, register map | **Written, not verified** |
| **Sim kernel** | `ember_sim_kernel.c` — peripheral lifecycle, event queue, NVIC, bus | **Written, not verified** |
| **Register model** | `ember_regs.c` — generic register read/write with change tracking | **Written, not verified** |
| **Trace log** | `trace_log.c` — JSON Lines, append-only, per-call flush | Verified working |
| **Test fixtures** | `minimal_hal.h` — 16 HAL functions (GPIO + UART) | Used for regression |
| **Test files** | `test_uart_runtime.c`, `test_i2c.c`, `test_spi.c`, `test_tim_runtime.c` | **Written, not verified** |
| **CI** | `.github/workflows/rust.yml` — cargo build + test on push/PR | Present, untested in CI |

### 1.2 Technical Debt Catalog

| # | Issue | Severity | Location | Effort |
|---|---|---|---|---|
| TD1 | ISR handle lookup incomplete in `mock_uart.c` | Medium | `mock_uart.c` tick function | Small |
| TD2 | Type definitions hardcoded in `stubgen.rs`; no dynamic CMSIS extraction | High | `stubgen.rs` | Large |
| TD3 | Missing `HAL_UART_STATE_*` constants for non-UART peripherals | Low | `mock_hal.h` generation | Small |
| TD4 | Windows-only path separators in some template paths | Medium | `templates.rs` / `project.rs` | Small |
| TD5 | `project.rs` does not validate `embersim.json` schema | Medium | `project.rs` | Medium |
| TD6 | No test coverage for discovery, project, clean, preprocess, stubgen, templates | High | All untested modules | Large |
| TD7 | CMake build pipeline not end-to-end tested | High | `project.rs` | Medium |
| TD8 | I2C/SPI/TIM mocks compiled but never run against real test scenarios | High | C templates + test files | Medium |
| TD9 | GCC hard dependency — no MSVC or clang-cl fallback on Windows | Medium | `preprocess.rs` | Small |
| TD10 | No versioning in `embersim.json` schema | Medium | `model.rs` | Small |
| TD11 | Hardcoded STM32F4 family assumption throughout | High | Multiple files | Large |
| TD12 | Error messages lack structured diagnostics (no error codes, no fix hints) | Medium | All modules | Medium |
| TD13 | No `--version` flag that reports all dependency versions for bug reports | Low | `main.rs` | Small |
| TD14 | `ember_event_bus` appears duplicate/drifted from kernel bus system | Medium | Templates | Medium |

---

## 2. Architecture Enhancements

### 2.1 Multi-Architecture Peripheral Model

**Current:** Hardcoded for STM32F4xx. Dummy base addresses, register maps, and handle types are F4-specific.

**Target:** A peripheral-model registry driven by CMSIS-SVD files.

```
embersim-core/
├── src/
│   ├── arch/                    # NEW — architecture abstraction layer
│   │   ├── mod.rs               # Arch trait: register_width, base_addrs, irq_table
│   │   ├── stm32f4.rs           # F4-specific constants and SVD path
│   │   ├── stm32f7.rs
│   │   ├── stm32h7.rs
│   │   └── stm32g0.rs
│   ├── svd/                     # NEW — SVD parser module
│   │   ├── mod.rs               # Parse CMSIS-SVD XML → PeripheralModel
│   │   ├── parser.rs            # SVD XML → Rust structs
│   │   └── codegen.rs           # PeripheralModel → C header/shim generation
│   └── ...
```

**Key decisions:**
- **Trait `Arch`** defines the interface: `register_width()`, `base_addresses()`, `irq_count()`, `svd_path()`
- **SVD parsing** replaces hardcoded type definitions in `stubgen.rs` (fixes TD2)
- **Peripheral discovery** becomes data-driven: SVD says what registers exist → mocks auto-generate register maps
- **User provides** `--arch stm32f407` or it's auto-detected from `.ioc` file

### 2.2 Plugin Architecture for Peripheral Mocks

**Current:** Each peripheral mock is a C file embedded in `templates.rs`. Adding a new peripheral requires Rust recompilation.

**Target:** A plugin interface so community members can contribute mocks without touching the Rust core.

```
embersim-core/src/plugins/        # NEW
├── mod.rs                         # Plugin trait + registry
├── gpio.rs                        # GPIO mock plugin
├── uart.rs                        # UART mock plugin
├── i2c.rs                         # I2C mock plugin
└── loader.rs                      # External plugin discovery (.dll/.so)
```

**Key decisions:**
- **Trait `PeripheralPlugin`**: `name()`, `category()`, `c_template()`, `h_template()`, `required_types()`
- **Built-in plugins** compiled into the binary (GPIO, UART, I2C, SPI, TIM, ADC, DMA, RCC)
- **External plugins** via dynamic loading — enables proprietary/forked mocks
- **Plugin manifest** in `embersim.json`: `"plugins": ["gpio", "uart", "i2c"]`

### 2.3 Structured Diagnostics System

**Current:** Errors use `anyhow::bail!` with ad-hoc messages. No error codes, no structured data.

**Target:** A diagnostic system modeled after Rust's compiler diagnostics.

```rust
// embersim-core/src/diagnostics.rs  — NEW
pub struct Diagnostic {
    pub code: ErrorCode,           // e.g. "E001" = preprocessor not found
    pub severity: Severity,        // Error, Warning, Note, Help
    pub message: String,
    pub location: Option<Location>, // file:line:col
    pub fix_hint: Option<String>,   // "Install gcc via: pacman -S mingw-w64-ucrt-x86_64-gcc"
}

pub enum ErrorCode {
    PreprocessorNotFound,       // E001
    HeaderParseFailed,          // E002
    ConfigSchemaMismatch,       // E003
    BuildFailed { exit_code: i32, stderr: String }, // E004
    TestFailed { exit_code: i32 },                   // E005
    // ...
}
```

**Key decisions:**
- Every error gets a unique code → searchable docs
- `--explain E001` prints a detailed help page
- JSON output mode for IDE integration (`--diagnostics-format json`)

### 2.4 Configuration Schema Versioning

**Current:** `embersim.json` has no version field. Adding fields silently breaks old configs.

**Target:** Versioned config with migration support.

```json
{
  "schema_version": 2,
  "project": { ... },
  "arch": "stm32f407",
  "plugins": ["gpio", "uart", "i2c"],
  "mocks": { ... }
}
```

- `embersim migrate` auto-upgrades `embersim.json` from v1 → v2
- Schema validation rejects unknown fields with helpful errors
- JSON Schema published at `embersim.github.io/schema/v2.json`

### 2.5 Cross-Cutting: Interface Segregation

**Current:** `project.rs` mixes init, build, and test concerns. `stubgen.rs` mixes header generation with SPI/TIM/I2C hardcoding.

**Target:** Each module has a single responsibility:

| Module | Responsibility |
|---|---|
| `pipeline.rs` | Orchestrate preprocess→clean→parse |
| `stubgen.rs` | Generate weak stubs only — no hardcoded peripherals |
| `typegen.rs` (NEW) | Generate type definitions from arch/SVD |
| `project/init.rs` | Workspace initialization |
| `project/build.rs` | Build orchestration |
| `project/test.rs` | Test execution |
| `project/config.rs` | Config loading + validation |
| `diagnostics.rs` (NEW) | Structured error/warning system |

---

## 3. Feature Enhancements

### 3.1 Phase A — Core Reliability (now → 2 weeks)

#### A1. End-to-End Test Harness
- CI job that runs `embersim init` → `gcc` compile all mocks → link → run → assert trace output
- Golden-file tests: known HAL input → expected mock_hal.h output
- Property-based tests for parser: fuzz random C declarations, assert no panics

#### A2. Multi-Compiler Support
- Detect available compilers: gcc, clang, clang-cl (Windows)
- `embersim doctor` command: check environment, suggest fixes
- Preprocessor fallback chain: `gcc -E` → `clang -E` → `clang-cl /E`

#### A3. Schema Validation
- JSON Schema for `embersim.json`
- `embersim validate` command
- Inline validation on `init` and `build` with actionable error messages

#### A4. Version Command
- `embersim --version` → `embersim 0.1.0 (core: 0.1.0, rustc: 1.85.0, target: x86_64-pc-windows-msvc)`
- `embersim --version --verbose` → all dependency versions for bug reports

### 3.2 Phase B — Broader MCU Support (2 → 6 weeks)

#### B1. CMSIS-SVD Integration
- Parse `.svd` files (XML) into a `ChipModel` struct
- Auto-generate CMSIS shim headers from SVD (fixes TD2)
- Auto-discover peripheral registers from SVD for mock generation

#### B2. MCU Family Expansion
- STM32F7, STM32H7, STM32G0, STM32L4 from CubeMX
- Vendor extension point for non-ST MCUs (NXP, TI, Microchip)
- `embersim list-archs` to show supported targets

#### B3. CMSIS-Shim Generation
- Replace hand-crafted `GPIO_TypeDef` etc. with SVD-derived types
- Generate `stm32f407xx.h` shim with all peripheral base addresses
- IRQn enum auto-generated from SVD interrupt table

### 3.3 Phase C — Developer Experience (6 → 10 weeks)

#### C1. VS Code Extension
- `.vscode/embersim.json` launch config
- Status bar indicator: "EmberSim: running | 127 events | 0 failures"
- Inline trace annotations on source: `// → HAL_GPIO_WritePin(GPIOA, 5, HIGH) at t=1234ms`
- One-click "Run on EmberSim" button

#### C2. Interactive Trace Viewer
- WebSocket-based live trace streaming during host execution
- Browser dashboard: timeline, state inspector, register heatmap
- `embersim serve` — starts local server + opens browser
- Filter/search/zoom on trace events

#### C3. Watch Mode
- `embersim watch` — re-inits, rebuilds, and re-runs when source files change
- Uses `notify` crate for cross-platform file watching
- Terminal UI with pass/fail history (like `cargo watch`)

### 3.4 Phase D — Test Ecosystem (10 → 16 weeks)

#### D1. Assertion Framework
- C header `ember_assert.h` for in-test assertions
- `EMBER_ASSERT_EQ(actual, expected)` → logged to trace with pass/fail
- `EMBER_ASSERT_TIMING(op, min_us, max_us)` → validates execution time window
- `EMBER_ASSERT_IRQ_FIRED(IRQn, within_us)` → validates interrupt delivery
- Summary printed at end of run: `3/3 assertions passed`

#### D2. Test Runner Integration
- Generate Ceedling-compatible `project.yml`
- Generate Unity test runners from `embersim.json`
- `embersim test --framework unity` — auto-discovers and runs test cases

#### D3. Code Coverage
- `embersim build --coverage` → adds `--coverage` to gcc, generates `.gcda`/`.gcno`
- `embersim report --coverage` → renders coverage HTML alongside trace timeline
- Coverage gap analysis: which HAL functions were never called?

#### D4. Regression Guard
- `embersim snapshot` → saves current trace as golden reference
- `embersim test --regression` → compares current trace against golden
- Diff output: which HAL calls changed order, timing, or arguments

### 3.5 Phase E — Collaboration & CI (16 → 20 weeks)

#### E1. Docker Image
- `embersim/ci:latest` — Ubuntu + gcc + cmake + embersim preinstalled
- `embersim ci-init` → generates `.github/workflows/embersim.yml`
- `embersim ci-init --gitlab` → generates `.gitlab-ci.yml`

#### E2. GitHub Actions Summary
- Annotated diff: which tests passed/failed
- Trace timeline rendered as SVG in job summary
- PR comments with embersim results

#### E3. Team Configuration Sharing
- `embersim.lock` — pins plugin versions and arch definitions
- Check into git for reproducible CI runs
- `embersim install` — downloads and caches arch packs

### 3.6 Phase F — Advanced Simulation (20+ weeks)

#### F1. Multi-Peripheral Scenario Engine
- Scenario files (YAML): define sequences of peripheral events over time
  ```yaml
  # scenario/i2c_temp_read.yaml
  scenario:
    name: "Temperature sensor read loop"
    duration_ms: 5000
    events:
      - at_ms: 100
        i2c:
          I2C1:
            device: 0x48
            register: 0x00
            response: [0x1A, 0x3B]
      - at_ms: 200
        gpio:
          GPIOA:
            pin: 5
            set: HIGH
  ```
- `embersim run --scenario scenario/i2c_temp_read.yaml`
- Scenario validation: did all expected events fire?

#### F2. Fault Injection Framework
- Inject bus faults, clock glitches, peripheral errors
- `embersim run --fault-config fault.yaml` — configurable fault rates
- Chaos-mode: random error injection within safety bounds
- Validates firmware error-handling paths

#### F3. RTOS Simulation
- Mock FreeRTOS `xTaskCreate`, `vTaskDelay`, `xQueueSend` etc.
- Host-native task switching via cooperative yielding
- Trace shows RTOS context switches as separate lane

---

## 4. Quality & Infrastructure Enhancements

### 4.1 Testing Pyramid

```
                    ┌──────────┐
                    │   E2E    │  3-5 full scenarios: init→build→run→assert
                    ├──────────┤
                    │Integration│  10-15: pipeline stages, cmake build, gcc compile chain
                    ├──────────┤
                    │   Unit    │  50+: every Rust fn, every C mock function
                    └──────────┘
```

**Current state vs target:**

| Layer | Current | Target |
|---|---|---|
| Rust unit tests | 5 (parser only) | 50+ (every pub fn in every module) |
| Rust integration tests | 0 | 10+ (pipeline e2e, stubgen golden files, config round-trip) |
| C unit tests | 0 | 30+ (each HAL mock function with known inputs) |
| C integration tests | 4 test files (unverified) | 10+ (multi-peripheral scenarios, kernel scheduling) |
| E2E tests | 0 | 3+ (CI: init→build→run→assert output) |

### 4.2 Continuous Integration Matrix

```yaml
# .github/workflows/ci.yml — expanded
strategy:
  matrix:
    os: [ubuntu-latest, macos-latest, windows-latest]
    compiler: [gcc, clang]
    rust: [stable]
    exclude:
      - os: windows-latest
        compiler: clang  # clang-cl instead
```

Additional CI jobs:
- **Clippy** (deny warnings) — already configured in `.cargo/config.toml`
- **Rustfmt** check
- **C linter** (`cppcheck` or `clang-tidy` on mock sources)
- **Security audit** (`cargo audit` on dependencies)
- **Benchmark** regression (parser throughput, stubgen speed)
- **Coverage** (cargo-tarpaulin or grcov → Codecov)

### 4.3 Static Analysis & Linting

| Tool | Target | Status |
|---|---|---|
| clippy | Rust code | `-Dwarnings` configured |
| rustfmt | Rust code | Needs CI job |
| cppcheck | C templates | Not configured |
| clang-tidy | C templates | Not configured |
| shellcheck | CI scripts | Not configured |
| cargo-deny | Dependency licenses + advisories | Not configured |
| cargo-audit | Security vulnerabilities | Not configured |

### 4.4 Performance Benchmarks

Tracked in CI via `criterion`:

| Benchmark | What it measures | Why it matters |
|---|---|---|
| `parse_hal_header` | Time to parse a 50-function HAL header | Large projects have 100s of HAL functions |
| `stubgen_emit` | Time to generate mock_hal.h/c for N functions | CLI responsiveness |
| `preprocess_header` | GCC -E wall-clock time | Dominant cost on large projects |
| `init_workspace` | Full `init` command wall-clock | First-run experience |

### 4.5 Security

| Area | Action |
|---|---|
| Dependency auditing | `cargo audit` in CI, Dependabot for Rust |
| License compliance | `cargo deny` check — no GPL in permissive project |
| Pro license system | Ed25519 → review key handling, no hardcoded private keys |
| Generated C code | No code execution during parsing; preprocessor sandboxing |
| Template injection | Handlebars HTML escaping for report generation |
| Secret scanning | GitLeaks in CI to catch API keys in templates |

### 4.6 Documentation Suite

```
docs/
├── index.md                    # Landing page
├── getting-started.md          # "Your first host test in 5 minutes"
├── installation.md             # Per-OS: Windows, macOS, Linux
├── user-guide/
│   ├── embersim-json.md        # Full schema reference
│   ├── cli-reference.md        # Every command and flag
│   ├── mocks/
│   │   ├── gpio.md             # GPIO mock guide + examples
│   │   ├── uart.md
│   │   ├── i2c.md
│   │   ├── spi.md
│   │   └── tim.md
│   ├── scenarios.md            # Scenario file format
│   ├── assertions.md           # Assertion framework
│   └── ci-integration.md       # GitHub/GitLab CI setup
├── architecture/
│   ├── overview.md             # Pipeline, plugin system, kernel
│   ├── adding-an-arch.md       # How to add a new MCU family
│   └── writing-a-plugin.md     # Peripheral plugin guide
├── examples/
│   ├── blinky/                 # Complete CubeMX blinky project
│   ├── uart-echo/              # UART echo + test
│   ├── i2c-sensor/             # I2C temperature sensor
│   └── multi-peripheral/       # GPIO + UART + I2C + TIM together
└── contributing.md             # Dev setup, testing, PR process
```

### 4.7 Release Engineering

```
main branch
  │
  ├── v0.1.0  (MVP launch)
  ├── v0.2.0  (SVD integration)
  ├── v0.3.0  (Interactive trace viewer)
  ├── v0.4.0  (Test ecosystem)
  └── v1.0.0  (Industry standard)
```

- **Semantic versioning** from v0.1.0 forward
- **CHANGELOG.md** with [Keep a Changelog](https://keepachangelog.com/) format
- **GitHub Releases** with platform binaries (`.deb`, `.rpm`, `.msi`, `.pkg`, plain `.tar.gz`)
- **cargo-dist** or **cargo-binstall** for `cargo install embersim-cli`
- **Chocolatey** (Windows), **Homebrew** (macOS/Linux) packages
- **Canary channel**: every merge to `main` → `embersim-canary` binary

---

## 5. Phased Execution Roadmap

### Milestone 1: Stabilize & Verify (now → 2 weeks)

**Goal:** Everything that's written actually works and is tested.

| Task | Effort | Description |
|---|---|---|
| M1.1 | S | Write Rust tests for `clean.rs`, `preprocess.rs`, `model.rs`, `project.rs` |
| M1.2 | M | Verify I2C, SPI, TIM C mocks compile and link with test files |
| M1.3 | M | Run existing `test_uart_runtime.c`, `test_i2c.c`, `test_spi.c`, `test_tim_runtime.c` |
| M1.4 | S | Fix TD1 (ISR handle lookup), TD3 (missing state constants), TD4 (path separators) |
| M1.5 | S | Add `embersim doctor` command |
| M1.6 | S | Add `--version --verbose` |
| M1.7 | M | E2E CI test: init → gcc → link → run → assert trace output |
| **Exit criteria:** 50+ Rust tests passing, all C mocks compile, 1 E2E test in CI |

### Milestone 2: Hardened Foundation (2 → 6 weeks)

**Goal:** The tool works reliably across platforms and MCU families.

| Task | Effort | Description |
|---|---|---|
| M2.1 | L | Implement SVD parsing (`svd/parser.rs`) |
| M2.2 | M | Implement `arch` trait + STM32F4/F7/H7 implementations |
| M2.3 | L | Dynamic type generation from SVD (replace hardcoded types in stubgen) — fixes TD2 |
| M2.4 | M | Schema validation for `embersim.json` (JSON Schema + migration) |
| M2.5 | M | Structured diagnostics system (`diagnostics.rs`) |
| M2.6 | M | Multi-compiler detection; MSVC/clang-cl fallback on Windows |
| M2.7 | S | Fix TD5 (config validation), TD12 (error diagnostics), TD13 (version info) |
| **Exit criteria:** 2+ MCU families supported, SVD-driven type gen, validated config schema |

### Milestone 3: Developer Experience (6 → 10 weeks)

**Goal:** Developers choose EmberSim because it's faster and more informative than hardware.

| Task | Effort | Description |
|---|---|---|
| M3.1 | L | Interactive trace viewer (WebSocket + browser dashboard) |
| M3.2 | M | VS Code extension (launch config, status bar, inline annotations) |
| M3.3 | M | Watch mode (`embersim watch`) |
| M3.4 | M | Documentation suite (user guide, examples, architecture docs) |
| M3.5 | S | `embersim report --coverage` (gcov integration) |
| **Exit criteria:** Full docs published, VS Code extension on marketplace, trace dashboard functional |

### Milestone 4: Test Ecosystem (10 → 16 weeks)

**Goal:** EmberSim is a credible alternative to hardware-in-the-loop for firmware testing.

| Task | Effort | Description |
|---|---|---|
| M4.1 | M | Assertion framework (`ember_assert.h`) |
| M4.2 | M | Golden-file regression testing (`embersim snapshot` / `--regression`) |
| M4.3 | M | Ceedling/Unity integration |
| M4.4 | L | Scenario engine (YAML-driven multi-peripheral sequences) |
| M4.5 | M | Fault injection framework |
| **Exit criteria:** Scenario engine demo, assertion framework documented, regression guard in CI |

### Milestone 5: Ecosystem & Launch (16 → 20 weeks)

**Goal:** EmberSim integrates into every STM32 developer's workflow.

| Task | Effort | Description |
|---|---|---|
| M5.1 | M | Docker CI image + GitHub Actions template |
| M5.2 | M | Plugin system for community-contributed peripheral mocks |
| M5.3 | M | Release automation (cargo-dist, package managers) |
| M5.4 | S | `embersim.lock` for reproducible team configs |
| M5.5 | M | Landing page, blog, community forum |
| M5.6 | L | RTOS simulation (FreeRTOS mocks) |
| **Exit criteria:** Docker image published, 1 community plugin, automated releases to 3 package managers |

---

## 6. Architectural Principles

These govern all future design decisions:

1. **Zero-config startup, infinite configurability.** `embersim init --auto` should work on any CubeMX project with no manual setup. Every knob should have a sensible default.

2. **Don't emulate the CPU. Stub the HAL.** EmberSim runs firmware on the host CPU with mocked peripherals — not an ARM emulator. This keeps it fast and simple.

3. **Weak by default, strong on demand.** Auto-generated stubs are `__attribute__((weak))`. Users write strong overrides for the peripherals they care about.

4. **Trace everything. Render later.** Every HAL call, register write, and bus event goes to `trace.jsonl`. Reports, timelines, and assertions are post-processing on the trace.

5. **The kernel is optional.** Simple projects can use flat weak stubs. Complex projects opt into `ember_sim_kernel` for tick-driven simulation, NVIC, and event-based peripherals.

6. **Data-driven peripherals.** Peripheral behavior comes from data (SVD files, YAML scenarios, config), not code. Adding a new MCU should be a data contribution, not a code change.

7. **Diagnose, don't just fail.** Every error should answer: what happened, why, and how to fix it. Error codes are searchable.

8. **Cross-platform from day one.** CI runs on Windows, macOS, and Linux. No platform-specific code paths without an abstraction.

9. **Test the tests.** The assertion framework and regression guard are themselves tested. A broken test tool is worse than no tool.

10. **Open core, proprietary power.** The free tier is genuinely useful. Pro features (scenario engine, fault injection, advanced reports) add value for teams without crippling the free experience.

---

## 7. Key Metrics

| Metric | Current | Milestone 1 | Milestone 3 | Milestone 5 |
|---|---|---|---|---|
| Rust unit test count | 5 | 50+ | 100+ | 150+ |
| C mock test count | 0 | 4+ | 20+ | 40+ |
| E2E tests in CI | 0 | 1 | 3 | 5+ |
| MCU families supported | 1 (F4) | 1 (F4) | 4 (F4, F7, H7, G0) | 8+ |
| Test coverage (% lines) | ~5% | 40% | 65% | 80% |
| CI platforms | 0 working | 1 (ubuntu) | 3 (all) | 3 (all) |
| Time-to-first-test | ~10 min (manual) | ~5 min | ~2 min | ~1 min |
| Documentation pages | 1 (README) | 5 | 15+ | 25+ |
| Release channels | 0 | GitHub tag | + package managers | + canary channel |

---

*This document is a living plan. It should be reviewed and updated at each milestone boundary.*
