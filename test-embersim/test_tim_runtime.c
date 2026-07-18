#include <stdio.h>
#include "mock_hal.h"
#include "mock_tim.h"
#include "tim_regs.h"
#include "ember_sim_runtime.h"
#include "trace_log.h"

extern void mock_tim_init(void);
extern void HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim);

static int period_cb = 0;
static int pwm_cb = 0;
static int ic_cb = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    period_cb++;
    char details[128];
    snprintf(details, sizeof(details),
             "{\"instance\":\"TIM2\",\"address\":\"0x%08X\"}", (uint32_t)htim->Instance);
    trace_software_event("TIM", "period_elapsed", details);
}
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    pwm_cb++;
    char details[128];
    snprintf(details, sizeof(details),
             "{\"instance\":\"TIM2\",\"address\":\"0x%08X\"}", (uint32_t)htim->Instance);
    trace_software_event("TIM", "pwm_pulse_finished", details);
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    ic_cb++;
    char details[128];
    snprintf(details, sizeof(details),
             "{\"instance\":\"TIM2\",\"address\":\"0x%08X\"}", (uint32_t)htim->Instance);
    trace_software_event("TIM", "ic_capture", details);
}

void HAL_IRQ_Dispatch(IRQn_Type irq) {
    if (irq == TIM2_IRQn) {
        TIM_HandleTypeDef htim2 = { .Instance = (uint32_t)0x40000400, .ErrorCode = 0 };
        HAL_TIM_IRQHandler(&htim2);
    }
}

int main(void) {
    int failures = 0;
    trace_log_init("trace_tim_runtime.jsonl");
    mock_tim_init();
    ember_runtime_init(0);
    NVIC_EnableIRQ(TIM2_IRQn);

    TIM_HandleTypeDef htim2 = { .Instance = (uint32_t)0x40000400, .ErrorCode = 0 };
    HAL_TIM_Base_Init(&htim2);  /* ← MUST BE CALLED */

    /* Test 1: period elapsed */
    mock_tim_set_period_ticks(0x40000400, 3);
    period_cb = 0;
    HAL_TIM_Base_Start_IT(&htim2);
    ember_runtime_run_until(10);
    if (period_cb < 1) {
        printf("FAIL: Period elapsed (got %d, expected >= 1)\n", period_cb);
        failures++;
    } else {
        printf("PASS: Period elapsed via runtime (%d callbacks)\n", period_cb);
    }

    /* Test 2: PWM */
    pwm_cb = 0;
    HAL_TIM_PWM_Start(&htim2, 1);
    ember_runtime_run_until(20);
    if (pwm_cb < 1) {
        printf("FAIL: PWM pulse finished\n");
        failures++;
    } else {
        printf("PASS: PWM pulse finished via runtime\n");
    }

    /* Test 3: input capture */
    ic_cb = 0;
    mock_tim_inject_capture(0x40000400, 2, 12345);
    HAL_TIM_IC_Start_IT(&htim2, 2);
    ember_runtime_run_until(30);
    if (ic_cb < 1) {
        printf("FAIL: Input capture\n");
        failures++;
    } else {
        printf("PASS: Input capture via runtime\n");
    }

    trace_log_close();
    FILE *f = fopen("trace_tim_runtime.jsonl", "r");
    if (f) {
        char line[256];
        printf("\n--- Trace ---\n");
        while (fgets(line, sizeof(line), f)) printf("%s", line);
        fclose(f);
    }

    if (!failures) printf("\nALL RUNTIME TESTS PASSED\n");
    else printf("\n%d TEST(S) FAILED\n", failures);
    return failures;
}