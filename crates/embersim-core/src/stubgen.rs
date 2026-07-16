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

    // ============================================================
    // mock_hal.h
    // ============================================================
    writeln!(h, "#ifndef MOCK_HAL_H")?;
    writeln!(h, "#define MOCK_HAL_H")?;
    writeln!(h)?;
    writeln!(h, "#include <stdint.h>")?;
    writeln!(h, "#include <stdbool.h>")?;
    writeln!(h, "#include <stddef.h>")?;
    writeln!(h)?;

    writeln!(h, "/* HAL status */")?;
    writeln!(h, "typedef enum {{ HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 }} HAL_StatusTypeDef;")?;
    writeln!(h)?;
    writeln!(h, "/* GPIO types */")?;
    writeln!(h, "typedef struct {{ volatile uint32_t dummy; }} GPIO_TypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} GPIO_InitTypeDef;")?;
    writeln!(h, "typedef enum {{ GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 }} GPIO_PinState;")?;
    writeln!(h)?;
    writeln!(h, "/* UART state machine */")?;
    writeln!(h, "typedef enum {{ HAL_UART_STATE_RESET = 0x00, HAL_UART_STATE_READY = 0x20, HAL_UART_STATE_BUSY = 0x24, HAL_UART_STATE_BUSY_TX = 0x21, HAL_UART_STATE_BUSY_RX = 0x22, HAL_UART_STATE_BUSY_TX_RX = 0x23, HAL_UART_STATE_TIMEOUT = 0xA0, HAL_UART_STATE_ERROR = 0xE0 }} HAL_UART_StateTypeDef;")?;
    writeln!(h, "#define HAL_UART_ERROR_PE  0x00000001U")?;
    writeln!(h, "#define HAL_UART_ERROR_NE  0x00000002U")?;
    writeln!(h, "#define HAL_UART_ERROR_FE  0x00000004U")?;
    writeln!(h, "#define HAL_UART_ERROR_ORE 0x00000008U")?;
    writeln!(h, "#define HAL_UART_ERROR_DMA 0x00000010U")?;
    writeln!(h)?;
    writeln!(h, "/* UART handle — full struct for professional mock */")?;
    writeln!(h, "typedef struct {{ uint32_t Instance; uint32_t Init; uint8_t *pTxBuffPtr; uint16_t TxXferSize; uint16_t TxXferCount; uint8_t *pRxBuffPtr; uint16_t RxXferSize; uint16_t RxXferCount; HAL_UART_StateTypeDef gState; HAL_UART_StateTypeDef RxState; uint32_t ErrorCode; }} UART_HandleTypeDef;")?;
    writeln!(h)?;
    writeln!(h, "/* Other peripheral handles (minimal) */")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} I2C_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} SPI_HandleTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t dummy; }} TIM_HandleTypeDef;")?;
    // ... keep the rest of your handle types ...

    // Forward declarations
    for func in functions {
        writeln!(h, "{};", func.to_c_decl())?;
    }

    writeln!(h)?;
    writeln!(h, "#endif /* MOCK_HAL_H */")?;

    // ============================================================
    // mock_hal.c
    // ============================================================
    writeln!(c, "#include \"mock_hal.h\"")?;
    writeln!(c, "#include <stdio.h>")?;
    writeln!(c, "#include \"trace_log.h\"")?;
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
    // Copy mock_state.c template if it exists
    // ============================================================
    let template_dir = Path::new(env!("CARGO_MANIFEST_DIR")).join("templates");
    let state_template = template_dir.join("mock_state.c");
    if state_template.exists() {
        let dest = output_dir.join("mock_state.c");
        std::fs::copy(&state_template, &dest)?;
    }

    Ok(())
}