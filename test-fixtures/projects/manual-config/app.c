/* manual-config test — minimal firmware: TIM2 periodic interrupt only */
#include "mock_hal.h"
#include "mock_tim.h"

static TIM_HandleTypeDef htim2;
static int ticks = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == 0x40000400) ticks++;
}

void app_init(void) {
    mock_tim_init();
    mock_tim_set_period_ticks(0x40000400, 100);
    htim2.Instance = 0x40000400;
    HAL_TIM_Base_Init(&htim2);
    HAL_TIM_Base_Start_IT(&htim2);
}

void app_run(void) {}
