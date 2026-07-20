/* main.c — What an external engineer naturally writes.
   Uses standard STM32 HAL patterns. No EmberSim-specific knowledge. */

#include "stm32f4xx_hal.h"
#include <string.h>

static TIM_HandleTypeDef htim2;
static UART_HandleTypeDef huart2;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == 0x40000400) {
        char msg[] = "OK\n";
        HAL_UART_Transmit(&huart2, (uint8_t *)msg, 4, 100);
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
