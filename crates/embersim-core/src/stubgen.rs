use anyhow::Result;
use std::io::Write;
use std::path::Path;
use crate::model::HalFunction;

pub fn generate(functions: &[HalFunction], output_dir: &Path) -> Result<()> {
    std::fs::create_dir_all(output_dir)?;
    
    let header_path = output_dir.join("mock_hal.h");
    let impl_path = output_dir.join("mock_hal.c");
    
    let mut h = std::fs::File::create(&header_path)?;
    let mut c = std::fs::File::create(&impl_path)?;
    
    // Header guard and includes
    writeln!(h, "#ifndef MOCK_HAL_H")?;
    writeln!(h, "#define MOCK_HAL_H")?;
    writeln!(h, "#include <stdint.h>")?;
    writeln!(h, "#include <stdbool.h>")?;
    writeln!(h, "#include <stddef.h>")?;
    writeln!(h)?;
    
    // Minimal type definitions (these will be expanded in Day 4 with CMSIS shim)
    writeln!(h, "typedef enum {{ HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 }} HAL_StatusTypeDef;")?;
    writeln!(h, "typedef struct {{ volatile uint32_t dummy; }} GPIO_TypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} GPIO_InitTypeDef;")?;
    writeln!(h, "typedef enum {{ GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 }} GPIO_PinState;")?;
    writeln!(h, "typedef struct {{ uint32_t Instance; }} UART_HandleTypeDef;")?;
    writeln!(h)?;
    
    // Forward declarations
    for func in functions {
        writeln!(h, "{};", func.to_c_decl())?;
    }
    
    writeln!(h)?;
    writeln!(h, "#endif /* MOCK_HAL_H */")?;
    
    // Implementation file
    writeln!(c, "#include \"mock_hal.h\"")?;
    writeln!(c, "#include <stdio.h>")?;
    writeln!(c)?;
    writeln!(c, "static void trace_log(const char *func, const char *fmt, ...) {{")?;
    writeln!(c, "    (void)func; (void)fmt;")?;
    writeln!(c, "    /* TODO: implement trace logging in Day 5 */")?;
    writeln!(c, "}}")?;
    writeln!(c)?;
    
    for func in functions {
        writeln!(c, "{}", func.to_c_decl())?;
        writeln!(c, "{{")?;
        
        // Generate trace_log call with parameter names
        let param_names: Vec<String> = func.params.iter().map(|p| p.name.clone()).collect();
        if !param_names.is_empty() {
            writeln!(c, "    trace_log(\"{}\", \"{}\");", func.name, param_names.join(","))?;
        } else {
            writeln!(c, "    trace_log(\"{}\", \"\");", func.name)?;
        }
        
        // Return default value based on return type
        let ret = match func.return_type.as_str() {
            "HAL_StatusTypeDef" => "    return HAL_OK;",
            "void" => "    /* void */",
            t if t.contains('*') => "    return NULL;",
            t if t.starts_with("uint") || t.starts_with("int") || t == "uint32_t" || t == "uint16_t" || t == "uint8_t" || t == "int32_t" || t == "int16_t" || t == "int8_t" => "    return 0;",
            _ => "    return 0; /* default */",
        };
        writeln!(c, "{}", ret)?;
        writeln!(c, "}}")?;
        writeln!(c)?;
    }
    
    Ok(())
}