/**
  ******************************************************************************
  * @file    sensor.h
  * @brief   SPI temperature sensor — mock read returning fixed 25.0°C.
  ******************************************************************************
  */

#ifndef __SENSOR_H
#define __SENSOR_H

#include <stdint.h>

#define SPI_TEMP_CMD   0x00U
#define SPI_TEMP_FIXED 25.0f

float sensor_read_temperature(void);

#endif /* __SENSOR_H */
