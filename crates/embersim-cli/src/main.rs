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
        hal: PathBuf,
        #[arg(short = 'I', long = "include", value_name = "DIR")]
        include: Vec<String>,
        #[arg(short = 'D', long = "define", value_name = "MACRO")]
        define: Vec<String>,
        #[arg(short = 'o', long, value_name = "DIR", default_value = "ember_sim")]
        output: PathBuf,
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
        Commands::Init { hal, include, define, output } => {
            if !hal.exists() {
                anyhow::bail!("Header not found: {}", hal.display());
            }
            let functions = embersim_core::pipeline::run(&hal, &include, &define)?;
            embersim_core::project::initialize(&functions, &output)?;
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