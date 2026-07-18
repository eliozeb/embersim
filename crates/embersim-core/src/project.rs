use anyhow::{Context, Result};
use std::fs;
use std::path::Path;
use std::process::Command;
use crate::model::{HalFunction, ProjectConfig};
use crate::stubgen;
use handlebars::Handlebars;

pub fn initialize(
    hal_functions: &[HalFunction],
    output_dir: &Path,
) -> Result<()> {
    fs::create_dir_all(output_dir.join("mocks"))?;

    stubgen::generate(hal_functions, &output_dir.join("mocks"))?;

    fs::write(output_dir.join("mocks/trace_log.h"), crate::templates::TRACE_LOG_H)?;
    fs::write(output_dir.join("mocks/trace_log.c"), crate::templates::TRACE_LOG_C)?;
    fs::write(output_dir.join("mocks/mock_state.c"), crate::templates::MOCK_STATE_C)?;
    fs::write(output_dir.join("mocks/mock_uart.c"), crate::templates::MOCK_UART_C)?;
    fs::write(output_dir.join("mocks/mock_i2c.c"), crate::templates::MOCK_I2C_C)?;
    fs::write(output_dir.join("mocks/mock_spi.h"), crate::templates::MOCK_SPI_H)?;
    fs::write(output_dir.join("mocks/mock_spi.c"), crate::templates::MOCK_SPI_C)?;
    fs::write(output_dir.join("mocks/mock_tim.h"), crate::templates::MOCK_TIM_H)?;
    fs::write(output_dir.join("mocks/mock_tim.c"), crate::templates::MOCK_TIM_C)?;
    fs::write(output_dir.join("mocks/ember_sim_scheduler.h"), crate::templates::EMBER_SIM_SCHEDULER_H)?;
    fs::write(output_dir.join("mocks/ember_sim_scheduler.c"), crate::templates::EMBER_SIM_SCHEDULER_C)?;
    fs::write(output_dir.join("mocks/ember_sim_runtime.h"), crate::templates::EMBER_SIM_RUNTIME_H)?;
    fs::write(output_dir.join("mocks/ember_sim_runtime.c"), crate::templates::EMBER_SIM_RUNTIME_C)?;
    fs::write(output_dir.join("host_main.c"), crate::templates::HOST_MAIN_C)?;

    eprintln!("EmberSim workspace initialized in: {}", output_dir.display());
    Ok(())
}

/// Load project configuration from embersim.json
pub fn load_config(path: &Path) -> Result<ProjectConfig> {
    let contents = fs::read_to_string(path)
        .with_context(|| format!("Cannot read config file at {}", path.display()))?;
    serde_json::from_str(&contents).context("Invalid embersim.json format")
}

/// Render CMakeLists.txt and build the host binary
pub fn build_host(config: &ProjectConfig, output_dir: &Path, profile: &str) -> Result<()> {
    // Render CMake template
    let template_str = std::str::from_utf8(crate::templates::HOST_CMAKE)
        .context("CMake template not valid UTF-8")?;
    let mut handlebars = Handlebars::new();
    handlebars.register_template_string("cmake", template_str)?;

    let data = serde_json::json!({
        "project_name": config.project.name,
        "user_sources": config.project.source_files,
    });

    let rendered = handlebars.render("cmake", &data)?;
    fs::write(output_dir.join("host_CMakeLists.txt"), rendered)?;

    // Create build directory
    let build_dir = output_dir.join("build");
    fs::create_dir_all(&build_dir)?;

    // CMake configure
    let build_type = if profile == "release" { "Release" } else { "Debug" };
    let status = Command::new("cmake")
        .args(["-DCMAKE_BUILD_TYPE=", build_type])
        .arg("..")
        .current_dir(&build_dir)
        .status()
        .context("Failed to run cmake configure")?;
    if !status.success() {
        anyhow::bail!("CMake configuration failed");
    }

    // CMake build
    let status = Command::new("cmake")
        .args(["--build", "."])
        .current_dir(&build_dir)
        .status()
        .context("cmake build failed")?;
    if !status.success() {
        anyhow::bail!("Build failed");
    }

    eprintln!("Build successful. Binary: {}", build_dir.join("embersim_host").display());
    Ok(())
}

/// Run the compiled host binary
pub fn run_tests(_config: &ProjectConfig, output_dir: &Path, trace_path: &Path) -> Result<()> {
    let exe_path = output_dir.join("build").join("embersim_host");
    if !exe_path.exists() {
        anyhow::bail!(
            "Host binary not found at {}. Run 'embersim build' first.",
            exe_path.display()
        );
    }

    // Ensure output directory for trace exists
    if let Some(parent) = trace_path.parent() {
        fs::create_dir_all(parent)?;
    }

    let status = Command::new(&exe_path)
        .arg(trace_path)
        .status()
        .with_context(|| format!("Failed to run {}", exe_path.display()))?;

    if !status.success() {
        anyhow::bail!("Host test binary exited with error");
    }

    eprintln!("Tests finished. Trace: {}", trace_path.display());
    Ok(())
}