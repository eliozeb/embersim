/**
  ******************************************************************************
  * @file    motor_control.c
  * @brief   Motor control via TIM2 PWM Channel 1.
  *
  *          Start/stop PWM output. Duty cycle is tracked in firmware state.
  *          On real hardware, __HAL_TIM_SET_COMPARE writes the CCR register
  *          directly. Under EmberSim, mock_tim.c handles PWM internally
  *          and HAL_TIM_PWM_Start/Stop produce the trace events.
  *
  *          Runtime duty cycle changes are supported via stop→reconfigure→start
  *          (the standard STM32 HAL pattern for PWM duty updates).
  ******************************************************************************
  */

#include "motor_control.h"
#include "board.h"

/* --------------------------------------------------------------------------
   Private state
   -------------------------------------------------------------------------- */

static MotorState g_motor_state = MOTOR_STOPPED;
static uint32_t   g_motor_duty  = 0U;

/* --------------------------------------------------------------------------
   Public API
   -------------------------------------------------------------------------- */

void motor_init(void)
{
    g_motor_state = MOTOR_STOPPED;
    g_motor_duty  = 0;
}

void motor_start(void)
{
    if (g_motor_state == MOTOR_RUNNING) {
        return;
    }

    /* Set default 50% duty cycle if not previously configured */
    if (g_motor_duty == 0) {
        g_motor_duty = MOTOR_DUTY_MAX / 2;
    }

    HAL_TIM_PWM_Start(&htim2, MOTOR_PWM_CHANNEL);
    g_motor_state = MOTOR_RUNNING;
}

void motor_stop(void)
{
    if (g_motor_state == MOTOR_STOPPED) {
        return;
    }

    HAL_TIM_PWM_Stop(&htim2, MOTOR_PWM_CHANNEL);
    g_motor_state = MOTOR_STOPPED;
}

MotorState motor_get_state(void)
{
    return g_motor_state;
}

void motor_set_duty(uint32_t duty)
{
    TIM_OC_InitTypeDef oc = {0};

    if (duty > MOTOR_DUTY_MAX) {
        duty = MOTOR_DUTY_MAX;
    }
    g_motor_duty = duty;

    if (g_motor_state == MOTOR_RUNNING) {
        /* Runtime duty cycle update: stop, reconfigure, restart.
           This is the correct HAL pattern when __HAL_TIM_SET_COMPARE
           is not available (or when using EmberSim's mock_tim which
           tracks PWM state through HAL calls). */
        HAL_TIM_PWM_Stop(&htim2, MOTOR_PWM_CHANNEL);

        oc.OCMode      = TIM_OCMODE_PWM1;
        oc.Pulse       = duty;
        oc.OCPolarity  = TIM_OCPOLARITY_HIGH;
        oc.OCFastMode  = 0;
        HAL_TIM_PWM_ConfigChannel(&htim2, &oc, MOTOR_PWM_CHANNEL);

        HAL_TIM_PWM_Start(&htim2, MOTOR_PWM_CHANNEL);
    }
}
