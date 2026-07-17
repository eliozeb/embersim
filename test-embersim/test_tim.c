#include <stdio.h>
#include "mock_hal.h"
#include "mock_tim.h"
#include "trace_log.h"

extern void mock_tim_init(void);
extern void ember_sim_tim_tick(void);

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

int main(void) {
    int failures = 0;
    trace_log_init("trace_tim.jsonl");
    mock_tim_init();

    TIM_HandleTypeDef htim2 = { .Instance = (uint32_t)0x40000400, .ErrorCode = 0 };

    mock_tim_set_period_ticks(0x40000400, 3);
    HAL_TIM_Base_Start_IT(&htim2);
    period_cb = 0;
    for (int i = 0; i < 5; i++) ember_sim_tim_tick();
    if (period_cb < 1) { printf("FAIL: Period elapsed callback\n"); failures++; }
    else printf("PASS: Period elapsed callback after 3 ticks\n");

    pwm_cb = 0;
    HAL_TIM_PWM_Start(&htim2, 1);
    for (int i = 0; i < 5; i++) ember_sim_tim_tick();
    if (pwm_cb < 1) { printf("FAIL: PWM pulse finished callback\n"); failures++; }
    else printf("PASS: PWM pulse finished callback\n");

    ic_cb = 0;
    mock_tim_inject_capture(0x40000400, 2, 12345);
    HAL_TIM_IC_Start_IT(&htim2, 2);
    ember_sim_tim_tick();
    if (ic_cb < 1) { printf("FAIL: Input capture callback\n"); failures++; }
    else printf("PASS: Input capture callback\n");

    trace_log_close();
    FILE *f = fopen("trace_tim.jsonl", "r");
    if (f) { char line[256]; printf("\n--- Trace ---\n"); while (fgets(line,sizeof(line),f)) printf("%s",line); fclose(f); }

    if (!failures) printf("\nALL TIMER TESTS PASSED\n");
    else printf("\n%d TEST(S) FAILED\n", failures);
    return failures;
}