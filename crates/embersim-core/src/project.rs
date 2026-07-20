use anyhow::{Context, Result};
use std::fs;
use std::path::Path;
use std::process::Command;
use crate::model::{EmberSimConfig, HalFunction, ProjectConfig};
use crate::stubgen;
use handlebars::Handlebars;

pub fn initialize(
    hal_functions: &[HalFunction],
    output_dir: &Path,
) -> Result<()> {
    fs::create_dir_all(output_dir.join("mocks"))?;

    stubgen::generate(hal_functions, &output_dir.join("mocks"))?;

    // Core runtime and register model
    fs::write(output_dir.join("mocks/ember_sim_kernel.h"), crate::templates::EMBER_SIM_KERNEL_H)?;
    fs::write(output_dir.join("mocks/ember_sim_kernel.c"), crate::templates::EMBER_SIM_KERNEL_C)?;
    fs::write(output_dir.join("mocks/ember_regs.h"), crate::templates::EMBER_REGS_H)?;
    fs::write(output_dir.join("mocks/ember_regs.c"), crate::templates::EMBER_REGS_C)?;

    // Peripherals
    fs::write(output_dir.join("mocks/trace_log.h"), crate::templates::TRACE_LOG_H)?;
    fs::write(output_dir.join("mocks/trace_log.c"), crate::templates::TRACE_LOG_C)?;
    fs::write(output_dir.join("mocks/mock_state.c"), crate::templates::MOCK_STATE_C)?;
    fs::write(output_dir.join("mocks/mock_uart.c"), crate::templates::MOCK_UART_C)?;
    fs::write(output_dir.join("mocks/mock_i2c.c"), crate::templates::MOCK_I2C_C)?;
    fs::write(output_dir.join("mocks/mock_spi.c"), crate::templates::MOCK_SPI_C)?;
    fs::write(output_dir.join("mocks/mock_tim.c"), crate::templates::MOCK_TIM_C)?;
    fs::write(output_dir.join("mocks/mock_tim.h"), crate::templates::MOCK_TIM_H)?;

    fs::write(output_dir.join("mocks/mock_spi.h"), crate::templates::MOCK_SPI_H)?;

    fs::write(output_dir.join("host_main.c"), crate::templates::HOST_MAIN_C)?;

    // Generate minimal embersim.toml — describes intent, not build system
    let config = EmberSimConfig {
        project: crate::model::ProjectMeta {
            name: "embersim-project".into(),
            description: String::new(),
        },
        build: Default::default(),
        simulation: crate::model::SimulationConfig::default(),
    };
    let toml_str = toml::to_string_pretty(&config)?;
    fs::write(output_dir.join("embersim.toml"), toml_str)?;

    eprintln!("EmberSim workspace initialized in: {}", output_dir.display());
    eprintln!();
    eprintln!("Next step:");
    eprintln!("  embersim check -o {}", output_dir.display());
    eprintln!();
    eprintln!("This validates HAL coverage and identifies required simulation adaptations.");
    Ok(())
}

/// Load embersim.toml configuration
pub fn load_toml_config(path: &Path) -> Result<EmberSimConfig> {
    let contents = fs::read_to_string(path)
        .with_context(|| format!("Cannot read config: {}", path.display()))?;
    toml::from_str(&contents).context("Invalid embersim.toml format")
}

/// Repair project configuration: generate missing shims, update includes, report fixes.
/// Never modifies firmware source files.
pub fn repair_project(output_dir: &Path) -> Result<Vec<String>> {
    let mut fixes = Vec::new();
    let mocks_dir = output_dir.join("mocks");

    // Read current config
    let config_path = Path::new("embersim.toml");
    let mut cfg: EmberSimConfig = if config_path.exists() {
        load_toml_config(config_path)?
    } else {
        EmberSimConfig {
            project: crate::model::ProjectMeta {
                name: "embersim-project".into(),
                description: String::new(),
            },
            build: Default::default(),
            simulation: crate::model::SimulationConfig::default(),
        }
    };

    // Run check to find issues
    let check_result = crate::check::check_project(
        &cfg.build.sources,
        &cfg.build.includes,
        &mocks_dir,
    )?;

    let mut missing_macros: Vec<String> = Vec::new();
    let mut missing_includes: Vec<String> = Vec::new();
    let mut type_issues = false;

    for item in &check_result.items {
        match item.kind {
            crate::check::CheckKind::HalMacro if item.status == crate::check::CheckStatus::Missing => {
                missing_macros.push(item.name.clone());
            }
            crate::check::CheckKind::IncludePath if item.status == crate::check::CheckStatus::Missing => {
                if let Some(ref s) = item.suggestion {
                    // Extract directory from suggestion like "Add 'Core/Src' to [build.includes]"
                    for word in s.split('\'') {
                        let w = word.trim();
                        if !w.is_empty() && w.contains('/') && !w.starts_with('[') {
                            missing_includes.push(w.to_string());
                        }
                    }
                }
            }
            crate::check::CheckKind::HalType if item.status == crate::check::CheckStatus::Missing => {
                type_issues = true;
            }
            _ => {}
        }
    }

    // 1. Generate cmsis_shim.h for missing macros
    if !missing_macros.is_empty() {
        let mut shim = String::from(
            "/* cmsis_shim.h — EmberSim CMSIS macro compatibility shim\n\
             * Generated by 'embersim init --repair'.\n\
             * These stubs emit runtime warnings. Replace with mock_* API calls in production.\n\
             */\n\n\
             #ifndef CMSIS_SHIM_H\n\
             #define CMSIS_SHIM_H\n\n\
             #include <stdio.h>\n\n\
             #define EMBER_SIM_WARN_MACRO(name) \\\n    fprintf(stderr, \"EMBERSIM: HAL macro '%s' not simulated. Replace with mock_* API.\\n\", name)\n\n"
        );

        for mac in &missing_macros {
            shim.push_str(&format!(
                "#ifndef {0}\n#define {0}(...) \\\n    EMBER_SIM_WARN_MACRO(\"{0}\")\n#endif\n\n",
                mac
            ));
        }

        shim.push_str("#endif /* CMSIS_SHIM_H */\n");
        fs::write(mocks_dir.join("cmsis_shim.h"), &shim)?;
        fixes.push(format!(
            "Created cmsis_shim.h with {} macro stub(s): {}. Add '#include \"cmsis_shim.h\"' to firmware sources.",
            missing_macros.len(),
            missing_macros.join(", ")
        ));
    }

    // 2. Add discovered include paths to embersim.toml
    let mut added_includes = Vec::new();
    for inc in &missing_includes {
        if !cfg.build.includes.contains(inc) {
            cfg.build.includes.push(inc.clone());
            added_includes.push(inc.clone());
        }
    }
    // Also auto-discover conventional directories
    for dir in &["Core/Inc", "Core/Src", "Drivers/CMSIS/Include", "Inc", "Src"] {
        if Path::new(dir).is_dir() && !cfg.build.includes.contains(&dir.to_string()) {
            cfg.build.includes.push(dir.to_string());
            added_includes.push(dir.to_string());
        }
    }
    if !added_includes.is_empty() {
        let toml_str = toml::to_string_pretty(&cfg)?;
        fs::write("embersim.toml", &toml_str)?;
        fixes.push(format!(
            "Updated embersim.toml: added include paths: {}",
            added_includes.join(", ")
        ));
    }

    // 3. Note type guidance without modifying firmware
    if type_issues {
        fixes.push(
            "HAL types not found in configured headers. Include 'mock_hal.h' in firmware sources. (Firmware not modified — add manually.)"
                .into(),
        );
    }

    // 4. Ensure mocks directory exists
    fs::create_dir_all(&mocks_dir)?;

    Ok(fixes)
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