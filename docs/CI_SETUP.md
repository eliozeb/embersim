# CI Setup Guide

How `embersim ci-init` works, what it generates, and how to configure your
repository for GitHub Actions.

## What ci-init does

```bash
embersim ci-init
```

Creates `.github/workflows/embersim.yml` — a complete CI workflow that:

1. Checks out your firmware repository
2. Installs Rust and gcc
3. Checks out EmberSim from `eliozeb/embersim` into `_embersim/`
4. Caches Rust dependencies for faster subsequent runs
5. Builds the EmberSim CLI
6. Runs `embersim init` to generate the simulation workspace
7. Runs `embersim check` to validate HAL coverage
8. Runs `embersim run` to build and execute your firmware
9. Runs `embersim compare` to detect regressions against the baseline

The workflow triggers on every push and pull request to `main` or `master`.

## Repository layout after setup

```
your-firmware-repo/
├── .github/workflows/embersim.yml   ← generated CI workflow (commit)
├── baseline/trace.jsonl              ← golden baseline (commit)
├── embersim.toml                     ← project configuration (commit)
├── ember_sim/                        ← generated workspace (do NOT commit — .gitignored)
├── _embersim/                        ← CI EmberSim checkout (do NOT commit — .gitignored)
└── your firmware source files...
```

## Required variable: HAL_HEADER

Set one repository variable so the CI workflow knows where your HAL header is:

| Field | Value |
|-------|-------|
| **Name** | `HAL_HEADER` |
| **Value** | Path to your HAL master header, e.g. `Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h` |

The value must match the `-f` argument you passed to `embersim init` locally.

**Where to set it:**

1. Open your repository on GitHub
2. Settings → Secrets and variables → Actions → Variables
3. New repository variable
4. Name: `HAL_HEADER`
5. Value: your HAL header path
6. Add variable

## Optional variable: EMBERSIM_REPO

If you're using the official EmberSim repository (`eliozeb/embersim`), you don't
need to set this — it's the default.

To use a fork (e.g. for a customized or private EmberSim instance):

| Field | Value |
|-------|-------|
| **Name** | `EMBERSIM_REPO` |
| **Value** | `your-org/embersim` |

### Private EmberSim repositories

If your EmberSim fork is private, you'll also need to grant CI access. The
easiest way is a [deploy key](https://docs.github.com/en/authentication/connecting-to-github-with-ssh/managing-deploy-keys):

1. Generate an SSH key pair
2. Add the public key to your EmberSim fork (Settings → Deploy keys)
3. Add the private key as a repository secret named `EMBERSIM_DEPLOY_KEY`
4. Add a step before "Checkout EmberSim" in the workflow to configure SSH

This is an advanced setup — the default public `eliozeb/embersim` works for
most users.

## What each workflow step does

| Step | Command | Purpose |
|------|---------|---------|
| Checkout repository | `actions/checkout@v4` | Gets your firmware code |
| Install Rust toolchain | `dtolnay/rust-toolchain@stable` | Provides `cargo` and `rustc` |
| Install gcc | `apt-get install -y gcc` | Compiles your firmware for x86 |
| Checkout EmberSim | `actions/checkout@v4` into `_embersim/` | Gets the EmberSim CLI source |
| Cache Rust dependencies | `Swatinem/rust-cache@v2` | Speeds up subsequent runs |
| Build EmberSim CLI | `cargo build --manifest-path _embersim/Cargo.toml` | Compiles the CLI |
| Initialize workspace | `embersim init -f $HAL_HEADER ...` | Generates mocks from your HAL |
| Check readiness | `embersim check` | Validates HAL coverage |
| Run regression | `embersim run` | Builds firmware, executes, generates trace |
| Compare baseline | `embersim compare -g baseline/trace.jsonl -c trace.jsonl` | Detects regressions |

## First run verification

After pushing, open Actions on your repository. The first run takes 2–3 minutes
(cold Rust cache). Subsequent runs are faster (~30 seconds for the Rust build,
~30 seconds for firmware compilation and execution).

All 10 steps should show ✓. If any step fails, see [TROUBLESHOOTING.md](TROUBLESHOOTING.md).

## Baseline workflow

The golden baseline at `baseline/trace.jsonl` is your known-good reference.
The workflow:

```
Push to main
    │
    ▼
embersim run → generates trace.jsonl
    │
    ▼
embersim compare -g baseline/trace.jsonl -c trace.jsonl
    │
    ├── match    → ✅ PASS, job succeeds
    └── mismatch → ❌ FAIL, job fails with divergence details
```

### Updating the baseline

When you intentionally change firmware behavior:

```bash
embersim run -o ember_sim
embersim baseline create -t trace.jsonl
git add baseline/trace.jsonl
git commit -m "Update golden baseline for <reason>"
git push
```

Always include a reason in the commit message. Future you (and your team) will
need to distinguish "intentional behavior change" from "someone accidentally
overwrote the baseline."

## Local CI simulation

You can run the same sequence locally that CI runs:

```bash
embersim init -f <hal.h> -I <inc> -o ember_sim
embersim check -o ember_sim
embersim run -o ember_sim
embersim compare -g baseline/trace.jsonl -c trace.jsonl
```

This is useful for debugging CI failures without pushing.
