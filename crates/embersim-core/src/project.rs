use anyhow::{ Result};
use std::fs;
use std::path::Path;
use crate::model::HalFunction;
use crate::stubgen;

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