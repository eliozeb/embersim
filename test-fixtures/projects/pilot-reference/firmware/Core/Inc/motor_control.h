/**
  ******************************************************************************
  * @file    motor_control.h
  * @brief   Motor control via TIM2 PWM — start, stop, state query.
  ******************************************************************************
  */

#ifndef __MOTOR_CONTROL_H
#define __MOTOR_CONTROL_H

#include <stdint.h>
#include "stm32f4xx_hal.h"

#define MOTOR_PWM_CHANNEL  TIM_CHANNEL_1
#define MOTOR_DUTY_MAX     1000U

typedef enum {
    MOTOR_STOPPED = 0,
    MOTOR_RUNNING = 1,
} MotorState;

void       motor_init(void);
void       motor_start(void);
void       motor_stop(void);
MotorState motor_get_state(void);
void       motor_set_duty(uint32_t duty);

#endif /* __MOTOR_CONTROL_H */
