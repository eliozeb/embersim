# Stage 7 Readiness Report — Pilot Preparation Audit

**Date:** 2026-07-20
**Status:** Assessment complete — 4 blockers, 5 documentation gaps, 3 UX gaps identified

---

## 1. Documentation Audit

### Existing docs

| Document | Type | Quality | Pilot-ready? |
|----------|------|---------|-------------|
| `README.md` | Project overview | Minimal — 4 bullet points, no setup, no quickstart | ❌ |
| `docs/GETTING_STARTED_REAL_PROJECT.md` | Onboarding guide | Good content but stale (pre-S6.4 CI, missing ci-init) | ⚠️ Needs update |
| `docs/WHY_EMBERSIM.md` | Motivation/pitch | Unknown | Needs review |
| `docs/ARCHITECTURE_EVIDENCE.md` | Internal reference | Good | Not pilot-facing |
| `docs/S6.4_CLOSURE_REPORT.md` | Validation evidence | Complete | Useful for credibility |
| `docs/STAGE_7_PILOT_PLAN.md` | Planning | Complete | Internal only |
| `embersim-ci-validation/README.md` | Fixture reference | Good | Useful as example |

### Required pilot docs (from Stage 7 plan)

| Document | Exists? | Closest existing | Gap |
|----------|---------|-----------------|-----|
| `GETTING_STARTED.md` | ❌ | `docs/GETTING_STARTED_REAL_PROJECT.md` (stale) | Step-by-step from zero to green CI with current workflow |
| `HAL_BOUNDARY.md` | ❌ | None | How to replace HAL includes, why mock_hal.h exists, how mock_tim.h etc. fit in |
| `CI_SETUP.md` | ❌ | `embersim-ci-validation/README.md` (partial) | How ci-init works, what variables to set, how EmberSim is acquired, what each workflow step does |
| `TROUBLESHOOTING.md` | ❌ | `docs/GETTING_STARTED_REAL_PROJECT.md` Section "What to expect" (4 items) | Structured error→cause→fix format covering: preprocessor not found, header not found, check issues, baseline missing, CI variable not set, trace divergence |

### Other documentation issues

| Issue | Severity |
|-------|----------|
| Root directory has 8+ historical `.md` files (plans, feedback, synthesis) — confusing for newcomers | Low |
| `README.md` is essentially empty — first thing a pilot sees | **Blocker** |
| `docs/GETTING_STARTED_REAL_PROJECT.md` references `cargo install embersim-cli` (not the S6.4 checkout workflow) | Medium |
| No architecture overview diagram showing kernel→peripheral→HAL flow | Medium |
| `embersim-ci-validation/README.md` uses `_embersim` path — needs to be consistent with any main docs | Low |

---

## 2. CLI Onboarding Experience Audit

### Fresh-user path tested

| Step | Command | Status | UX issue |
|------|---------|--------|----------|
| 0 | `embersim --help` | ✅ | `generate` and `init` have no description text (unlike `check`, `run`, etc.) |
| 0 | `embersim --version` | ✅ | No `--verbose` extra info |
| 1 | `embersim doctor` | ✅ | Clear, actionable. OS-specific install instructions. |
| 2 | `embersim init` (no args) | ✅ | Clear: "Missing -f <HAL header>. Use --auto" |
| 2 | `embersim init --help` | ⚠️ | No description. `-f`, `-I`, `-D` flags have no help text explaining what they accept |
| 2 | `embersim init -f <hal>` | ❌ | Fails if no preprocessor on PATH. Error message is good but the requirement isn't documented in `--help` |
| 3 | `embersim check` (no workspace) | ✅ | Clear: "Mocks not found. Run 'embersim init' first." |
| 4 | `embersim run` (no config) | ✅ | Clear: "No embersim.toml found." |
| 5 | `embersim baseline create` (no trace) | ✅ | Clear: "Trace file not found. Run firmware first." |
| 6 | `embersim ci-init` (exists) | ✅ | Clear: "Already exists. Remove first or edit manually." |
| 7 | `embersim compare` (no args) | ✅ | Clap gives good required-args error |

### UX gaps

| # | Issue | Severity | Fix |
|---|-------|----------|-----|
| 1 | `init --help` has no flag descriptions — user doesn't know what `-f` expects or what `-I` does | **Blocker** | Add clap doc comments to `-f`, `-I`, `-D`, `-a`, `--repair` |
| 2 | `--help` top-level: `generate` and `init` have no about text | Medium | Add `about = "..."` to clap attributes |
| 3 | Preprocessor requirement is undocumented in `--help` / `init --help` | High | Add to `init --help` description |
| 4 | `embersim init` output says "Next step: embersim check" but doesn't explain what check does or why | Low | Add brief "why" to next-step guidance |
| 5 | No command to show current project state (`embersim status` or similar) | Low | Future consideration |
| 6 | `embersim run` output doesn't show the baseline path it's looking for | Low | Print "Looking for baseline at baseline/trace.jsonl" before comparison |
| 7 | `embersim` (no subcommand) shows help — good, but doesn't suggest starting with `doctor` | Low | Add "Run 'embersim doctor' first to check your environment." |

### Step count: zero → green CI

Current: **13 steps**

```
1. Install Rust
2. Install gcc
3. embersim doctor
4. embersim init -f <hal> -I <inc> ...
5. embersim check
6. Fix check issues (manual)
7. embersim run
8. embersim baseline create
9. Commit baseline/
10. embersim ci-init
11. Commit workflow
12. Set HAL_HEADER in GitHub
13. Push
```

Pilot plan target: 8 steps. **Gap: 5 steps.** The biggest contributors are manual git operations (steps 9, 11, 12). These could potentially be consolidated or automated, but that's a Stage 8 concern.

---

## 3. Fixture as First-Time-User Reference

### Assessment: "Could a new STM32 engineer understand..."

| Question | Answer | Issue |
|----------|--------|-------|
| Where does HAL_HEADER come from? | ⚠️ Partially | The README says `firmware/stm32f4xx_hal.h` but doesn't explain this is a minimal fixture — a real project would use its actual CubeMX HAL path |
| Why does mock_hal.h exist? | ❌ | No explanation of the HAL boundary concept — that simulation replaces hardware HAL calls with mock implementations. A newcomer might think mock_hal.h is something they need to write. |
| Where does baseline/trace.jsonl come from? | ✅ | README Section "Setup" step 2 shows the commands |
| How does CI get EmberSim? | ✅ | README "How It Works" explains the checkout step |
| What is the relationship between ember_sim/, _embersim/, baseline/? | ❌ | Three directories with similar names but different purposes — no glossary or directory map |
| Why does CI re-run embersim init every time? | ❌ | Not explained. Might seem wasteful. |
| What is a "golden baseline"? | ❌ | Term used but not defined |
| What happens when CI detects a regression? | ❌ | The README says "FAIL — firmware regression detected" but doesn't show the full output or explain how to investigate |

### Fixture-specific gaps

| # | Issue | Severity |
|---|-------|----------|
| 1 | `stm32f4xx_hal.h` is a 29-line minimal fixture — a newcomer might think they need to write this | Medium |
| 2 | No explanation of why `app_init()`/`app_run()` exist instead of `main()` | High |
| 3 | No glossary defining: mock, stub, baseline, trace, golden, workspace, HAL boundary | Medium |
| 4 | No annotated example showing the trace → baseline → regression cycle with real output | Medium |

---

## 4. Pilot Blockers

These must be resolved before the first pilot begins:

| # | Blocker | Category | Fix |
|---|---------|----------|-----|
| B1 | `README.md` is essentially empty | Documentation | Write a proper README with: what EmberSim is, quickstart (5 lines), link to full docs |
| B2 | Zero of 4 required pilot docs exist | Documentation | Write GETTING_STARTED.md, HAL_BOUNDARY.md, CI_SETUP.md, TROUBLESHOOTING.md |
| B3 | `init --help` has no flag descriptions | CLI UX | Add clap `help = "..."` for -f, -I, -D, -a, --repair |
| B4 | No one has tested the full pilot onboarding path with a real STM32 project (not a fixture) | Validation | Dry-run before pilot starts |

---

## 5. Recommended Priority Order

### Week 1 (before pilot begins)

| Priority | Action | Effort | Blocks |
|----------|--------|--------|--------|
| **P0** | Write `README.md` quickstart section | 2 hours | B1 |
| **P0** | Write `GETTING_STARTED.md` (update + expand existing) | 4 hours | B2 |
| **P0** | Write `HAL_BOUNDARY.md` | 3 hours | B2 |
| **P0** | Write `CI_SETUP.md` | 2 hours | B2 |
| **P0** | Write `TROUBLESHOOTING.md` | 3 hours | B2 |
| **P0** | Add `help` text to `init` flags in clap | 30 min | B3 |
| **P1** | Add `about` text to `generate` and `init` subcommands | 15 min | UX gap #2 |
| **P1** | Add preprocessor requirement to `init --help` | 15 min | UX gap #3 |
| **P1** | Dry-run onboarding with a real STM32 project | 4 hours | B4 |

### Week 2 (pilot begins)

| Priority | Action | Effort |
|----------|--------|--------|
| **P1** | Guided onboarding session with pilot | 1 hour |
| **P2** | Collect friction log daily | 15 min/day |
| **P2** | Mid-pilot survey | 30 min |

### Deferred (not blockers)

| Action | Why deferred |
|--------|-------------|
| `--verbose` version info | Nice-to-have |
| `embersim status` command | New feature |
| Architecture diagram | Important but not blocking |
| Glossary in fixture README | Nice-to-have |
| Consolidate root-level .md files | Cosmetic |
| Reduce step count (13 → 8) | Requires CLI changes — Stage 8 |

---

## 6. Architecture Verification

During the audit, confirmed zero changes to frozen areas:

```
Kernel          — untouched
Scheduler       — untouched
Trace engine    — untouched
Peripherals     — untouched
Event bus       — untouched
NVIC dispatch   — untouched
Register model  — untouched
Plugin API      — untouched
```

15/15 tests passing. No regressions.

---

## 7. Go/No-Go for Stage 7 Pilot

### Go (proceed to pilot onboarding)

- [ ] B1: README.md has quickstart
- [ ] B2: Four pilot docs written
- [ ] B3: `init --help` has flag descriptions
- [ ] B4: Dry-run completed with real STM32 project
- [ ] 15/15 tests passing
- [ ] Pilot candidate identified and confirmed

### Current status: **NOT READY** — 4 blockers unresolved

**Estimated effort to clear blockers:** ~15 hours of documentation work, ~1 hour of CLI work, ~4 hours of dry-run validation.

Recommendation: **Week 1 focus on docs.** No code except CLI help text additions. Dry-run at end of week. If dry-run succeeds, start pilot Week 2.
