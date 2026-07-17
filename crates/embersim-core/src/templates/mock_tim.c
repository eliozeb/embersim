/* mock_tim.c — Professional timer mock with period, PWM, input capture,
   tick‑driven updates, and trace logging. */

   #include "mock_hal.h"
   #include "mock_tim.h"
   #include "trace_log.h"
   #include <string.h>
   #include <stdio.h>
   #include <stdbool.h>
   
   /* ----- forward declarations for callbacks ----- */
   void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
   void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);
   void HAL_TIM_IC_CaptureCallback(TIM_HandleTypeDef *htim);
   void HAL_TIM_ErrorCallback(TIM_HandleTypeDef *htim);
   
   /* ----- internal config ----- */
   #define TIM_MAX_INSTANCES  4
   
   typedef struct {
       /* base timer config */
       uint32_t instance;
       bool     running;
       uint32_t period_ticks;       /* update period in ember ticks */
       uint32_t counter;            /* current counter value */
       uint32_t prescaler;
       uint32_t autoreload;
   
       /* PWM */
       uint32_t pwm_duty[4];       /* channel duty cycle (0..period_ticks) */
   
       /* input capture */
       uint32_t ic_value[4];
       bool     ic_pending[4];
   
       /* callbacks */
       bool     period_elapsed_pending;
       bool     pwm_pulse_finished_pending;
       bool     ic_capture_pending[4];
       uint32_t error_flags;
   
       /* async scheduling (for IT/DMA) */
       bool     xfer_pending;
       uint32_t xfer_end_tick;
       TIM_HandleTypeDef *active_htim;
   } TIM_Instance;
   
   static TIM_Instance tim_instances[TIM_MAX_INSTANCES];
   static uint32_t    s_tick = 0;
   
   /* ----- base‑address lookup ----- */
   static int tim_index(uintptr_t base) {
       switch (base) {
           case 0x40010000: return 0; /* TIM1 */
           case 0x40000400: return 1; /* TIM2 */
           case 0x40000800: return 2; /* TIM3 */
           case 0x40000C00: return 3; /* TIM4 */
           default:         return 0;
       }
   }
   
   /* ----- public API ----- */
   void mock_tim_init(void) {
       memset(tim_instances, 0, sizeof(tim_instances));
       s_tick = 0;
   }
   
   void mock_tim_set_period_ticks(uintptr_t tim_base, uint32_t ticks) {
       int idx = tim_index(tim_base);
       tim_instances[idx].period_ticks = ticks;
       tim_instances[idx].autoreload = ticks - 1;  /* mimic ARR */
   }
   
   void mock_tim_set_pwm_duty(uintptr_t tim_base, uint32_t channel, uint32_t duty) {
       int idx = tim_index(tim_base);
       if (channel < 4)
           tim_instances[idx].pwm_duty[channel] = duty;
   }
   
   void mock_tim_inject_capture(uintptr_t tim_base, uint32_t channel, uint32_t value) {
       int idx = tim_index(tim_base);
       if (channel < 4) {
           tim_instances[idx].ic_value[channel] = value;
           tim_instances[idx].ic_capture_pending[channel] = true;
       }
   }
   
   /* ----- tick function (must be called periodically) ----- */
   void ember_sim_tim_tick(void) {
       s_tick++;
       for (int i = 0; i < TIM_MAX_INSTANCES; i++) {
           TIM_Instance *tim = &tim_instances[i];
           if (!tim->running) continue;
   
           tim->counter++;
           if (tim->period_ticks > 0 && tim->counter >= tim->period_ticks) {
               tim->counter = 0;
               tim->period_elapsed_pending = true;
               tim->pwm_pulse_finished_pending = true;
           }
   
           /* fire pending callbacks (IT/DMA completion is separate) */
           if (tim->period_elapsed_pending && !tim->xfer_pending) {
               tim->period_elapsed_pending = false;
               HAL_TIM_PeriodElapsedCallback(tim->active_htim ? tim->active_htim : NULL);
           }
           if (tim->pwm_pulse_finished_pending && !tim->xfer_pending) {
               tim->pwm_pulse_finished_pending = false;
               HAL_TIM_PWM_PulseFinishedCallback(tim->active_htim ? tim->active_htim : NULL);
           }
           for (int ch = 0; ch < 4; ch++) {
               if (tim->ic_capture_pending[ch] && !tim->xfer_pending) {
                   tim->ic_capture_pending[ch] = false;
                   HAL_TIM_IC_CaptureCallback(tim->active_htim ? tim->active_htim : NULL);
               }
           }
   
           /* asynchronous IT/DMA completion */
           if (tim->xfer_pending && s_tick >= tim->xfer_end_tick) {
               tim->xfer_pending = false;
               /* fire appropriate completion callback */
               if (tim->period_elapsed_pending) {
                   tim->period_elapsed_pending = false;
                   HAL_TIM_PeriodElapsedCallback(tim->active_htim);
               }
               tim->active_htim = NULL;
           }
       }
   }
   
   /* ----- helper: get instance from handle ----- */
   static TIM_Instance *get_inst(TIM_HandleTypeDef *htim) {
       return &tim_instances[tim_index((uintptr_t)htim->Instance)];
   }
   
   /* ----- Base functions ----- */
   HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef *htim) {
       (void)htim;
       return HAL_OK;
   }
   HAL_StatusTypeDef HAL_TIM_Base_Start(TIM_HandleTypeDef *htim) {
        TIM_Instance *tim = get_inst(htim);
        tim->running = true;
        tim->counter = 0;
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
       return HAL_TIM_Base_Start(htim);
   }
   HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *htim) {
       return HAL_TIM_Base_Stop(htim);
   }
   
   /* ----- PWM functions ----- */
   HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim) {
       (void)htim;
       return HAL_OK;
   }
   HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel) {
        TIM_Instance *tim = get_inst(htim);
        tim->running = true;
        char payload[128];
        snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"ch\":%u", (uint32_t)htim->Instance, Channel);
        trace_log("HAL_TIM_PWM_Start", payload);
        return HAL_OK;
    }
   HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel) {
       (void)Channel;
       return HAL_TIM_Base_Stop(htim);
   }
   
   /* ----- Input Capture functions ----- */
   HAL_StatusTypeDef HAL_TIM_IC_Init(TIM_HandleTypeDef *htim) {
       (void)htim;
       return HAL_OK;
   }
   HAL_StatusTypeDef HAL_TIM_IC_Start_IT(TIM_HandleTypeDef *htim, uint32_t Channel) {
        TIM_Instance *tim = get_inst(htim);
        tim->running = true;
        tim->active_htim = htim;
        char payload[128];
        snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"ch\":%u", (uint32_t)htim->Instance, Channel);
        trace_log("HAL_TIM_IC_Start_IT", payload);
        return HAL_OK;
    }
   
    /* ----- Weak default callbacks ----- */
    __attribute__((weak)) void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
        char payload[128];
        snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)htim->Instance);
        trace_log("HAL_TIM_PeriodElapsedCallback", payload);
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