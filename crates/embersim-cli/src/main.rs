use anyhow::Result;
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
    /// Run the host binary and collect trace
    Test {
        #[arg(short = 'c', long, value_name = "FILE", default_value = "embersim.json")]
        config: PathBuf,
        #[arg(short = 'o', long, value_name = "DIR", default_value = "ember_sim")]
        output: PathBuf,
        #[arg(short = 't', long, value_name = "FILE", default_value = "ember_sim/out/trace.jsonl")]
        trace: PathBuf,
    },
    /// Initialize workspace, build, and run tests in one command
    Run {
        #[arg(short = 'f', long, value_name = "FILE")]
        hal: PathBuf,
        #[arg(short = 'I', long = "include", value_name = "DIR")]
        include: Vec<String>,
        #[arg(short = 'D', long = "define", value_name = "MACRO")]
        define: Vec<String>,
        #[arg(short = 'o', long, value_name = "DIR", default_value = "ember_sim")]
        output: PathBuf,
        #[arg(short = 'c', long, value_name = "FILE", default_value = "embersim.json")]
        config: PathBuf,
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
        Commands::Init { hal, include, define, output, auto } => {
            if auto {
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
        Commands::Test { config, output, trace } => {
            let cfg = embersim_core::project::load_config(&config)?;
            embersim_core::project::run_tests(&cfg, &output, &trace)?;
        }
        Commands::Run { hal, include, define, output, config } => {
            if !hal.exists() {
                anyhow::bail!("Header not found: {}", hal.display());
            }
            let functions = embersim_core::pipeline::run(&hal, &include, &define)?;
            embersim_core::project::initialize(&functions, &output)?;

            let cfg = embersim_core::project::load_config(&config)?;
            embersim_core::project::build_host(&cfg, &output, "debug")?;

            let trace_file = output.join("out/trace.jsonl");
            embersim_core::project::run_tests(&cfg, &output, &trace_file)?;
        }
    }
    Ok(())
}