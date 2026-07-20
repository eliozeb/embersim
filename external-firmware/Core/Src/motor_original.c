/* motor_original.c — What an external engineer naturally writes before discovering EmberSim issues.
   Uses real STM32 HAL patterns: stm32f4xx_hal.h include, __HAL_TIM_SET_AUTORELOAD macro. */

#include "stm32f4xx_hal.h"
#include "motor_control.h"
#include <string.h>

static TIM_HandleTypeDef htim2;
static UART_HandleTypeDef huart2;
static uint32_t motor_speed_rpm = 0;
static uint32_t tick_count = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == 0x40000400) {
        tick_count++;
        motor_speed_rpm = 1200 + (tick_count % 10);

        if (tick_count % 50 == 0) {
            char msg[32];
            int len = snprintf(msg, sizeof(msg), "RPM:%lu\n", motor_speed_rpm);
            HAL_UART_Transmit(&huart2, (uint8_t *)msg, len, 100);
        }
    }
}

void app_init(void) {
    htim2.Instance = 0x40000400;
    HAL_TIM_Base_Init(&htim2);
    __HAL_TIM_SET_AUTORELOAD(&htim2, 999);
    HAL_TIM_Base_Start_IT(&htim2);

    huart2.Instance = 0x40004400;
    HAL_UART_Init(&huart2);
}

void app_run(void) {}
