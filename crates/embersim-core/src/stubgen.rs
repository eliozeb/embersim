use anyhow::Result;
use std::io::Write;
use std::path::Path;
use crate::model::HalFunction;

/// Returns true if the function should NOT have a stub generated.
/// This includes functions provided by peripheral models AND callbacks
/// that the firmware defines (HAL_*_Callback, HAL_*_IRQHandler).
fn skip_stub(func_name: &str) -> bool {
    // Callbacks and IRQ handlers are defined by the firmware, not the mock
    if func_name.contains("Callback") || func_name.contains("IRQHandler") {
        return true;
    }
    has_peripheral_model(func_name)
}

/// Returns true if the function is provided by a dedicated peripheral model
/// (.c file in the mocks directory with a strong implementation). These
/// functions do NOT need a stub in mock_hal.c — the peripheral model .o
/// provides the real implementation.
fn has_peripheral_model(func_name: &str) -> bool {
    // UART functions → mock_uart.c
    if func_name.starts_with("HAL_UART_") {
        return true;
    }
    // TIM functions → mock_tim.c
    // mock_tim.c provides: Base_Init, Base_Start, Base_Stop, Base_Start_IT,
    // Base_Stop_IT, PWM_Start, PWM_Stop, IC_Init, IC_Start_IT, IRQHandler.
    // It does NOT provide: PWM_Init, PWM_ConfigChannel, or callbacks
    // (callbacks are filtered by skip_stub before reaching here).
    if func_name.starts_with("HAL_TIM_") {
        if func_name.contains("_PWM_Init") || func_name.contains("_PWM_ConfigChannel") {
            return false;
        }
        return true;
    }
    // SPI functions → mock_spi.c (except Init)
    if func_name.starts_with("HAL_SPI_") {
        if func_name.contains("_Init") {
            return false;
        }
        return true;
    }
    // GPIO Write/Read/Toggle → mock_state.c (Init, DeInit, LockPin, EXTI need stubs)
    if func_name.starts_with("HAL_GPIO_") {
        if func_name.contains("_WritePin") || func_name.contains("_ReadPin")
            || func_name.contains("_TogglePin")
        {
            return true;
        }
        return false;
    }
    // I2C functions → mock_i2c.c
    if func_name.starts_with("HAL_I2C_") {
        return true;
    }
    false
}

/// Generate mock_hal.h and mock_hal.c from parsed HAL functions.
pub fn generate(functions: &[HalFunction], output_dir: &Path) -> Result<()> {
    std::fs::create_dir_all(output_dir)?;

    let header_path = output_dir.join("mock_hal.h");
    let impl_path = output_dir.join("mock_hal.c");

    let mut h = std::fs::File::create(&header_path)?;
    let mut c = std::fs::File::create(&impl_path)?;

    // ============================================================
    // mock_hal.h — all types defined BEFORE any function declarations
    // ============================================================
    writeln!(h, "#ifndef MOCK_HAL_H")?;
    writeln!(h, "#define MOCK_HAL_H")?;
    writeln!(h)?;
    writeln!(h, "#include <stdint.h>")?;
    writeln!(h, "#include <stdbool.h>")?;
    writeln!(h, "#include <stddef.h>")?;
    writeln!(h)?;

    // --- Base types ---
    writeln!(h, "/* HAL status */")?;
    writeln!(h, "typedef enum {{ HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 }} HAL_StatusTypeDef;")?;
    writeln!(h)?;
    writeln!(h, "/* Lock type */")?;
    writeln!(h, "typedef enum {{ HAL_UNLOCKED = 0x00U, HAL_LOCKED = 0x01U }} HAL_LockTypeDef;")?;
    writeln!(h)?;

    // --- GPIO types ---
    writeln!(h, "/* GPIO types */")?;
    writeln!(h, "typedef struct {{ volatile uint32_t dummy; }} GPIO_TypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t Pin; uint32_t Mode; uint32_t Pull; uint32_t Speed; uint32_t Alternate; }} GPIO_InitTypeDef;")?;
    writeln!(h, "typedef enum {{ GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 }} GPIO_PinState;")?;
    writeln!(h)?;

    // --- UART types ---
    writeln!(h, "/* UART types */")?;
    writeln!(h, "typedef struct {{ uint32_t BaudRate; uint32_t WordLength; uint32_t StopBits; uint32_t Parity; uint32_t Mode; uint32_t HwFlowCtl; uint32_t OverSampling; }} UART_InitTypeDef;")?;
    writeln!(h, "typedef enum {{ HAL_UART_STATE_RESET = 0x00, HAL_UART_STATE_READY = 0x20, HAL_UART_STATE_BUSY = 0x24, HAL_UART_STATE_BUSY_TX = 0x21, HAL_UART_STATE_BUSY_RX = 0x22, HAL_UART_STATE_BUSY_TX_RX = 0x23, HAL_UART_STATE_TIMEOUT = 0xA0, HAL_UART_STATE_ERROR = 0xE0 }} HAL_UART_StateTypeDef;")?;
    writeln!(h, "#define HAL_UART_ERROR_PE  0x00000001U")?;
    writeln!(h, "#define HAL_UART_ERROR_NE  0x00000002U")?;
    writeln!(h, "#define HAL_UART_ERROR_FE  0x00000004U")?;
    writeln!(h, "#define HAL_UART_ERROR_ORE 0x00000008U")?;
    writeln!(h, "#define HAL_UART_ERROR_DMA 0x00000010U")?;
    writeln!(h, "typedef struct {{ uint32_t Instance; UART_InitTypeDef Init; uint8_t *pTxBuffPtr; uint16_t TxXferSize; uint16_t TxXferCount; uint8_t *pRxBuffPtr; uint16_t RxXferSize; uint16_t RxXferCount; HAL_UART_StateTypeDef gState; HAL_UART_StateTypeDef RxState; HAL_LockTypeDef Lock; uint32_t ErrorCode; }} UART_HandleTypeDef;")?;
    writeln!(h)?;

    // --- TIM types ---
    writeln!(h, "/* TIM types */")?;
    writeln!(h, "typedef struct {{ uint32_t Prescaler; uint32_t CounterMode; uint32_t Period; uint32_t ClockDivision; uint32_t RepetitionCounter; uint32_t AutoReloadPreload; }} TIM_Base_InitTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t OCMode; uint32_t Pulse; uint32_t OCPolarity; uint32_t OCNPolarity; uint32_t OCFastMode; uint32_t OCIdleState; uint32_t OCNIdleState; }} TIM_OC_InitTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t Instance; TIM_Base_InitTypeDef Init; uint32_t Channel; HAL_LockTypeDef Lock; uint32_t State; uint32_t ErrorCode; }} TIM_HandleTypeDef;")?;
    writeln!(h)?;

    // --- SPI types ---
    writeln!(h, "/* SPI types */")?;
    writeln!(h, "typedef struct {{ uint32_t Mode; uint32_t Direction; uint32_t DataSize; uint32_t CLKPolarity; uint32_t CLKPhase; uint32_t NSS; uint32_t BaudRatePrescaler; uint32_t FirstBit; uint32_t TIMode; uint32_t CRCCalculation; uint32_t CRCPolynomial; }} SPI_InitTypeDef;")?;
    writeln!(h, "#define HAL_SPI_ERROR_NONE  0x00000000U")?;
    writeln!(h, "#define HAL_SPI_ERROR_CRC   0x00000001U")?;
    writeln!(h, "#define HAL_SPI_ERROR_OVR   0x00000002U")?;
    writeln!(h, "#define HAL_SPI_ERROR_DMA   0x00000010U")?;
    writeln!(h, "typedef struct {{ uint32_t Instance; SPI_InitTypeDef Init; uint8_t *pTxBuffPtr; uint16_t TxXferSize; uint16_t TxXferCount; uint8_t *pRxBuffPtr; uint16_t RxXferSize; uint16_t RxXferCount; HAL_LockTypeDef Lock; uint32_t ErrorCode; }} SPI_HandleTypeDef;")?;
    writeln!(h)?;

    // --- RCC types ---
    writeln!(h, "/* RCC types */")?;
    writeln!(h, "typedef struct {{ uint32_t OscillatorType; uint32_t HSEState; uint32_t LSEState; uint32_t HSIState; uint32_t HSICalibrationValue; uint32_t LSIState; uint32_t PLL; uint32_t PLLSource; uint32_t PLLM; uint32_t PLLN; uint32_t PLLP; uint32_t PLLQ; uint32_t SYSCLKSource; uint32_t AHBCLKDivider; uint32_t APB1CLKDivider; uint32_t APB2CLKDivider; }} RCC_OscInitTypeDef;")?;
    writeln!(h, "typedef struct {{ uint32_t ClockType; uint32_t SYSCLKSource; uint32_t AHBCLKDivider; uint32_t APB1CLKDivider; uint32_t APB2CLKDivider; }} RCC_ClkInitTypeDef;")?;
    writeln!(h)?;

    // --- NVIC types ---
    writeln!(h, "/* NVIC types */")?;
    writeln!(h, "typedef enum {{ EXTI0_IRQn = 6, EXTI1_IRQn = 7, USART2_IRQn = 38, TIM2_IRQn = 28, SPI1_IRQn = 35 }} IRQn_Type;")?;
    writeln!(h)?;

    // --- I2C types ---
    writeln!(h, "/* I2C types */")?;
    writeln!(h, "typedef struct {{ uint32_t Instance; uint32_t Init; uint8_t *pBuffPtr; uint16_t XferSize; uint16_t XferCount; uint32_t ErrorCode; }} I2C_HandleTypeDef;")?;
    writeln!(h, "#define HAL_I2C_ERROR_NONE  0x00000000U")?;
    writeln!(h, "#define HAL_I2C_ERROR_BERR  0x00000001U")?;
    writeln!(h, "#define HAL_I2C_ERROR_ARLO  0x00000002U")?;
    writeln!(h, "#define HAL_I2C_ERROR_AF    0x00000004U")?;
    writeln!(h, "#define HAL_I2C_ERROR_OVR   0x00000008U")?;
    writeln!(h, "#define HAL_I2C_ERROR_DMA   0x00000010U")?;
    writeln!(h, "#define HAL_I2C_ERROR_TIMEOUT 0x00000020U")?;
    writeln!(h)?;

    // --- TIM error codes ---
    writeln!(h, "/* TIM error codes */")?;
    writeln!(h, "#define HAL_TIM_ERROR_NONE  0x00000000U")?;
    writeln!(h, "#define HAL_TIM_ERROR_TIMEOUT 0x00000001U")?;
    writeln!(h)?;

    // ============================================================
    // Function declarations — generated from parsed HAL header
    // All types used in these declarations are defined above.
    // ============================================================
    writeln!(h, "/* ── HAL function declarations ── */")?;
    for func in functions {
        writeln!(h, "{};", func.to_c_decl())?;
    }

    // Always emit I2C HAL prototypes so professional mocks compile
    // even when the source HAL header doesn't include them.
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
    // mock_hal.c — strong stubs only for functions WITHOUT peripheral models
    // ============================================================
    writeln!(c, "#include \"mock_hal.h\"")?;
    writeln!(c, "#include <stdio.h>")?;
    writeln!(c, "#include \"trace_log.h\"")?;
    writeln!(c)?;

    // Generate strong stubs for parsed functions that don't have a peripheral model
    for func in functions {
        if skip_stub(&func.name) {
            continue;
        }
        writeln!(c, "{}", func.to_c_decl())?;
        writeln!(c, "{{")?;

        let param_names: Vec<String> = func.params.iter().map(|p| p.name.clone()).collect();
        if !param_names.is_empty() {
            writeln!(c, "    trace_log(\"{}\", \"{}\");", func.name, param_names.join(","))?;
        } else {
            writeln!(c, "    trace_log(\"{}\", \"\");", func.name)?;
        }

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

    // Always emit weak I2C stubs so linking never fails
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

    // ============================================================
    // Copy mock_state.c template
    // ============================================================
    let template_dir = Path::new(env!("CARGO_MANIFEST_DIR")).join("templates");
    let state_template = template_dir.join("mock_state.c");
    if state_template.exists() {
        let dest = output_dir.join("mock_state.c");
        std::fs::copy(&state_template, &dest)?;
    }

    Ok(())
}
