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
        /// Path to the HAL header (e.g. Drivers/STM32F4xx_HAL_Driver/Inc/stm32f4xx_hal.h)
        #[arg(short = 'f', long, value_name = "FILE")]
        hal: PathBuf,

        /// Add an include path for the C preprocessor (can be given multiple times).
        #[arg(short = 'I', long = "include", value_name = "DIR")]
        include: Vec<String>,

        /// Add a preprocessor define (can be given multiple times).
        #[arg(short = 'D', long = "define", value_name = "MACRO")]
        define: Vec<String>,
    },
}

fn main() -> Result<()> {
    let cli = Cli::parse();
    match cli.command {
        Commands::Generate { hal, include, define } => {
            if !hal.exists() { anyhow::bail!("Header not found: {}", hal.display()); }
            let functions = embersim_core::pipeline::run(&hal, &include, &define)?;
            println!("{}", serde_json::to_string_pretty(&functions)?);
        }
    }
    Ok(())
}