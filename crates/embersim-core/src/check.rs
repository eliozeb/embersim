// check.rs — project readiness analysis for embersim check command
use anyhow::Result;
use regex::Regex;
use std::collections::HashSet;
use std::path::Path;

#[derive(Debug, Clone, PartialEq, Eq, Hash)]
pub enum SymbolKind {
    HalFunction(String),
    HalMacro(String),
    Include(String),
}

#[derive(Debug)]
pub struct CheckItem {
    pub name: String,
    pub kind: CheckKind,
    pub status: CheckStatus,
    pub suggestion: Option<String>,
}

#[derive(Debug, PartialEq, Eq)]
pub enum CheckKind {
    HalFunction,
    HalMacro,
    HalType,
    IncludePath,
}

#[derive(Debug, PartialEq, Eq)]
pub enum CheckStatus {
    Supported,
    Missing,
    Suggested,
}

#[derive(Debug)]
pub struct CheckResult {
    pub items: Vec<CheckItem>,
    pub issue_count: usize,
}

/// Scan a C source file for HAL function calls, HAL macros, HAL types, and includes.
pub fn scan_source(path: &Path) -> Result<(HashSet<String>, HashSet<String>, HashSet<String>, HashSet<String>)> {
    let content = std::fs::read_to_string(path)?;
    let content_no_comments = strip_c_comments(&content);

    // HAL_* function calls: HAL_UART_Transmit, HAL_TIM_Base_Start_IT, etc.
    let hal_func_re = Regex::new(r"\b(HAL_[A-Za-z0-9_]+)\s*\(")?;
    let mut hal_funcs = HashSet::new();
    for cap in hal_func_re.captures_iter(&content_no_comments) {
        hal_funcs.insert(cap[1].to_string());
    }

    // __HAL_* macros: __HAL_TIM_SET_AUTORELOAD, __HAL_RCC_*, etc.
    let hal_macro_re = Regex::new(r"\b(__HAL_[A-Za-z0-9_]+)\s*\(")?;
    let mut hal_macros = HashSet::new();
    for cap in hal_macro_re.captures_iter(&content_no_comments) {
        hal_macros.insert(cap[1].to_string());
    }

    // HAL type usage: TIM_HandleTypeDef, GPIO_InitTypeDef, UART_HandleTypeDef, etc.
    let type_re = Regex::new(r"\b((?:TIM|GPIO|UART|SPI|I2C|ADC|DMA|RCC|NVIC|EXTI|FLASH|PWR|RTC|WWDG|IWDG|CAN|USB|SD|ETH)_[A-Za-z0-9_]+TypeDef)\b")?;
    let mut types = HashSet::new();
    for cap in type_re.captures_iter(&content_no_comments) {
        types.insert(cap[1].to_string());
    }

    // #include "..." directives (local includes only — not system headers)
    let include_re = Regex::new(r#"#include\s+"([^"]+)""#)?;
    let mut includes = HashSet::new();
    for cap in include_re.captures_iter(&content) {
        includes.insert(cap[1].to_string());
    }

    Ok((hal_funcs, hal_macros, types, includes))
}

/// Strip C block comments and line comments. Keeps line numbering intact
/// by replacing comments with spaces.
fn strip_c_comments(source: &str) -> String {
    // Remove block comments /* ... */
    let re_block = Regex::new(r"/\*.*?\*/").unwrap();
    let no_block = re_block.replace_all(source, " ");
    // Remove line comments // ...
    let re_line = Regex::new(r"//[^\n]*").unwrap();
    re_line.replace_all(&no_block, " ").to_string()
}

/// Check which discovered symbols are supported by the generated mocks directory.
/// `configured_includes` are the include paths from embersim.toml [build.includes].
pub fn check_project(
    sources: &[String],
    configured_includes: &[String],
    mocks_dir: &Path,
) -> Result<CheckResult> {
    let mut all_funcs = HashSet::new();
    let mut all_macros = HashSet::new();
    let mut all_types = HashSet::new();
    let mut all_includes = HashSet::new();

    // Scan all source files
    for src in sources {
        let path = Path::new(src);
        if path.exists() && path.extension().map_or(false, |e| e == "c" || e == "h") {
            let (funcs, macros, types, includes) = scan_source(path)?;
            all_funcs.extend(funcs);
            all_macros.extend(macros);
            all_types.extend(types);
            all_includes.extend(includes);
        }
    }

    // Also scan all .c and .h files in current directory if sources list is empty
    if sources.is_empty() {
        if let Ok(entries) = std::fs::read_dir(".") {
            for e in entries.flatten() {
                let p = e.path();
                if p.extension().map_or(false, |ext| ext == "c" || ext == "h") {
                    let (funcs, macros, types, includes) = scan_source(&p)?;
                    all_funcs.extend(funcs);
                    all_macros.extend(macros);
                    all_types.extend(types);
                    all_includes.extend(includes);
                }
            }
        }
    }

    // Discover what mocks provide (functions and types)
    let mut mock_funcs = HashSet::new();
    let mut mock_types = HashSet::new();
    if let Ok(entries) = std::fs::read_dir(mocks_dir) {
        for e in entries.flatten() {
            let p = e.path();
            if p.extension().map_or(false, |ext| ext == "c" || ext == "h") {
                if let Ok(content) = std::fs::read_to_string(&p) {
                    let content_no_comments = strip_c_comments(&content);
                    let re_func = Regex::new(r"\b(HAL_[A-Za-z0-9_]+)\s*\(").unwrap();
                    for cap in re_func.captures_iter(&content_no_comments) {
                        mock_funcs.insert(cap[1].to_string());
                    }
                    let re_type = Regex::new(r"\b((?:TIM|GPIO|UART|SPI|I2C|ADC|DMA|RCC|NVIC|EXTI|FLASH|PWR|RTC|WWDG|IWDG|CAN|USB|SD|ETH)_[A-Za-z0-9_]+TypeDef)\b").unwrap();
                    for cap in re_type.captures_iter(&content_no_comments) {
                        mock_types.insert(cap[1].to_string());
                    }
                }
            }
        }
    }

    let mut items = Vec::new();
    let mut issue_count = 0;

    // Check HAL functions
    for func in &all_funcs {
        if mock_funcs.contains(func.as_str()) {
            items.push(CheckItem {
                name: func.clone(),
                kind: CheckKind::HalFunction,
                status: CheckStatus::Supported,
                suggestion: None,
            });
        } else {
            issue_count += 1;
            items.push(CheckItem {
                name: func.clone(),
                kind: CheckKind::HalFunction,
                status: CheckStatus::Missing,
                suggestion: Some(
                    "Add declaration to HAL header or regenerate mocks with 'embersim init'".into(),
                ),
            });
        }
    }

    // Check HAL types against configured include paths
    // A type is "reachable" if it's declared in any header in the configured include dirs or mocks
    let mut reachable_types = HashSet::new();
    for dir in configured_includes {
        if let Ok(entries) = std::fs::read_dir(dir) {
            for e in entries.flatten() {
                let p = e.path();
                if p.extension().map_or(false, |ext| ext == "h") {
                    if let Ok(content) = std::fs::read_to_string(&p) {
                        let re = Regex::new(r"\b((?:TIM|GPIO|UART|SPI|I2C|ADC|DMA|RCC|NVIC|EXTI|FLASH|PWR|RTC|WWDG|IWDG|CAN|USB|SD|ETH)_[A-Za-z0-9_]+TypeDef)\b").unwrap();
                        for cap in re.captures_iter(&content) {
                            reachable_types.insert(cap[1].to_string());
                        }
                    }
                }
            }
        }
    }
    // Also check in mocks dir (always reachable via -I mocks)
    for ty in &mock_types {
        reachable_types.insert(ty.clone());
    }

    // Check source directories for unconfigured include paths
    let mut unconfigured_dirs = HashSet::new();
    for src in sources {
        let path = Path::new(src);
        if let Some(parent) = path.parent() {
            let parent_str = parent.to_string_lossy().to_string();
            if !parent_str.is_empty() && parent_str != "." {
                let is_configured = configured_includes.iter().any(|d| {
                    Path::new(d) == Path::new(&parent_str)
                });
                if !is_configured {
                    // Check if this directory contains .h files
                    if let Ok(entries) = std::fs::read_dir(parent) {
                        let has_headers = entries.flatten().any(|e| {
                            e.path().extension().map_or(false, |ext| ext == "h")
                        });
                        if has_headers {
                            unconfigured_dirs.insert(parent_str);
                        }
                    }
                }
            }
        }
    }

    for ty in &all_types {
        if reachable_types.contains(ty.as_str()) {
            // Type is reachable — but check WHERE it comes from
            let in_user_headers = configured_includes.iter().any(|dir| {
                if let Ok(entries) = std::fs::read_dir(dir) {
                    entries.flatten().any(|e| {
                        let p = e.path();
                        if p.extension().map_or(false, |ext| ext == "h") {
                            if let Ok(c) = std::fs::read_to_string(&p) {
                                c.contains(ty.as_str())
                            } else { false }
                        } else { false }
                    })
                } else { false }
            });
            let in_mocks = mock_types.contains(ty.as_str());

            if in_user_headers {
                items.push(CheckItem {
                    name: ty.clone(),
                    kind: CheckKind::HalType,
                    status: CheckStatus::Supported,
                    suggestion: None,
                });
            } else if in_mocks {
                // Type is in mock_hal.h but NOT in the user's HAL header
                // This is a friction point: engineer includes stm32f4xx_hal.h but type is in mock_hal.h
                issue_count += 1;
                items.push(CheckItem {
                    name: ty.clone(),
                    kind: CheckKind::HalType,
                    status: CheckStatus::Missing,
                    suggestion: Some(
                        "Replace '#include \"stm32f4xx_hal.h\"' with '#include \"mock_hal.h\"' for simulation builds. EmberSim does not modify firmware semantics automatically — the firmware must explicitly select the simulation HAL boundary.".into(),
                    ),
                });
            } else {
                items.push(CheckItem {
                    name: ty.clone(),
                    kind: CheckKind::HalType,
                    status: CheckStatus::Supported,
                    suggestion: None,
                });
            }
        } else {
            issue_count += 1;
            items.push(CheckItem {
                name: ty.clone(),
                kind: CheckKind::HalType,
                status: CheckStatus::Missing,
                suggestion: Some(
                    "Type not found in configured include paths or generated mocks. Include 'mock_hal.h' or add the header directory to [build.includes].".into(),
                ),
            });
        }
    }

    // Report unconfigured source directories containing headers
    for dir in &unconfigured_dirs {
        issue_count += 1;
        items.push(CheckItem {
            name: format!("{}/", dir),
            kind: CheckKind::IncludePath,
            status: CheckStatus::Missing,
            suggestion: Some(format!(
                "Source directory '{}' contains headers but is not in [build.includes]. Add it.",
                dir
            )),
        });
    }

    // Check HAL macros against generated cmsis_shim.h
    let mut shim_macros = HashSet::new();
    let shim_path = mocks_dir.join("cmsis_shim.h");
    if shim_path.exists() {
        if let Ok(content) = std::fs::read_to_string(&shim_path) {
            let re = Regex::new(r"#define\s+(__HAL_[A-Za-z0-9_]+)\b").unwrap();
            for cap in re.captures_iter(&content) {
                shim_macros.insert(cap[1].to_string());
            }
        }
    }

    for mac in &all_macros {
        if shim_macros.contains(mac.as_str()) {
            items.push(CheckItem {
                name: mac.clone(),
                kind: CheckKind::HalMacro,
                status: CheckStatus::Supported,
                suggestion: Some("Stub provided by cmsis_shim.h. Add '#include \"cmsis_shim.h\"' to firmware sources. Emits runtime warning — replace with mock_* API in production.".into()),
            });
        } else {
            issue_count += 1;
            items.push(CheckItem {
                name: mac.clone(),
                kind: CheckKind::HalMacro,
                status: CheckStatus::Missing,
                suggestion: Some(
                    "CMSIS macro not simulated. Run 'embersim init --repair' to generate a compatibility stub."
                        .into(),
                ),
            });
        }
    }

    // Check include paths — report what the project needs
    for inc in &all_includes {
        // Check if the include file exists in mocks or known directories
        let in_mocks = mocks_dir.join(inc).exists();
        let in_configured = configured_includes.iter().any(|d| Path::new(d).join(inc).exists());
        let in_cwd = Path::new(inc).exists();

        if in_mocks || in_configured || in_cwd {
            items.push(CheckItem {
                name: inc.clone(),
                kind: CheckKind::IncludePath,
                status: CheckStatus::Supported,
                suggestion: None,
            });
        } else {
            // Try to find which directory contains this file
            let mut found_dir = None;
            for search_dir in &["Core/Inc", "Core/Src", "Src", "Inc", "include", "."] {
                if Path::new(search_dir).join(inc).exists() {
                    found_dir = Some(search_dir.to_string());
                    break;
                }
            }
            if let Some(dir) = found_dir {
                if configured_includes.contains(&dir) {
                    items.push(CheckItem {
                        name: inc.clone(),
                        kind: CheckKind::IncludePath,
                        status: CheckStatus::Supported,
                        suggestion: None,
                    });
                } else {
                    issue_count += 1;
                    items.push(CheckItem {
                        name: inc.clone(),
                        kind: CheckKind::IncludePath,
                        status: CheckStatus::Missing,
                        suggestion: Some(format!(
                            "Add '{}' to [build.includes] in embersim.toml", dir
                        )),
                    });
                }
            } else {
                items.push(CheckItem {
                    name: inc.clone(),
                    kind: CheckKind::IncludePath,
                    status: CheckStatus::Missing,
                    suggestion: Some(format!(
                        "Cannot locate '{}'. Ensure the directory is in [build.includes]",
                        inc
                    )),
                });
            }
        }
    }

    // If nothing was discovered at all, report that
    if items.is_empty() {
        items.push(CheckItem {
            name: "No HAL symbols or includes found in sources".into(),
            kind: CheckKind::HalFunction,
            status: CheckStatus::Missing,
            suggestion: Some("Check that [build.sources] in embersim.toml lists your firmware .c files".into()),
        });
        issue_count = 1;
    }

    Ok(CheckResult { items, issue_count })
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn test_scan_hal_functions() {
        let code = r#"
            void test(void) {
                HAL_UART_Transmit(&huart, data, 5, 100);
                HAL_TIM_Base_Start_IT(&htim);
                int x = HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_5);
            }
        "#;
        let tmp = std::env::temp_dir().join("embersim_test_scan.c");
        std::fs::write(&tmp, code).unwrap();
        let (funcs, _, _, _) = scan_source(&tmp).unwrap();
        std::fs::remove_file(&tmp).ok();
        assert!(funcs.contains("HAL_UART_Transmit"));
        assert!(funcs.contains("HAL_TIM_Base_Start_IT"));
        assert!(funcs.contains("HAL_GPIO_ReadPin"));
    }

    #[test]
    fn test_scan_hal_macros() {
        let code = r#"
            __HAL_TIM_SET_AUTORELOAD(&htim, 999);
            __HAL_RCC_GPIOA_CLK_ENABLE();
        "#;
        let tmp = std::env::temp_dir().join("embersim_test_macro.c");
        std::fs::write(&tmp, code).unwrap();
        let (_, macros, _, _) = scan_source(&tmp).unwrap();
        std::fs::remove_file(&tmp).ok();
        assert!(macros.contains("__HAL_TIM_SET_AUTORELOAD"));
        assert!(macros.contains("__HAL_RCC_GPIOA_CLK_ENABLE"));
    }

    #[test]
    fn test_scan_hal_types() {
        let code = r#"
            static TIM_HandleTypeDef htim2;
            UART_HandleTypeDef huart;
            GPIO_InitTypeDef gpio_init;
        "#;
        let tmp = std::env::temp_dir().join("embersim_test_type.c");
        std::fs::write(&tmp, code).unwrap();
        let (_, _, types, _) = scan_source(&tmp).unwrap();
        std::fs::remove_file(&tmp).ok();
        assert!(types.contains("TIM_HandleTypeDef"));
        assert!(types.contains("UART_HandleTypeDef"));
        assert!(types.contains("GPIO_InitTypeDef"));
    }

    #[test]
    fn test_scan_includes() {
        let code = r#"
            #include "stm32f4xx_hal.h"
            #include "motor_control.h"
            #include <stdint.h>
        "#;
        let tmp = std::env::temp_dir().join("embersim_test_inc.c");
        std::fs::write(&tmp, code).unwrap();
        let (_, _, _, includes) = scan_source(&tmp).unwrap();
        std::fs::remove_file(&tmp).ok();
        assert!(includes.contains("stm32f4xx_hal.h"));
        assert!(includes.contains("motor_control.h"));
        assert!(!includes.contains("<stdint.h>"));
    }

    #[test]
    fn test_strip_comments() {
        let code = r#"
            /* HAL_UART_Transmit(&huart, data, 5, 100); */
            HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, SET);
            // HAL_TIM_Base_Start_IT(&htim);
        "#;
        let stripped = strip_c_comments(code);
        assert!(!stripped.contains("HAL_UART_Transmit")); // commented out
        assert!(stripped.contains("HAL_GPIO_WritePin"));  // not commented
        assert!(!stripped.contains("HAL_TIM_Base_Start_IT")); // commented out
    }
}
