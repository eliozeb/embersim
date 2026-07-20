# Troubleshooting

Common errors, their causes, and how to fix them. Each entry comes from a real
failure encountered during EmberSim development and validation.

---

## Environment

### No C preprocessor found

```
[FAIL] No C preprocessor found (gcc or clang)
```

**Cause:** `embersim init` needs a C preprocessor to parse your HAL header.

**Fix:**
- **Windows:** Install [MSYS2](https://www.msys2.org/), then run `pacman -S mingw-w64-ucrt-x86_64-gcc`. Add `C:\msys64\ucrt64\bin` to your PATH.
- **macOS:** `xcode-select --install`
- **Linux:** `sudo apt install gcc`

### rustc not found

```
[FAIL] rustc not found on PATH
```

**Cause:** Rust is not installed.

**Fix:** `curl --proto '=https' --tlsv1.2 -sSf https://sh.rustup.rs | sh` (or https://rustup.rs on Windows). Restart your shell.

### embersim: command not found

**Cause:** The EmberSim CLI is not installed.

**Fix:** `cargo install --git https://github.com/eliozeb/embersim embersim-cli`

---

## Init

### Header not found

```
Error: Header not found: Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h
```

**Cause:** The path passed to `-f` doesn't exist or is wrong.

**Fix:**
1. Verify the path: `ls Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h`
2. Find your HAL header: `find . -name "stm32*_hal.h"`
3. Use the correct path. Remember it's relative to your project root.

### Missing -f <HAL header>

```
Error: Missing -f <HAL header>. Use --auto for automatic detection.
```

**Cause:** `embersim init` was called without `-f` and without `--auto`.

**Fix:** Either provide `-f <path>` or use `--auto` for automatic detection.

### Preprocessor failed on header

**Cause:** The HAL header includes files that can't be found with the given `-I` paths.

**Fix:** Add `-I` for every directory needed to resolve the header's `#include` directives. If your HAL header includes `stm32f4xx_hal_conf.h` from `Core/Inc`, you need `-I Core/Inc`.

---

## Check

### Mocks not found

```
Error: Mocks not found at ember_sim/mocks. Run 'embersim init' first.
```

**Cause:** `embersim check` was run before `embersim init`.

**Fix:** Run `embersim init` first, then `embersim check`.

### HAL type MISSING

```
HAL types:
  TIM_HandleTypeDef                      MISSING
    → Replace '#include "stm32f4xx_hal.h"' with '#include "mock_hal.h"'
```

**Cause:** The type is used by your firmware but not found in the configured include paths.

**Fix:** Usually solved by including the right mock header. For `TIM_HandleTypeDef`, add `#include "mock_tim.h"` to your firmware source. See [HAL_BOUNDARY.md](HAL_BOUNDARY.md).

### CMSIS macro MISSING

```
CMSIS macros:
  __disable_irq                          MISSING
```

**Cause:** Your firmware uses CMSIS intrinsics that have no simulation equivalent.

**Fix:** Run `embersim init --repair`. This generates `ember_sim/mocks/cmsis_shim.h` with stub macros. Add `#include "cmsis_shim.h"` to your firmware.

---

## Run

### No embersim.toml found

```
Error: No embersim.toml found. Create one or run 'embersim init' first.
```

**Cause:** `embersim run` looks for `embersim.toml` in the current directory (your project root), not inside `ember_sim/`.

**Fix:** Make sure `embersim.toml` exists at your project root:
```bash
cp ember_sim/embersim.toml ./embersim.toml
```
Then edit it with your project's source files and include paths.

### No source files found

```
Error: No source files found. Specify with -s or add [build.sources] to embersim.toml.
```

**Cause:** The `[build.sources]` list in `embersim.toml` is empty or contains paths that don't exist.

**Fix:** Add your firmware `.c` files to `[build.sources]`. Make sure to include `ember_sim/host_main.c`. Paths are relative to the project root.

### Compilation warnings

```
ember_sim/mocks/mock_spi.c:155:75: warning: '%s' directive output may be truncated
```

**Cause:** Format string warnings in generated mock code. These are harmless — the mocks are generated for a different target and some format truncation warnings appear when compiled for x86.

**Fix:** No action needed. These warnings don't affect execution or trace output. If they bother you, add `-Wno-format-truncation` to your gcc flags.

---

## Baseline

### Trace file not found

```
Error: Trace file not found: trace.jsonl
Run firmware first to generate a trace.
```

**Cause:** `embersim baseline create` can't find the trace file.

**Fix:** Run `embersim run` first. It generates `trace.jsonl` (or the path specified in `embersim.toml` `[simulation].trace`).

### No baseline found

```
No baseline found. Run 'embersim baseline create' to set one.
```

**Cause:** `embersim run` or `embersim compare` can't find `baseline/trace.jsonl`.

**Fix:** Run `embersim baseline create -t trace.jsonl` to create the baseline, then commit `baseline/trace.jsonl`.

---

## CI

### could not find Cargo.toml

```
error: could not find Cargo.toml in /home/runner/work/<project>
```

**Cause (historical):** The CI workflow used `cargo build -p embersim-cli` from the firmware repo root, which has no Cargo workspace. **This is fixed in the current ci-init template** — the workflow now checks out EmberSim into `_embersim/` and uses `--manifest-path`.

**If you still see this:** Your workflow was generated by an older version of `embersim ci-init`. Re-run `embersim ci-init` to regenerate it.

### Golden trace not found (CI)

```
Error: Golden trace not found: baseline/trace.jsonl
```

**Cause:** The baseline file is missing from the repository or the CI checkout.

**Fix:**
1. Verify `baseline/trace.jsonl` is committed: `git ls-files baseline/trace.jsonl`
2. Verify it was pushed: check your repository on GitHub
3. If missing locally: `embersim baseline create -t trace.jsonl`, commit, push

### Workflow not triggering

**Cause:** The push/pull_request trigger isn't matching.

**Fix:** The workflow triggers on pushes to `main` or `master`, and on pull requests targeting those branches. Pushes to other branches don't trigger the push event — open a PR instead.

### HAL_HEADER variable not set

**Cause:** The repository variable wasn't configured.

**Fix:** GitHub → Settings → Secrets and variables → Actions → Variables → check that `HAL_HEADER` exists. The value should match the `-f` argument you use locally.

---

## Regression

### Result: FAIL — firmware regression detected

```
Result: FAIL — firmware regression detected

First divergence:
  line: 42

Expected:
  HAL_UART_Transmit

Observed:
  REGISTER_EVENT

Evidence: strong
```

**Cause:** The firmware trace differs from the golden baseline. This is EmberSim working correctly — it detected a behavioral change.

**Fix:** Two options:
1. **The change was intentional** — update the baseline: `embersim baseline create -t trace.jsonl`, commit, push.
2. **The change was accidental** — investigate the divergence. The "Expected" and "Observed" lines show what changed. Revert the change.

### Understanding divergence output

- **line:** The trace line number where the first difference appears
- **Expected:** The function name in the golden baseline
- **Observed:** The function name in the current run
- **Evidence:** `strong` if both a missing function and a different function were found

The divergence always reports the **first** difference. There may be more differences after it. Fix the first one, re-run, and check again.

### Evidence: strong — what this means

"Strong" means the divergence was detected in two independent ways (e.g., a function was removed AND a different function appeared at the same position). "Weak" means only one signal was found. Both indicate a real regression — the strength just indicates how confident the detection is.
