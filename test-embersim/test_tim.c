#include <stdio.h>
#include "mock_hal.h"
#include "mock_tim.h"
#include "ember_sim_scheduler.h"
#include "trace_log.h"

extern void mock_tim_init(void);
extern void HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim);  // ← add this

static int period_cb = 0, pwm_cb = 0, ic_cb = 0;

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    period_cb++;
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_PeriodElapsedCallback", payload);
}
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    pwm_cb++;
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_PWM_PulseFinishedCallback", payload);
}
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    ic_cb++;
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_IC_CaptureCallback", payload);
}

void HAL_IRQ_Handler(IRQn_Type irq) {
    if (irq == TIM2_IRQn) {
        TIM_HandleTypeDef htim2 = { .Instance = (uint32_t)0x40000400, .ErrorCode = 0 };
        HAL_TIM_IRQHandler(&htim2);
    }
}

int main(void) {
    int failures = 0;
    trace_log_init("trace_tim_sched.jsonl");
    mock_tim_init();
    ember_sim_init(0);

    TIM_HandleTypeDef htim2 = { .Instance = (uint32_t)0x40000400, .ErrorCode = 0 };

    /* Test 1: Period elapsed via scheduler */
    mock_tim_set_period_ticks(0x40000400, 3);
    HAL_TIM_Base_Start_IT(&htim2);
    period_cb = 0;
    ember_sim_run_until(10);
    if (period_cb < 1) { printf("FAIL: Period elapsed callback\n"); failures++; }
    else printf("PASS: Period elapsed callback via scheduler\n");

    /* Test 2: PWM pulse finished */
    pwm_cb = 0;
    HAL_TIM_PWM_Start(&htim2, 1);
    ember_sim_run_until(20);
    if (pwm_cb < 1) { printf("FAIL: PWM pulse finished callback\n"); failures++; }
    else printf("PASS: PWM pulse finished callback\n");

    /* Test 3: Input capture */
    ic_cb = 0;
    mock_tim_inject_capture(0x40000400, 2, 12345);
    HAL_TIM_IC_Start_IT(&htim2, 2);
    ember_sim_run_until(30);
    if (ic_cb < 1) { printf("FAIL: Input capture callback\n"); failures++; }
    else printf("PASS: Input capture callback\n");

    trace_log_close();
    FILE *f = fopen("trace_tim_sched.jsonl", "r");
    if (f) { char line[256]; printf("\n--- Trace ---\n"); while (fgets(line,sizeof(line),f)) printf("%s",line); fclose(f); }

    if (!failures) printf("\nALL TIMER SCHEDULER TESTS PASSED\n");
    else printf("\n%d TEST(S) FAILED\n", failures);
    return failures;
}