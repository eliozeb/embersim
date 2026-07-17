use anyhow::{Context, Result};
use std::fs;
use std::path::{Path, PathBuf};

use crate::model::{Project, ProjectConfig};

/// Automatically detect project structure in `project_root` and return a
/// ready‑to‑use ProjectConfig.
pub fn discover(project_root: &Path) -> Result<ProjectConfig> {
    // 1. Try to find a .ioc file
    if let Some(ioc_path) = find_ioc(project_root) {
        return discover_from_ioc(project_root, &ioc_path);
    }

    // 2. Fall back to build‑system heuristics
    if project_root.join("Makefile").exists() || project_root.join("CMakeLists.txt").exists() {
        return discover_from_build_system(project_root);
    }

    anyhow::bail!(
        "No .ioc file or Makefile/CMakeLists.txt found in {}.\n\
        Run 'embersim init' with explicit -f/-I options or create an embersim.json manually.",
        project_root.display()
    )
}

// ---------------------------------------------------------------------------
// helpers
// ---------------------------------------------------------------------------

fn find_ioc(dir: &Path) -> Option<PathBuf> {
    fs::read_dir(dir).ok()?.filter_map(|e| {
        let e = e.ok()?;
        let p = e.path();
        if p.extension().map_or(false, |ext| ext == "ioc") {
            Some(p)
        } else {
            None
        }
    }).next()
}

fn project_name_from_dir(dir: &Path) -> String {
    dir.file_name()
        .map(|n| n.to_string_lossy().to_string())
        .unwrap_or_else(|| "untitled".to_string())
}

fn collect_c_files(dir: &Path, relative_to: &Path) -> Result<Vec<String>> {
    let mut files = Vec::new();
    if !dir.is_dir() {
        return Ok(files);
    }
    for entry in walkdir::WalkDir::new(dir).into_iter().filter_map(|e| e.ok()) {
        if entry.path().extension().map_or(false, |ext| ext == "c") {
            let rel = entry.path().strip_prefix(relative_to)
                .unwrap_or(entry.path());
            files.push(rel.to_string_lossy().to_string());
        }
    }
    Ok(files)
}

// ---------------------------------------------------------------------------
// .ioc parsing
// ---------------------------------------------------------------------------

fn discover_from_ioc(project_root: &Path, ioc_path: &Path) -> Result<ProjectConfig> {
    let xml = fs::read_to_string(ioc_path).context("Failed to read .ioc file")?;

    use quick_xml::Reader;
    use quick_xml::events::Event;

    let mut reader = Reader::from_str(&xml);
    reader.config_mut().trim_text(true);

    let mut project_name = project_name_from_dir(project_root);

    // Crude extraction: look for <Project Name="...">
    loop {
        match reader.read_event()? {
            Event::Start(e) if e.name().as_ref() == b"Project" => {
                for attr in e.attributes().flatten() {
                    if attr.key.as_ref() == b"Name" {
                        project_name = attr.unescape_value().unwrap_or_default().to_string();
                    }
                }
            }
            Event::Eof => break,
            _ => {}
        }
    }

    // Typical CubeMX paths – will be extended later for more MCU families
    let hal_path = project_root.join("Drivers/STM32F4xx_HAL_Driver/Inc");
    let _core_inc = project_root.join("Core/Inc");
    let _cmsis_inc = project_root.join("Drivers/CMSIS/Device/ST/STM32F4xx/Include");
    // Collect sources under Core/Src (conventional)
    let src_dir = project_root.join("Core/Src");
    let source_files = collect_c_files(&src_dir, project_root)?;

    // Store hal_path as string (relative to project_root if possible)
    let hal_path_str = hal_path
        .strip_prefix(project_root)
        .unwrap_or(&hal_path)
        .to_string_lossy()
        .to_string();

    Ok(ProjectConfig {
        project: Project {
            name: project_name,
            source_files,
            exclude_files: Vec::new(),
            hal_path: hal_path_str,
            hal_conf: "Core/Inc/stm32f4xx_hal_conf.h".to_string(),
        },
    })
}

// ---------------------------------------------------------------------------
// build‑system fallback
// ---------------------------------------------------------------------------

fn discover_from_build_system(project_root: &Path) -> Result<ProjectConfig> {
    let project_name = project_name_from_dir(project_root);

    // Try multiple conventional source directories
    let src_dirs = ["Core/Src", "Src", "src"];
    let mut source_files = Vec::new();

    for dir_name in &src_dirs {
        let p = project_root.join(dir_name);
        if p.exists() {
            source_files.extend(collect_c_files(&p, project_root)?);
        }
    }

    if source_files.is_empty() {
        anyhow::bail!(
            "No C source files found in {:?}. Please create an embersim.json manually.",
            src_dirs
        );
    }

    let hal_path = project_root.join("Drivers/STM32F4xx_HAL_Driver/Inc");
    let hal_path_str = hal_path
        .strip_prefix(project_root)
        .unwrap_or(&hal_path)
        .to_string_lossy()
        .to_string();

    Ok(ProjectConfig {
        project: Project {
            name: project_name,
            source_files,
            exclude_files: Vec::new(),
            hal_path: hal_path_str,
            hal_conf: "Core/Inc/stm32f4xx_hal_conf.h".to_string(),
        },
    })
}