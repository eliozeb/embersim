/**
  ******************************************************************************
  * @file    app.c
  * @brief   Application core — state machine, LED heartbeat, command dispatch.
  *
  *          This is the central coordination module. It owns the application
  *          state and delegates to uart_service, motor_control, and sensor.
  *
  *          For EmberSim, app_init() contains the FULL initialization
  *          sequence (HAL_Init → Clock → MX_*_Init → subsystems) matching
  *          the two-function entry model documented in HAL_BOUNDARY.md.
  *
  *          For the native gcc build, main() calls app_init() directly.
  ******************************************************************************
  */

#include "app.h"
#include "board.h"
#include "uart_service.h"
#include "motor_control.h"
#include "sensor.h"
#include <stdio.h>

/* --------------------------------------------------------------------------
   Private state
   -------------------------------------------------------------------------- */

static AppState g_state;

/* --------------------------------------------------------------------------
   Public API — accessors
   -------------------------------------------------------------------------- */

AppState *app_get_state(void)
{
    return &g_state;
}

void app_set_motor_running(uint8_t running)
{
    g_state.motor_running = running;
}

void app_set_temperature(float temp)
{
    g_state.temperature = temp;
}

/* --------------------------------------------------------------------------
   app_init — full initialization, called once at startup
   -------------------------------------------------------------------------- */

void app_init(void)
{
    /* ---- Hardware initialization (BSP layer) ---- */
    board_init();

    /* ---- Application state ---- */
    g_state.tick_count    = 0U;
    g_state.motor_running = 0U;
    g_state.temperature   = SPI_TEMP_FIXED;
    g_state.led_state     = 0U;

    /* ---- Subsystem initialization ---- */
    uart_service_init();
    motor_init();

    /* ---- Start periodic timer (10 ms interrupt) ---- */
    HAL_TIM_Base_Start_IT(&htim2);

    /* ---- Ensure LED starts off ---- */
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_5, GPIO_PIN_RESET);
}

/* --------------------------------------------------------------------------
   app_run — called repeatedly in the main loop
   -------------------------------------------------------------------------- */

void app_run(void)
{
    Command cmd;

    /* Check for incoming UART commands */
    cmd = uart_service_poll();

    switch (cmd) {
    case CMD_STATUS:
        if (g_state.motor_running) {
            uart_service_respond("OK:MOTOR_RUNNING");
        } else {
            uart_service_respond("OK:READY");
        }
        break;

    case CMD_MOTOR_ON:
        motor_start();
        g_state.motor_running = 1;
        uart_service_respond("OK:MOTOR_RUNNING");
        break;

    case CMD_MOTOR_OFF:
        motor_stop();
        g_state.motor_running = 0;
        uart_service_respond("OK:MOTOR_STOPPED");
        break;

    case CMD_TEMPERATURE:
        g_state.temperature = sensor_read_temperature();
        {
            char buf[32];
            snprintf(buf, sizeof(buf), "OK:TEMP=%.1fC", g_state.temperature);
            uart_service_respond(buf);
        }
        break;

    case CMD_UNKNOWN:
        uart_service_respond("ERR:UNKNOWN_CMD");
        break;

    case CMD_NONE:
    default:
        break;
    }

    /* Check fault interrupt flag — set by HAL_GPIO_EXTI_Callback on PA0 falling edge.
       Also poll the GPIO as a secondary check (belt-and-suspenders pattern common
       in industrial firmware where a missed interrupt must not hide a fault). */
    if (board_fault_irq_pending || (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_0) == GPIO_PIN_RESET)) {
        board_fault_irq_pending = 0U;
        if (g_state.motor_running) {
            motor_stop();
            g_state.motor_running = 0U;
            uart_service_respond("ERR:FAULT");
        }
    }
}

/* --------------------------------------------------------------------------
   HAL Callbacks — called from interrupt context (simulated)
   -------------------------------------------------------------------------- */

/**
  * @brief  TIM2 period elapsed callback — fires every 10 ms.
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
    if (htim->Instance == TIM2) {
        g_state.tick_count++;

        /* Heartbeat LED toggle every 100 ms (10 ticks) */
        if ((g_state.tick_count % 10U) == 0U) {
            HAL_GPIO_TogglePin(GPIOA, GPIO_PIN_5);
            g_state.led_state = !g_state.led_state;
        }
    }
}
