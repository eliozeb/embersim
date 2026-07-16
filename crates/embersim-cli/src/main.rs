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
    /// Parse HAL headers and generate mock stubs
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

    /// Initialize a full EmberSim workspace (mocks + templates + host runner)
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
}

fn main() -> Result<()> {
    let cli = Cli::parse();
    match cli.command {
        Commands::Generate { hal, include, define, output } => {
            if !hal.exists() { anyhow::bail!("Header not found: {}", hal.display()); }
            let functions = embersim_core::pipeline::run(&hal, &include, &define)?;
            embersim_core::stubgen::generate(&functions, &output)?;
            eprintln!("Generated mocks in {}", output.display());
        }
        Commands::Init { hal, include, define, output } => {
            if !hal.exists() { anyhow::bail!("Header not found: {}", hal.display()); }
            let functions = embersim_core::pipeline::run(&hal, &include, &define)?;
            embersim_core::project::initialize(&functions, &output)?;
        }
    }
    Ok(())
}