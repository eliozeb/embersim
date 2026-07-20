use anyhow::{Context, Result};
use clap::{Parser, Subcommand};
use std::path::PathBuf;

#[derive(Parser)]
#[command(name = "embersim", about = "Host-native firmware test harness", version = "0.1.0")]
struct Cli {
    #[command(subcommand)]
    command: Commands,
}

#[derive(Subcommand)]
enum Commands {
    Generate {
        #[arg(short = 'f', long, value_name = "FILE")]
        hal: PathBuf,
        #[arg(short = 'I', long = "include", value_name = "DIR")]
        include: Vec<String>,
        #[arg(short = 'D', long = "define", value_name = "MACRO")]
        define: Vec<String>,
        #[arg(short = 'o', long, value_name = "DIR", default_value = "ember_sim/mocks")]
        output: PathBuf,
    },
    Init {
        #[arg(short = 'f', long, value_name = "FILE")]
        hal: Option<PathBuf>,  // optional when --auto is used
        #[arg(short = 'I', long = "include", value_name = "DIR")]
        include: Vec<String>,
        #[arg(short = 'D', long = "define", value_name = "MACRO")]
        define: Vec<String>,
        #[arg(short = 'o', long, value_name = "DIR", default_value = "ember_sim")]
        output: PathBuf,
        #[arg(long = "auto", short = 'a', action = clap::ArgAction::SetTrue)]
        auto: bool,
        #[arg(long = "repair", action = clap::ArgAction::SetTrue)]
        repair: bool,
    },
    /// Build the host binary from embersim.json configuration
    Build {
        #[arg(short = 'c', long, value_name = "FILE", default_value = "embersim.json")]
        config: PathBuf,
        #[arg(short = 'o', long, value_name = "DIR", default_value = "ember_sim")]
        output: PathBuf,
        #[arg(short = 'p', long, default_value = "debug")]
        profile: String,
    },
    /// Run the host binary and collect trace (legacy, use 'embersim test' for CI workflow)
    RunTrace {
        #[arg(short = 'c', long, value_name = "FILE", default_value = "embersim.json")]
        config: PathBuf,
        #[arg(short = 'o', long, value_name = "DIR", default_value = "ember_sim")]
        output: PathBuf,
        #[arg(short = 't', long, value_name = "FILE", default_value = "ember_sim/out/trace.jsonl")]
        trace: PathBuf,
    },
    /// Compile, execute, and compare against baseline (CI-ready workflow)
    Run {
        #[arg(short = 'o', long, value_name = "DIR", default_value = "ember_sim")]
        output: PathBuf,
        #[arg(short = 's', long, value_name = "FILE")]
        source: Option<PathBuf>,
    },
    /// Check the development environment and report issues
    Doctor,
    /// Compare two trace files and report the first behavioural divergence
    Compare {
        #[arg(short = 'g', long, value_name = "FILE")]
        golden: PathBuf,
        #[arg(short = 'c', long, value_name = "FILE")]
        candidate: PathBuf,
    },
    /// Save the current trace as the regression baseline
    Baseline {
        #[command(subcommand)]
        action: BaselineAction,
    },
    /// Analyze project readiness — report missing HAL symbols, macros, and includes
    Check {
        #[arg(short = 'o', long, value_name = "DIR", default_value = "ember_sim")]
        output: PathBuf,
    },
    /// Build, execute, and compare against the regression baseline
    Test {
        #[arg(short = 'c', long, value_name = "FILE", default_value = "embersim.json")]
        config: PathBuf,
        #[arg(short = 'o', long, value_name = "DIR", default_value = "ember_sim")]
        output: PathBuf,
    },
}

#[derive(Subcommand)]
enum BaselineAction {
    /// Create a new baseline from the current firmware execution
    Create {
        #[arg(short = 't', long, value_name = "FILE", default_value = "trace.jsonl")]
        trace: PathBuf,
    },
}

fn main() -> Result<()> {
    let cli = Cli::parse();
    match cli.command {
        Commands::Generate { hal, include, define, output } => {
            if !hal.exists() {
                anyhow::bail!("Header not found: {}", hal.display());
            }
            let functions = embersim_core::pipeline::run(&hal, &include, &define)?;
            embersim_core::stubgen::generate(&functions, &output)?;
            eprintln!("Generated mocks in {}", output.display());
        }
        Commands::Init { hal, include, define, output, auto, repair } => {
            if repair {
                if !output.join("mocks").exists() {
                    anyhow::bail!(
                        "No simulation workspace found at {}. Run 'embersim init' first.",
                        output.display()
                    );
                }
                println!("Repairing project environment...\n");
                let fixes = embersim_core::project::repair_project(&output)?;
                if fixes.is_empty() {
                    println!("No issues found. Project is ready.");
                } else {
                    for fix in &fixes {
                        println!("  ✓ {}", fix);
                    }
                    println!("\nRun 'embersim check' to verify, then 'embersim run'.");
                }
            } else if auto {
                let cwd = std::env::current_dir()?;
                let config = embersim_core::discovery::discover(&cwd)?;

                // Write the auto‑detected config so the user can see / edit it
                let json = serde_json::to_string_pretty(&config)?;
                std::fs::write("embersim.json", &json)?;
                eprintln!("Auto‑detected project structure written to embersim.json");

                // Determine HAL header from the discovered config
                let hal_dir = PathBuf::from(&config.project.hal_path);
                let candidates = [
                    "stm32f4xx_hal.h", "stm32f7xx_hal.h", "stm32g0xx_hal.h",
                    "stm32h7xx_hal.h", "stm32l4xx_hal.h", "stm32wbxx_hal.h",
                ];
                let hal_header = candidates.iter()
                    .find_map(|name| {
                        let p = hal_dir.join(name);
                        if p.exists() { Some(p) } else { None }
                    })
                    .ok_or_else(|| anyhow::anyhow!(
                        "Could not find any HAL master header in {}. Please specify -f manually.",
                        hal_dir.display()
                    ))?;

                // Merge provided includes with guessed ones
                let mut includes = include.clone();
                // Add typical CubeMX include dirs (relative to project root)
                for dir in &["Core/Inc", "Drivers/CMSIS/Device/ST/STM32F4xx/Include", "Drivers/CMSIS/Include"] {
                    includes.push(dir.to_string());
                }

                let functions = embersim_core::pipeline::run(&hal_header, &includes, &define)?;
                embersim_core::project::initialize(&functions, &output)?;
            } else {
                let hal = hal.ok_or_else(|| anyhow::anyhow!("Missing -f <HAL header>. Use --auto for automatic detection."))?;
                if !hal.exists() {
                    anyhow::bail!("Header not found: {}", hal.display());
                }
                let functions = embersim_core::pipeline::run(&hal, &include, &define)?;
                embersim_core::project::initialize(&functions, &output)?;
            }
        }
        Commands::Build { config, output, profile } => {
            let cfg = embersim_core::project::load_config(&config)?;
            if !output.join("mocks").exists() {
                anyhow::bail!(
                    "Workspace not initialized in {}. Run 'embersim init' first.",
                    output.display()
                );
            }
            embersim_core::project::build_host(&cfg, &output, &profile)?;
        }
        Commands::RunTrace { config, output, trace } => {
            let cfg = embersim_core::project::load_config(&config)?;
            embersim_core::project::run_tests(&cfg, &output, &trace)?;
        }
        Commands::Run { output, source } => {
            // Read project config
            let config_path = PathBuf::from("embersim.toml");
            if !config_path.exists() {
                anyhow::bail!(
                    "No embersim.toml found. Create one or run 'embersim init' first."
                );
            }
            let cfg = embersim_core::project::load_toml_config(&config_path)?;
            let mocks_dir = output.join("mocks");

            // Find source files
            let sources: Vec<PathBuf> = if let Some(s) = &source {
                vec![s.clone()]
            } else {
                let mut found = Vec::new();
                for pattern in &cfg.build.sources {
                    // Simple glob: *.c in current directory
                    if pattern == "*.c" {
                        if let Ok(entries) = std::fs::read_dir(".") {
                            for e in entries.flatten() {
                                let p = e.path();
                                if p.extension().map_or(false, |ext| ext == "c") {
                                    found.push(p);
                                }
                            }
                        }
                    } else if let Ok(meta) = std::fs::metadata(pattern) {
                        if meta.is_file() {
                            found.push(PathBuf::from(pattern));
                        }
                    }
                }
                // Fallback: scan for *.c in current directory
                if found.is_empty() {
                    if let Ok(entries) = std::fs::read_dir(".") {
                        for e in entries.flatten() {
                            let p = e.path();
                            if p.extension().map_or(false, |ext| ext == "c") {
                                found.push(p);
                            }
                        }
                    }
                }
                found
            };

            if sources.is_empty() {
                anyhow::bail!("No source files found. Specify with -s or add [build.sources] to embersim.toml.");
            }

            // Compile mock .c files
            let mut mock_objects = Vec::new();
            if let Ok(entries) = std::fs::read_dir(&mocks_dir) {
                for entry in entries.flatten() {
                    let p = entry.path();
                    if p.extension().map_or(false, |e| e == "c") {
                        let obj = p.with_extension("o");
                        let status = std::process::Command::new("gcc")
                            .args(["-I", &mocks_dir.to_string_lossy()])
                            .args(["-c", &p.to_string_lossy()])
                            .args(["-o", &obj.to_string_lossy()])
                            .status()?;
                        if status.success() {
                            mock_objects.push(obj);
                        }
                    }
                }
            }

            // Compile each source
            println!("Building firmware...");
            let mut objects = Vec::new();
            for src in &sources {
                let obj = src.with_extension("o");
                let mut gcc = std::process::Command::new("gcc");
                gcc.arg("-I").arg(&mocks_dir);
                for inc in &cfg.build.includes {
                    gcc.arg("-I").arg(inc);
                }
                let status = gcc
                    .args(["-c", &src.to_string_lossy()])
                    .args(["-o", &obj.to_string_lossy()])
                    .status()
                    .with_context(|| format!("gcc compile failed for {}", src.display()))?;
                if !status.success() {
                    anyhow::bail!("Compilation failed: {}", src.display());
                }
                objects.push(obj);
            }

            // Link
            let exe = std::env::current_dir()?.join("firmware.exe");
            let mut link_cmd = std::process::Command::new("gcc");
            for obj in &objects {
                link_cmd.arg(obj);
            }
            for obj in &mock_objects {
                link_cmd.arg(obj);
            }
            link_cmd.arg("-o").arg(&exe);
            let status = link_cmd.status().context("gcc link failed")?;
            if !status.success() {
                anyhow::bail!("Linking failed");
            }
            println!("Build successful");

            // Execute
            println!("Running simulation...");
            let trace_path = PathBuf::from(
                &cfg.simulation.trace
            );
            let status = std::process::Command::new(&exe)
                .arg(&trace_path)
                .status()
                .context("Firmware execution failed")?;
            if !status.success() {
                anyhow::bail!("Firmware exited with error");
            }

            let candidate = embersim_core::trace::parse_trace(&trace_path)?;
            println!("Trace generated ({} events)", candidate.len());

            // Compare against baseline
            let baseline = PathBuf::from(".embersim/baseline/trace.jsonl");
            if baseline.exists() {
                println!("Comparing baseline...");
                let baseline_events = embersim_core::trace::parse_trace(&baseline)?;
                match embersim_core::trace::find_first_divergence(&baseline_events, &candidate) {
                    None => {
                        println!("\nResult: PASS");
                        println!("Firmware behaviour matches baseline.");
                    }
                    Some(div) => {
                        println!("\nResult: FAIL — firmware regression detected\n");
                        println!("First divergence:");
                        println!("  line: {}", div.line_number);
                        println!();
                        println!("Expected:");
                        println!("  {}", div.expected_func);
                        println!();
                        println!("Observed:");
                        println!("  {}", div.observed_func);
                        println!();
                        let strength = embersim_core::trace::evidence_strength(true, true);
                        println!("Evidence: {}", strength);
                        std::process::exit(1);
                    }
                }
            } else {
                println!("No baseline found. Run 'embersim baseline create' to set one.");
            }
        }
        Commands::Check { output } => {
            let config_path = PathBuf::from("embersim.toml");
            let cfg = if config_path.exists() {
                Some(embersim_core::project::load_toml_config(&config_path)?)
            } else {
                None
            };

            let mocks_dir = output.join("mocks");
            if !mocks_dir.exists() {
                anyhow::bail!(
                    "Mocks not found at {}. Run 'embersim init' first.",
                    mocks_dir.display()
                );
            }

            let sources: Vec<String> = if let Some(ref c) = cfg {
                c.build.sources.clone()
            } else {
                Vec::new()
            };
            let includes: Vec<String> = if let Some(ref c) = cfg {
                c.build.includes.clone()
            } else {
                Vec::new()
            };

            println!("Project analysis\n");
            let result = embersim_core::check::check_project(&sources, &includes, &mocks_dir)?;

            // Group by kind
            let funcs: Vec<_> = result.items.iter().filter(|i| i.kind == embersim_core::check::CheckKind::HalFunction).collect();
            let types: Vec<_> = result.items.iter().filter(|i| i.kind == embersim_core::check::CheckKind::HalType).collect();
            let macros: Vec<_> = result.items.iter().filter(|i| i.kind == embersim_core::check::CheckKind::HalMacro).collect();
            let includes: Vec<_> = result.items.iter().filter(|i| i.kind == embersim_core::check::CheckKind::IncludePath).collect();

            if !funcs.is_empty() {
                println!("HAL functions:");
                for item in &funcs {
                    let mark = match item.status {
                        embersim_core::check::CheckStatus::Supported => "OK",
                        _ => "MISSING",
                    };
                    println!("  {:<40} {}", item.name, mark);
                    if let Some(ref s) = item.suggestion {
                        if item.status != embersim_core::check::CheckStatus::Supported {
                            println!("    → {}", s);
                        }
                    }
                }
                println!();
            }

            if !types.is_empty() {
                println!("HAL types:");
                for item in &types {
                    let mark = match item.status {
                        embersim_core::check::CheckStatus::Supported => "OK",
                        _ => "MISSING",
                    };
                    println!("  {:<40} {}", item.name, mark);
                    if let Some(ref s) = item.suggestion {
                        if item.status != embersim_core::check::CheckStatus::Supported {
                            println!("    → {}", s);
                        }
                    }
                }
                println!();
            }

            if !macros.is_empty() {
                println!("CMSIS macros:");
                for item in &macros {
                    let mark = match item.status {
                        embersim_core::check::CheckStatus::Supported => "STUBBED",
                        _ => "MISSING",
                    };
                    println!("  {:<40} {}", item.name, mark);
                    if let Some(ref s) = item.suggestion {
                        println!("    → {}", s);
                    }
                }
                println!();
            }

            if !includes.is_empty() {
                println!("Includes:");
                for item in &includes {
                    let mark = match item.status {
                        embersim_core::check::CheckStatus::Supported => "OK",
                        _ => "SUGGESTED",
                    };
                    println!("  {:<40} {}", item.name, mark);
                    if let Some(ref s) = item.suggestion {
                        println!("    → {}", s);
                    }
                }
                println!();
            }

            let issues = result.issue_count;
            if issues == 0 {
                println!("Result: ready to run.\nRun 'embersim run' to build and execute.");
            } else {
                println!("Result: {} issue(s) before simulation.", issues);
                println!("Fix the MISSING items above, then run 'embersim run'.");
            }
        }
        Commands::Doctor => {
            run_doctor();
        }
        Commands::Compare { golden, candidate } => {
            if !golden.exists() {
                anyhow::bail!("Golden trace not found: {}", golden.display());
            }
            if !candidate.exists() {
                anyhow::bail!("Candidate trace not found: {}", candidate.display());
            }
            let golden_events = embersim_core::trace::parse_trace(&golden)?;
            let candidate_events = embersim_core::trace::parse_trace(&candidate)?;

            println!("=== Firmware Regression ===\n");
            match embersim_core::trace::find_first_divergence(&golden_events, &candidate_events) {
                None => {
                    println!("Result: PASS");
                    println!("Traces are identical ({} events)", golden_events.len());
                }
                Some(div) => {
                    println!("Result: FAIL — firmware regression detected\n");
                    println!("First divergence:");
                    println!("  line: {}", div.line_number);
                    println!();
                    println!("Expected:");
                    println!("  {}", div.expected_func);
                    println!();
                    println!("Observed:");
                    println!("  {}", div.observed_func);
                    println!();
                    let strength = embersim_core::trace::evidence_strength(true, true);
                    println!("Evidence: {}", strength);
                }
            }
        }
        Commands::Baseline { action } => match action {
            BaselineAction::Create { trace } => {
                if !trace.exists() {
                    anyhow::bail!("Trace file not found: {}\nRun firmware first to generate a trace.", trace.display());
                }
                let baseline_dir = PathBuf::from(".embersim/baseline");
                std::fs::create_dir_all(&baseline_dir)?;
                let dest = baseline_dir.join("trace.jsonl");
                std::fs::copy(&trace, &dest)?;
                println!("Baseline created: {}", dest.display());
                println!("  events: {}", embersim_core::trace::parse_trace(&dest)?.len());
                println!();
                println!("Run 'embersim test' to compare future runs against this baseline.");
            }
        },
        Commands::Test { config, output } => {
            let baseline = PathBuf::from(".embersim/baseline/trace.jsonl");
            if !baseline.exists() {
                anyhow::bail!(
                    "No baseline found at {}\nRun 'embersim baseline create' first.",
                    baseline.display()
                );
            }

            let cfg = embersim_core::project::load_config(&config)?;
            if !output.join("mocks").exists() {
                anyhow::bail!(
                    "Workspace not initialized in {}. Run 'embersim init' first.",
                    output.display()
                );
            }

            println!("Building firmware...");
            embersim_core::project::build_host(&cfg, &output, "debug")?;

            let trace_path = output.join("out/trace.jsonl");
            println!("Running simulation...");
            embersim_core::project::run_tests(&cfg, &output, &trace_path)?;

            let candidate_events = embersim_core::trace::parse_trace(&trace_path)?;
            println!("Trace generated ({} events)", candidate_events.len());
            println!();

            let baseline_events = embersim_core::trace::parse_trace(&baseline)?;
            match embersim_core::trace::find_first_divergence(&baseline_events, &candidate_events) {
                None => {
                    println!("Result: PASS");
                    println!("Firmware behaviour matches baseline.");
                }
                Some(div) => {
                    println!("Result: FAIL — firmware regression detected\n");
                    println!("First divergence:");
                    println!("  line: {}", div.line_number);
                    println!();
                    println!("Expected:");
                    println!("  {}", div.expected_func);
                    println!();
                    println!("Observed:");
                    println!("  {}", div.observed_func);
                    println!();
                    let strength = embersim_core::trace::evidence_strength(true, true);
                    println!("Evidence: {}", strength);
                    std::process::exit(1);
                }
            }
        }
    }
    Ok(())
}

fn run_doctor() {
    use std::process::Command;

    println!("EmberSim Doctor — Environment Check\n");
    println!("{}", "=".repeat(50));

    let mut all_ok = true;

    // Check Rust
    match Command::new("rustc").arg("--version").output() {
        Ok(o) => {
            let v = String::from_utf8_lossy(&o.stdout);
            println!("[PASS] rustc: {}", v.trim());
        }
        Err(_) => {
            println!("[FAIL] rustc not found on PATH");
            println!("       Install: https://rustup.rs");
            all_ok = false;
        }
    }

    // Check C preprocessor
    let mut preprocessor = None;
    for cmd in &["gcc", "clang"] {
        if let Ok(o) = Command::new(cmd).arg("--version").output() {
            let v = String::from_utf8_lossy(&o.stdout);
            let first_line = v.lines().next().unwrap_or("");
            println!("[PASS] C preprocessor: {} — {}", cmd, first_line);
            preprocessor = Some(cmd.to_string());
            break;
        }
    }
    if preprocessor.is_none() {
        println!("[FAIL] No C preprocessor found (gcc or clang)");
        println!("       Windows: Install MSYS2 and run: pacman -S mingw-w64-ucrt-x86_64-gcc");
        println!("       Then add: C:\\msys64\\ucrt64\\bin to PATH");
        println!("       macOS:   xcode-select --install");
        println!("       Linux:   sudo apt install gcc");
        all_ok = false;
    }

    // Check CMake
    match Command::new("cmake").arg("--version").output() {
        Ok(o) => {
            let v = String::from_utf8_lossy(&o.stdout);
            println!("[PASS] cmake: {}", v.lines().next().unwrap_or(""));
        }
        Err(_) => {
            println!("[WARN] cmake not found on PATH (needed for 'embersim build')");
            println!("       Install: https://cmake.org/download/");
        }
    }

    // Check embersim.json
    if std::path::Path::new("embersim.json").exists() {
        match embersim_core::project::load_config(std::path::Path::new("embersim.json")) {
            Ok(cfg) => {
                println!("[PASS] embersim.json: valid (project: {})", cfg.project.name);
            }
            Err(e) => {
                println!("[FAIL] embersim.json: parse error — {}", e);
                all_ok = false;
            }
        }
    } else {
        println!("[INFO] No embersim.json in current directory (run 'embersim init --auto')");
    }

    println!("{}", "=".repeat(50));
    if all_ok {
        println!("\nEnvironment ready. Run 'embersim init' to create a workspace.");
    } else {
        println!("\nFix the FAIL items above before running embersim.");
    }
}