use anyhow::Result;
use std::io::Write;
use std::path::Path;
use crate::model::HalFunction;

/// Generate mock_hal.h and mock_hal.c from parsed HAL functions.
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
    
    // Minimal type definitions for x86 host compilation
    writeln!(h, "typedef enum {{ HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 }} HAL_StatusTypeDef;")?;
    writeln!(h, "typedef struct {{ volatile uint32_t dummy; }} GPIO_TypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} GPIO_InitTypeDef;")?;
    writeln!(h, "typedef enum {{ GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 }} GPIO_PinState;")?;
    writeln!(h, "typedef struct {{ uint32_t Instance; }} UART_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} I2C_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} SPI_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} TIM_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} DMA_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} ADC_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} DAC_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} RTC_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} CAN_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} CRC_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} ETH_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} HRTIM_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} SMBUS_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} WWDG_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} IWDG_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} LPTIM_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} OPAMP_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} COMP_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} CRYP_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} HASH_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} RNG_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} DCMI_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} DMA2D_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} LTDC_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} DSI_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} JPEG_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} MDIOS_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} SWPMI_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} SAI_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} SD_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} MMC_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} NAND_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} NOR_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} SRAM_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} SDRAM_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} CEC_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} QSPI_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} FMPSMBUS_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} HCD_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} PCD_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} IRDA_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} SMARTCARD_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} PCDEx_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} HCDEx_HandleTypeDef;")?;
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
        // Emit as weak so mock_state.c can override with strong symbols
        writeln!(c, "__attribute__((weak)) {}", func.to_c_decl())?;
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

        // ============================================================
    // INSERT THE TEMPLATE COPY HERE
    // ============================================================
    let template_dir = Path::new(env!("CARGO_MANIFEST_DIR")).join("templates");
    let state_template = template_dir.join("mock_state.c");
    if state_template.exists() {
        let dest = output_dir.join("mock_state.c");
        std::fs::copy(&state_template, &dest)?;
    }
    // ============================================================
    
    Ok(())
}