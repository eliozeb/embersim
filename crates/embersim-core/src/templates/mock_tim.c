/* mock_tim.c — Timer mock using the unified Ember runtime */
#include "mock_hal.h"
#include "mock_tim.h"
#include "ember_sim_runtime.h"
#include "trace_log.h"
#include <string.h>
#include <stdio.h>

/* Forward declarations */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim);

#define TIM_MAX_INSTANCES  4

typedef struct {
    uint32_t instance;
    bool     running;
    uint32_t period_us;
    uint32_t pwm_duty[4];
    bool     pwm_active[4];
    uint32_t ic_value[4];
    bool     ic_capture_pending[4];
    TIM_HandleTypeDef *active_htim;
} TIM_Instance;

static TIM_Instance tim_instances[TIM_MAX_INSTANCES];

static int tim_index(uintptr_t base) {
    switch (base) {
        case 0x40010000: return 0;
        case 0x40000400: return 1;
        case 0x40000800: return 2;
        case 0x40000C00: return 3;
        default:         return 0;
    }
}

static TIM_Instance *get_inst(TIM_HandleTypeDef *htim) {
    return &tim_instances[tim_index((uintptr_t)htim->Instance)];
}

/* ----- public API ----- */
void mock_tim_init(void) {
    memset(tim_instances, 0, sizeof(tim_instances));
}
void mock_tim_set_period_ticks(uintptr_t tim_base, uint32_t us) {
    int idx = tim_index(tim_base);
    tim_instances[idx].period_us = us;
}
void mock_tim_set_pwm_duty(uintptr_t tim_base, uint32_t channel, uint32_t duty) {
    int idx = tim_index(tim_base);
    if (channel < 4) tim_instances[idx].pwm_duty[channel] = duty;
}
void mock_tim_inject_capture(uintptr_t tim_base, uint32_t channel, uint32_t value) {
    int idx = tim_index(tim_base);
    if (channel < 4) {
        tim_instances[idx].ic_value[channel] = value;
        tim_instances[idx].ic_capture_pending[channel] = true;
    }
}

/* ----- internal callback firing (called by HAL_TIM_IRQHandler) ----- */
static bool any_pwm_active(TIM_Instance *tim) {
    for (int i = 0; i < 4; i++) if (tim->pwm_active[i]) return true;
    return false;
}

static void tim_fire_period(TIM_Instance *tim) {
    if (!tim->active_htim) return;
    HAL_TIM_PeriodElapsedCallback(tim->active_htim);
    if (any_pwm_active(tim)) {
        HAL_TIM_PWM_PulseFinishedCallback(tim->active_htim);
    }
    /* reschedule */
    ember_runtime_schedule(tim->period_us, EVENT_TIM_UPDATE,
                           tim->instance, 0);
}

static void tim_fire_capture(TIM_Instance *tim, uint32_t ch) {
    (void)ch;
    if (tim->active_htim) {
        HAL_TIM_IC_CaptureCallback(tim->active_htim);
    }
}

/* ----- HAL_TIM_IRQHandler (called by HAL_IRQ_Dispatch) ----- */
void HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim) {
    char details[128];
    snprintf(details, sizeof(details),
             "{\"instance\":\"TIM2\",\"address\":\"0x%08X\"}", (uint32_t)htim->Instance);
    trace_software_event("TIM", "irq_handler", details);

    TIM_Instance *tim = get_inst(htim);
    tim_fire_period(tim);
    for (int ch = 0; ch < 4; ch++) {
        if (tim->ic_capture_pending[ch]) {
            tim->ic_capture_pending[ch] = false;
            tim_fire_capture(tim, ch);
        }
    }
}

/* ----- HAL API functions (schedule events) ----- */
HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef *htim) { (void)htim; return HAL_OK; }

HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef *htim) {
    TIM_Instance *tim = get_inst(htim);
    tim->instance = (uintptr_t)htim->Instance;  // ← add this
    tim->running = true;
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_Base_Start", payload);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_Base_Stop(TIM_HandleTypeDef *htim) {
    TIM_Instance *tim = get_inst(htim);
    tim->running = false;
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_Base_Stop", payload);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim) {
    TIM_Instance *tim = get_inst(htim);
    tim->active_htim = htim;
    HAL_TIM_Base_Start(htim);
    /* Schedule first update event */
    ember_runtime_schedule(tim->period_us, EVENT_TIM_UPDATE,
                           tim->instance, 0);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *htim) {
    return HAL_TIM_Base_Stop(htim);
}

HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim) { (void)htim; return HAL_OK; }
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel) {
    TIM_Instance *tim = get_inst(htim);
    tim->running = true;
    if (Channel < 4) tim->pwm_active[Channel] = true;
    tim->active_htim = htim;
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"ch\":%u", (uint32_t)htim->Instance, Channel);
    trace_log("HAL_TIM_PWM_Start", payload);
    ember_runtime_schedule(tim->period_us, EVENT_TIM_UPDATE,
                           tim->instance, 0);
    return HAL_OK;
}
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel) {
    if (Channel < 4) {
        TIM_Instance *tim = get_inst(htim);
        tim->pwm_active[Channel] = false;
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_TIM_IC_Init(TIM_HandleTypeDef *htim) { (void)htim; return HAL_OK; }
HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel) {
    TIM_Instance *tim = get_inst(htim);
    tim->running = true;
    tim->active_htim = htim;
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"ch\":%u", (uint32_t)htim->Instance, Channel);
    trace_log("HAL_TIM_IC_Start_IT", payload);
    if (tim->ic_capture_pending[Channel]) {
        ember_runtime_schedule(0, EVENT_TIM_UPDATE, tim->instance, Channel);
    }
    return HAL_OK;
}

/* Weak default callbacks */
__attribute__((weak)) void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    char details[128];
    snprintf(details, sizeof(details),
             "{\"instance\":\"TIM2\",\"address\":\"0x%08X\"}", (uint32_t)htim->Instance);
    trace_software_event("TIM", "period_elapsed", details);
}
__attribute__((weak)) void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim) {
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_PWM_PulseFinishedCallback", payload);
}
__attribute__((weak)) void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim) {
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_IC_CaptureCallback", payload);
}
__attribute__((weak)) void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim) {
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
    trace_log("HAL_TIM_ErrorCallback", payload);
}