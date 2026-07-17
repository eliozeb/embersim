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

    // 1. Generate mock_hal.h and mock_hal.c (weak stubs) from parsed headers
    stubgen::generate(hal_functions, &output_dir.join("mocks"))?;

    // 2. Copy embedded templates
    fs::write(output_dir.join("mocks/trace_log.h"), crate::templates::TRACE_LOG_H)?;
    fs::write(output_dir.join("mocks/trace_log.c"), crate::templates::TRACE_LOG_C)?;
    fs::write(output_dir.join("mocks/mock_state.c"), crate::templates::MOCK_STATE_C)?;
    fs::write(output_dir.join("mocks/mock_uart.c"), crate::templates::MOCK_UART_C)?;
    fs::write(output_dir.join("host_main.c"), crate::templates::HOST_MAIN_C)?;

    eprintln!("EmberSim workspace initialized in: {}", output_dir.display());
    Ok(())
}

/// Load project configuration from an embersim.json file
pub fn load_config(path: &Path) -> Result<ProjectConfig> {
    let contents = fs::read_to_string(path)
        .with_context(|| format!("Cannot read config file at {}", path.display()))?;
    serde_json::from_str(&contents).context("Invalid embersim.json format")
}

/// Build the host binary using CMake
pub fn build_host(config: &ProjectConfig, output_dir: &Path, profile: &str) -> Result<()> {
    // 1. Render the CMakeLists template with user source files
    let template_str = std::str::from_utf8(crate::templates::HOST_CMAKE)
        .context("CMake template not valid UTF-8")?;
    let mut handlebars = Handlebars::new();
    handlebars.register_template_string("cmake", template_str)?;

    // Prepare data: relative paths from output_dir to source files (user sources are relative to project root)
    let user_sources: Vec<String> = config.project.source_files
        .iter()
        .map(|f| {
            // Convert to absolute path relative to the directory containing embersim.json
            // Since the CMakeLists will sit in output_dir (ember_sim/), we need relative paths from there
            // For simplicity, we'll use absolute paths if the source files are not within the build tree.
            // Better: assume source files are relative to the project root (where embersim.json lives).
            // The CMake will be run from output_dir/build, and we'll set source paths accordingly.
            // We'll compute relative path from output_dir/build/.. to the source file.
            // To keep it simple, we'll use absolute paths generated from the current directory.
            // Or we can require the user to list paths relative to the project root.
            // Here, we'll just pass them as given; the user should give paths relative to project root.
            // The CMakeLists will be in output_dir (ember_sim/), so we need paths like ../src/app.c
            // So we'll prepend ".." to go up one level from ember_sim/.
            format!("../{}", f)
        })
        .collect();

    let data = serde_json::json!({
        "project_name": config.project.name,
        "user_sources": user_sources,
    });

    let rendered = handlebars.render("cmake", &data)?;
    fs::write(output_dir.join("host_CMakeLists.txt"), rendered)?;

    // 2. Create build directory
    let build_dir = output_dir.join("build");
    fs::create_dir_all(&build_dir)?;

    // 3. Run CMake configure
    let mut cmake_config = Command::new("cmake");
    cmake_config
        .args(["-G", "Ninja"])  // optional, can fall back to default generator
        .arg(if profile == "release" { "-DCMAKE_BUILD_TYPE=Release" } else { "-DCMAKE_BUILD_TYPE=Debug" })
        .arg("..")
        .current_dir(&build_dir);

    // If Ninja not available, remove it and rely on default.
    // For portability, we can just call cmake without -G.
    // I'll omit -G for now.
    let mut cmake_config = Command::new("cmake");
    cmake_config
        .arg(if profile == "release" { "-DCMAKE_BUILD_TYPE=Release" } else { "-DCMAKE_BUILD_TYPE=Debug" })
        .arg("..")
        .current_dir(&build_dir);

    let status = cmake_config.status()
        .with_context(|| format!("Failed to run cmake configure in {}", build_dir.display()))?;
    if !status.success() {
        anyhow::bail!("CMake configuration failed");
    }

    // 4. Build
    let mut cmake_build = Command::new("cmake");
    cmake_build
        .args(["--build", "."])
        .current_dir(&build_dir);

    let status = cmake_build.status()
        .context("cmake build failed")?;
    if !status.success() {
        anyhow::bail!("Build failed");
    }

    eprintln!("Build successful. Binary: {}", build_dir.join("embersim_host").display());
    Ok(())
}

/// Run the compiled host binary and capture trace
pub fn run_tests(_config: &ProjectConfig, output_dir: &Path, trace_path: &Path) -> Result<()> {
    let exe_path = output_dir.join("build").join("embersim_host");
    if !exe_path.exists() {
        anyhow::bail!("Host binary not found at {}. Run 'embersim build' first.", exe_path.display());
    }

    let mut cmd = Command::new(&exe_path);
    cmd.arg(trace_path);
    // If the host binary expects a trace argument, pass it (already implemented in host_main.c)

    let status = cmd.status()
        .with_context(|| format!("Failed to run {}", exe_path.display()))?;
    if !status.success() {
        anyhow::bail!("Host test binary exited with error code");
    }

    eprintln!("Tests finished. Trace output: {}", trace_path.display());
    Ok(())
}