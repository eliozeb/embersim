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
    writeln!(h)?;
    writeln!(h, "/* I2C error codes */")?;
    writeln!(h, "#define HAL_I2C_ERROR_NONE  0x00000000U")?;
    writeln!(h, "#define HAL_I2C_ERROR_BERR  0x00000001U")?;
    writeln!(h, "#define HAL_I2C_ERROR_ARLO  0x00000002U")?;
    writeln!(h, "#define HAL_I2C_ERROR_AF    0x00000004U")?;
    writeln!(h, "#define HAL_I2C_ERROR_OVR   0x00000008U")?;
    writeln!(h, "#define HAL_I2C_ERROR_DMA   0x00000010U")?;
    writeln!(h, "#define HAL_I2C_ERROR_TIMEOUT 0x00000020U")?;

    writeln!(h, "/* Other peripheral handles (minimal) */")?;
    writeln!(h, "/* I2C handle — full struct for professional mock */")?;
    writeln!(h, "typedef struct {{ uint32_t Instance; uint32_t Init; uint8_t *pBuffPtr; uint16_t XferSize; uint16_t XferCount; uint32_t ErrorCode; }} I2C_HandleTypeDef;")?;
    //writeln!(h, "typedef struct {{ uint32_t dummy; }} SPI_HandleTypeDef;")?;
    //writeln!(h, "typedef struct {{ uint32_t dummy; }} TIM_HandleTypeDef;")?;
    // ... keep the rest of your handle types ...

    // Forward declarations (parsed HAL functions)
    for func in functions {
        writeln!(h, "{};", func.to_c_decl())?;
    }

    // ------------------------------------------------------------------
    // Always emit full I2C HAL prototypes so professional mocks compile
    // even when the source HAL header doesn't include them.
    // ------------------------------------------------------------------
    writeln!(h)?;
    writeln!(h, "/* I2C master functions (always provided by EmberSim) */")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Master_Transmit_IT(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Master_Receive_IT(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Master_Receive_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size, uint32_t Timeout);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size, uint32_t Timeout);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Mem_Write_IT(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Mem_Write_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_Mem_Read_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout);")?;

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

     // --------------------------------------------------------------
    // Always emit weak I2C stubs so linking never fails
    // --------------------------------------------------------------
    writeln!(c)?;
    writeln!(c, "/* ── I2C master weak stubs (always provided) ── */")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_I2C_Master_Transmit(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{ trace_log(\"HAL_I2C_Master_Transmit\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_I2C_Master_Receive(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{ trace_log(\"HAL_I2C_Master_Receive\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_I2C_Master_Transmit_IT(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size) {{ trace_log(\"HAL_I2C_Master_Transmit_IT\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_I2C_Master_Receive_IT(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size) {{ trace_log(\"HAL_I2C_Master_Receive_IT\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_I2C_Master_Transmit_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size) {{ trace_log(\"HAL_I2C_Master_Transmit_DMA\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_I2C_Master_Receive_DMA(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint8_t* pData, uint16_t Size) {{ trace_log(\"HAL_I2C_Master_Receive_DMA\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_I2C_Mem_Write(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{ trace_log(\"HAL_I2C_Mem_Write\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_I2C_Mem_Read(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{ trace_log(\"HAL_I2C_Mem_Read\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_I2C_Mem_Write_IT(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size) {{ trace_log(\"HAL_I2C_Mem_Write_IT\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_I2C_Mem_Read_IT(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint16_t MemAddress, uint16_t MemAddSize, uint8_t* pData, uint16_t Size) {{ trace_log(\"HAL_I2C_Mem_Read_IT\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_I2C_IsDeviceReady(I2C_HandleTypeDef* hi2c, uint16_t DevAddress, uint32_t Trials, uint32_t Timeout) {{ trace_log(\"HAL_I2C_IsDeviceReady\", \"\"); return HAL_OK; }}")?;
    
    
    writeln!(h)?;
    writeln!(h, "/* SPI error codes */")?;
    writeln!(h, "#define HAL_SPI_ERROR_NONE  0x00000000U")?;
    writeln!(h, "#define HAL_SPI_ERROR_CRC   0x00000001U")?;
    writeln!(h, "#define HAL_SPI_ERROR_OVR   0x00000002U")?;
    writeln!(h, "#define HAL_SPI_ERROR_DMA   0x00000010U")?;


    writeln!(h, "/* SPI handle — full struct for professional mock */")?;
    writeln!(h, "typedef struct {{ uint32_t Instance; uint32_t Init; uint8_t *pTxBuffPtr; uint16_t TxXferSize; uint16_t TxXferCount; uint8_t *pRxBuffPtr; uint16_t RxXferSize; uint16_t RxXferCount; uint32_t ErrorCode; }} SPI_HandleTypeDef;")?;


    writeln!(h)?;
    writeln!(h, "/* SPI master functions (always provided by EmberSim) */")?;
    writeln!(h, "HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size, uint32_t Timeout);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size, uint32_t Timeout);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size, uint32_t Timeout);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_SPI_Receive_DMA(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size);")?;


    writeln!(c)?;
    writeln!(c, "/* ── SPI master weak stubs (always provided) ── */")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{ trace_log(\"HAL_SPI_Transmit\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size, uint32_t Timeout) {{ trace_log(\"HAL_SPI_Receive\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size, uint32_t Timeout) {{ trace_log(\"HAL_SPI_TransmitReceive\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size) {{ trace_log(\"HAL_SPI_Transmit_IT\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size) {{ trace_log(\"HAL_SPI_Receive_IT\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size) {{ trace_log(\"HAL_SPI_TransmitReceive_IT\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_SPI_Transmit_DMA(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size) {{ trace_log(\"HAL_SPI_Transmit_DMA\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_SPI_Receive_DMA(SPI_HandleTypeDef* hspi, uint8_t* pData, uint16_t Size) {{ trace_log(\"HAL_SPI_Receive_DMA\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_SPI_TransmitReceive_DMA(SPI_HandleTypeDef* hspi, uint8_t* pTxData, uint8_t* pRxData, uint16_t Size) {{ trace_log(\"HAL_SPI_TransmitReceive_DMA\", \"\"); return HAL_OK; }}")?;


    writeln!(h)?;
    writeln!(h, "/* TIM error codes */")?;
    writeln!(h, "#define HAL_TIM_ERROR_NONE  0x00000000U")?;
    writeln!(h, "#define HAL_TIM_ERROR_TIMEOUT 0x00000001U")?;

    writeln!(h, "/* TIM handle — full struct for professional mock */")?;
    writeln!(h, "typedef struct {{ uint32_t Instance; uint32_t Init; uint32_t Channel; uint32_t Prescaler; uint32_t Period; uint32_t ErrorCode; }} TIM_HandleTypeDef;")?;

    writeln!(h)?;
    writeln!(h, "/* TIM functions (always provided by EmberSim) */")?;
    writeln!(h, "HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef* htim);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef* htim);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef* htim);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef* htim);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef* htim);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef* htim);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef* htim, uint32_t Channel);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef* htim, uint32_t Channel);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_TIM_IC_Init(TIM_HandleTypeDef* htim);")?;
    writeln!(h, "HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef* htim, uint32_t Channel);")?;


    writeln!(c)?;
    writeln!(c, "/* ── TIM weak stubs (always provided) ── */")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef* htim) {{ trace_log(\"HAL_TIM_Base_Init\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef* htim) {{ trace_log(\"HAL_TIM_Base_Start\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef* htim) {{ trace_log(\"HAL_TIM_Base_Stop\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef* htim) {{ trace_log(\"HAL_TIM_Base_Start_IT\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef* htim) {{ trace_log(\"HAL_TIM_Base_Stop_IT\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef* htim) {{ trace_log(\"HAL_TIM_PWM_Init\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef* htim, uint32_t Channel) {{ trace_log(\"HAL_TIM_PWM_Start\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef* htim, uint32_t Channel) {{ trace_log(\"HAL_TIM_PWM_Stop\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_TIM_IC_Init(TIM_HandleTypeDef* htim) {{ trace_log(\"HAL_TIM_IC_Init\", \"\"); return HAL_OK; }}")?;
    writeln!(c, "__attribute__((weak)) HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef* htim, uint32_t Channel) {{ trace_log(\"HAL_TIM_IC_Start_IT\", \"\"); return HAL_OK; }}")?;

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