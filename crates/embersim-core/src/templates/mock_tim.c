/* mock_tim.c — Timer mock using register model and runtime */
#include "mock_hal.h"
#include "mock_tim.h"
#include "tim_regs.h"
#include "ember_sim_runtime.h"
#include "trace_log.h"
#include <string.h>
#include <stdio.h>

/* Forward declarations */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim);

/* PWM active tracking (per timer base address) */
static uint32_t pwm_active_bases[4];
static bool     pwm_active_channels[4][4];  // [base_index][channel]

static int active_timer_count = 0;
static uint32_t active_timers[4];

static int find_timer_index(uint32_t base) {
    for (int i = 0; i < active_timer_count; i++)
        if (active_timers[i] == base) return i;
    return -1;
}

static void ensure_timer(uint32_t base) {
    if (find_timer_index(base) >= 0) return;
    if (active_timer_count < 4) {
        active_timers[active_timer_count] = base;
        pwm_active_bases[active_timer_count] = base;
        for (int c = 0; c < 4; c++) pwm_active_channels[active_timer_count][c] = false;
        active_timer_count++;
    }
}

static bool is_pwm_active(uint32_t base) {
    int idx = find_timer_index(base);
    if (idx < 0) return false;
    for (int c = 0; c < 4; c++) if (pwm_active_channels[idx][c]) return true;
    return false;
}

static void set_pwm_active(uint32_t base, uint32_t channel, bool active) {
    ensure_timer(base);
    int idx = find_timer_index(base);
    if (idx >= 0 && channel < 4) pwm_active_channels[idx][channel] = active;
}

/* Tick callback that advances timer registers and raises IRQs */
static void tim_tick_cb(void) {
    for (int i = 0; i < active_timer_count; i++) {
        tim_regs_tick(active_timers[i]);
        tim_regs_check_irq(active_timers[i]);
    }
}

/* ----- public API (for tests) ----- */
void mock_tim_init(void) {
    active_timer_count = 0;
    memset(pwm_active_channels, 0, sizeof(pwm_active_channels));
}

void mock_tim_set_period_ticks(uintptr_t tim_base, uint32_t us) {
    TIM_Registers *regs = tim_regs_get(tim_base);
    if (!regs) return;
    regs->ARR = us;
    regs->PSC = 0;
}

void mock_tim_set_pwm_duty(uintptr_t tim_base, uint32_t channel, uint32_t duty) {
    TIM_Registers *regs = tim_regs_get(tim_base);
    if (!regs || channel > 3) return;
    (&regs->CCR1)[channel] = duty;
}

void mock_tim_inject_capture(uintptr_t tim_base, uint32_t channel, uint32_t value) {
    TIM_Registers *regs = tim_regs_get(tim_base);
    if (!regs || channel > 3) return;
    (&regs->CCR1)[channel] = value;
    regs->SR |= (TIM_SR_CC1IF << channel);
    if (regs->DIER & (TIM_DIER_CC1IE << channel)) {
        NVIC_SetPendingIRQ(TIM2_IRQn);
    }
}

/* ----- HAL implementations using registers ----- */
HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef *htim) {
    tim_regs_init(htim->Instance);
    ensure_timer(htim->Instance);
    ember_runtime_on_tick(tim_tick_cb);
    trace_log("HAL_TIM_Base_Init", "\"registered\"");
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef *htim) {
    TIM_Registers *regs = tim_regs_get(htim->Instance);
    if (!regs) return HAL_ERROR;
    regs->CR1 |= TIM_CR1_CEN;
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_Base_Start", payload);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef *htim) {
    TIM_Registers *regs = tim_regs_get(htim->Instance);
    if (!regs) return HAL_ERROR;
    regs->CR1 &= ~TIM_CR1_CEN;
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_Base_Stop", payload);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim) {
    TIM_Registers *regs = tim_regs_get(htim->Instance);
    if (!regs) return HAL_ERROR;
    regs->DIER |= TIM_DIER_UIE;
    HAL_TIM_Base_Start(htim);
    /* No manual scheduling; tick callbacks will drive events */
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *htim) {
    TIM_Registers *regs = tim_regs_get(htim->Instance);
    if (regs) regs->DIER &= ~TIM_DIER_UIE;
    return HAL_TIM_Base_Stop(htim);
}

HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim) { return HAL_OK; }

HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel) {
    TIM_Registers *regs = tim_regs_get(htim->Instance);
    if (regs) {
        regs->DIER |= (TIM_DIER_CC1IE << (Channel - 1));
        set_pwm_active(htim->Instance, Channel - 1, true);
    }
    return HAL_TIM_Base_Start(htim);
}

HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel) {
    TIM_Registers *regs = tim_regs_get(htim->Instance);
    if (regs) {
        regs->DIER &= ~(TIM_DIER_CC1IE << (Channel - 1));
        set_pwm_active(htim->Instance, Channel - 1, false);
    }
    return HAL_TIM_Base_Stop(htim);
}

HAL_StatusTypeDef HAL_TIM_IC_Init(TIM_HandleTypeDef *htim) { return HAL_OK; }

HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel) {
    TIM_Registers *regs = tim_regs_get(htim->Instance);
    if (regs) {
        regs->DIER |= (TIM_DIER_CC1IE << (Channel - 1));
    }
    return HAL_TIM_Base_Start(htim);
}

/* ----- IRQ handler ----- */
void HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim) {
    TIM_Registers *regs = tim_regs_get(htim->Instance);
    if (!regs) return;

    char details[128];
    snprintf(details, sizeof(details),
             "{\"instance\":\"TIM2\",\"address\":\"0x%08X\"}", (uint32_t)htim->Instance);
    trace_software_event("TIM", "irq_handler", details);

    if (regs->SR & TIM_SR_UIF) {
        regs->SR &= ~TIM_SR_UIF;
        HAL_TIM_PeriodElapsedCallback(htim);
        /* Fire PWM pulse finished if any PWM channel is active */
        if (is_pwm_active(htim->Instance)) {
            HAL_TIM_PWM_PulseFinishedCallback(htim);
        }
    }

    /* Clear capture compare flags and fire callbacks */
    for (int ch = 0; ch < 4; ch++) {
        uint32_t flag = TIM_SR_CC1IF << ch;
        if (regs->SR & flag) {
            regs->SR &= ~flag;
            /* For channels configured as PWM, don't call capture callback;
               we rely on the UIF path for PWM. For IC, call capture. */
            int idx = find_timer_index(htim->Instance);
            if (idx >= 0 && pwm_active_channels[idx][ch]) {
                /* In PWM mode, the compare match flag may set; we could ignore or handle separately.
                   For now, we'll not call PulseFinished here because it's called above on UIF. */
            } else {
                HAL_TIM_IC_CaptureCallback(htim);
            }
        }
    }
}