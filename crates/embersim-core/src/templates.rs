pub const TRACE_LOG_H: &[u8] = include_bytes!("templates/trace_log.h");
pub const TRACE_LOG_C: &[u8] = include_bytes!("templates/trace_log.c");
pub const MOCK_STATE_C: &[u8] = include_bytes!("templates/mock_state.c");
pub const MOCK_UART_C: &[u8] = include_bytes!("templates/mock_uart.c");
pub const HOST_MAIN_C: &[u8] = include_bytes!("templates/host_main.c");
pub const HOST_CMAKE: &[u8] = include_bytes!("templates/host_CMakeLists.txt.hbs");  
pub const MOCK_I2C_C: &[u8] = include_bytes!("templates/mock_i2c.c");// new
pub const MOCK_SPI_H: &[u8] = include_bytes!("templates/mock_spi.h");
pub const MOCK_SPI_C: &[u8] = include_bytes!("templates/mock_spi.c");

pub const MOCK_TIM_H: &[u8] = include_bytes!("templates/mock_tim.h");
pub const MOCK_TIM_C: &[u8] = include_bytes!("templates/mock_tim.c");

pub const EMBER_SIM_SCHEDULER_H: &[u8] = include_bytes!("templates/ember_sim_scheduler.h");
pub const EMBER_SIM_SCHEDULER_C: &[u8] = include_bytes!("templates/ember_sim_scheduler.c");

pub const EMBER_SIM_RUNTIME_H: &[u8] = include_bytes!("templates/ember_sim_runtime.h");
pub const EMBER_SIM_RUNTIME_C: &[u8] = include_bytes!("templates/ember_sim_runtime.c");

pub const TIM_REGS_H: &[u8] = include_bytes!("templates/tim_regs.h");
pub const TIM_REGS_C: &[u8] = include_bytes!("templates/tim_regs.c");