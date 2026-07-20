/* motor_control.c — Motor controller firmware
   Uses TIM2 PWM for speed control and USART2 for telemetry.
   This is production-style firmware that happens to use the STM32 HAL. */

#include "mock_hal.h"
#include "mock_tim.h"
#include "motor_control.h"
#include <stdio.h>
#include <string.h>

static TIM_HandleTypeDef htim2;
static UART_HandleTypeDef huart2;
static uint32_t motor_speed_rpm = 0;
static uint32_t tick_count = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == 0x40000400) {
        tick_count++;

        /* Simulate speed measurement: ~1200 RPM */
        motor_speed_rpm = 1200 + (tick_count % 10);

        /* Transmit telemetry every 50ms */
        if (tick_count % 50 == 0) {
            char msg[32];
            int len = snprintf(msg, sizeof(msg), "RPM:%lu\n", motor_speed_rpm);
            HAL_UART_Transmit(&huart2, (uint8_t *)msg, len, 100);
        }
    }
}

void app_init(void) {
    /* Initialize peripherals through HAL */
    htim2.Instance = 0x40000400;
    HAL_TIM_Base_Init(&htim2);
    mock_tim_set_period_ticks(0x40000400, 1000);  /* 1ms period */
    HAL_TIM_Base_Start_IT(&htim2);

    huart2.Instance = 0x40004400;
    HAL_UART_Init(&huart2);
}

void app_run(void) {
    /* Main loop — idle for this demo */
}
