#include <stdio.h>
#include "mock_hal.h"
#include "mock_tim.h"
#include "ember_sim_kernel.h"
#include "trace_log.h"

#define TIM2_IRQn  28   // <-- add this line

extern void mock_tim_init(void);

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

int main(void) {
    int failures = 0;
    trace_log_init("trace_tim_runtime.jsonl");
    mock_tim_init();
    kernel_init();
    nvic_enable(TIM2_IRQn);

    TIM_HandleTypeDef htim2 = { .Instance = (uint32_t)0x40000400, .ErrorCode = 0 };
    HAL_TIM_Base_Init(&htim2);

    /* Test 1: period elapsed */
    mock_tim_set_period_ticks(0x40000400, 3);
    period_cb = 0;
    HAL_TIM_Base_Start_IT(&htim2);
    kernel_run_until(10);
    if (period_cb < 1) {
        printf("FAIL: Period elapsed (got %d)\n", period_cb);
        failures++;
    } else printf("PASS: Period elapsed via runtime (%d callbacks)\n", period_cb);

    /* Test 2: PWM */
    pwm_cb = 0;
    HAL_TIM_PWM_Start(&htim2, 1);
    kernel_run_until(20);
    if (pwm_cb < 1) {
        printf("FAIL: PWM pulse finished\n");
        failures++;
    } else printf("PASS: PWM pulse finished via runtime\n");

    /* Test 3: input capture *//* Stop PWM before input capture test */
    /* Stop PWM before input capture test */
    HAL_TIM_PWM_Stop(&htim2, 1);
    HAL_TIM_Base_Stop_IT(&htim2);


    ic_cb = 0;
    mock_tim_inject_capture(0x40000400, 2, 12345);
    HAL_TIM_IC_Start_IT(&htim2, 2);
    kernel_run_until(30);
    if (ic_cb < 1) {
        printf("FAIL: Input capture\n");
        failures++;
    } else printf("PASS: Input capture via runtime\n");

    /* Test 4: hardware state before dispatch */
    {
        HAL_TIM_Base_Stop_IT(&htim2);
        mock_tim_set_period_ticks(0x40000400, 3);
        uint32_t sr_before = mock_tim_get_sr(0x40000400);
        HAL_TIM_Base_Start_IT(&htim2);
        kernel_advance_ticks(10);   // timer runs, events queued, IRQ NOT yet serviced
        uint32_t sr_after = mock_tim_get_sr(0x40000400);
        if (sr_after & 0x0001) {
            printf("PASS: SR UIF set after tick (before dispatch)\n");
        } else {
            printf("FAIL: SR UIF not set (before=%04X after=%04X)\n", sr_before, sr_after);
            failures++;
        }
    }

    /* Test 5: dispatch clears UIF and invokes callback */
    {
        period_cb = 0;
        kernel_dispatch_pending();  // now process events → IRQ handler clears UIF
        uint32_t sr_final = mock_tim_get_sr(0x40000400);
        if (sr_final & 0x0001) {
            printf("FAIL: SR UIF still set after dispatch\n");
            failures++;
        } else if (period_cb < 1) {
            printf("FAIL: callback not invoked after dispatch\n");
            failures++;
        } else {
            printf("PASS: UIF cleared and period callback invoked after dispatch\n");
        }
    }

    /* Test 6: No callback when UIE disabled */
    {
        HAL_TIM_Base_Stop_IT(&htim2);
        HAL_TIM_Base_Start(&htim2);  // start without enabling interrupts
        period_cb = 0;
        kernel_run_until(50);
        if (period_cb != 0) {
            printf("FAIL: Callback fired with UIE disabled\n");
            failures++;
        } else printf("PASS: No callback when UIE disabled\n");
        HAL_TIM_Base_Stop(&htim2);
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