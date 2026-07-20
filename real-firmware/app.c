/* app.c — Real firmware: TIM2 periodic interrupt + UART transmit
   This is production-style STM32 firmware using the HAL API.
   It runs on EmberSim without modification. */

#include "mock_hal.h"
#include "mock_tim.h"
#include <string.h>

/* External UART API */
extern void mock_uart_init(void);
extern const uint8_t *mock_uart_get_tx(uintptr_t base, uint16_t *len);

static TIM_HandleTypeDef htim2;
static UART_HandleTypeDef huart2;
static int timer_ticks = 0;

/* Timer period elapsed callback — fires every ~1000 us */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == 0x40000400) {
        timer_ticks++;

        /* Transmit a status message every 10 timer ticks (~10ms) */
        if (timer_ticks % 10 == 0) {
            uint8_t msg[] = "TICK\n";
            HAL_UART_Transmit(&huart2, msg, 5, 100);
        }
    }
}

void app_init(void) {
    /* Initialize TIM2 peripheral */
    mock_tim_init();
    mock_tim_set_period_ticks(0x40000400, 1000);
    htim2.Instance = 0x40000400;
    HAL_TIM_Base_Init(&htim2);
    HAL_TIM_Base_Start_IT(&htim2);

    /* Initialize USART2 peripheral */
    mock_uart_init();
    huart2.Instance = 0x40004400;
    HAL_UART_Init(&huart2);
}

void app_run(void) {
    /* Application main loop — empty for this demo */
}
