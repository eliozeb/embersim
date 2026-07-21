/**
  ******************************************************************************
  * @file    app.h
  * @brief   Application core — state and entry points for Pilot 0 firmware.
  ******************************************************************************
  */

#ifndef __APP_H
#define __APP_H

#include <stdint.h>

/* Application state shared across modules */
typedef struct {
    uint32_t tick_count;
    uint8_t  motor_running;
    float    temperature;
    uint8_t  led_state;
} AppState;

/* --------------------------------------------------------------------------
   Two-function entry model (EmberSim + native)
   -------------------------------------------------------------------------- */

/**
  * @brief  Full initialization: HAL_Init → Clock → MX_*_Init → subsystem init.
  *         Called once at startup by both native main() and host_main.c.
  */
void app_init(void);

/**
  * @brief  Main loop iteration. Called repeatedly by both native main()
  *         and host_main.c.
  */
void app_run(void);

/* --------------------------------------------------------------------------
   Accessors for other modules
   -------------------------------------------------------------------------- */

AppState *app_get_state(void);
void      app_set_motor_running(uint8_t running);
void      app_set_temperature(float temp);

#endif /* __APP_H */
