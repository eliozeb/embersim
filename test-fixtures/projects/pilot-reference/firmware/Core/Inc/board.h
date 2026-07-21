/**
  ******************************************************************************
  * @file    board.h
  * @brief   Board Support Package — STM32F407 industrial motor controller.
  *
  *          The BSP owns all hardware initialization. It is called once by
  *          app_init() before any application logic runs. This mirrors the
  *          standard STM32 project layout where board_setup() precedes the
  *          application entry point.
  ******************************************************************************
  */

#ifndef __BOARD_H
#define __BOARD_H

#include "stm32f4xx_hal.h"

/* --------------------------------------------------------------------------
   Peripheral handles — owned by the BSP, referenced by application modules
   -------------------------------------------------------------------------- */

extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef  htim2;
extern SPI_HandleTypeDef  hspi1;

/* --------------------------------------------------------------------------
   Board pin assignments
   -------------------------------------------------------------------------- */

#define BOARD_LED_PORT    GPIOA
#define BOARD_LED_PIN     GPIO_PIN_5

#define BOARD_FAULT_PORT  GPIOA
#define BOARD_FAULT_PIN   GPIO_PIN_0
#define BOARD_FAULT_IRQ   EXTI0_IRQn

/* --------------------------------------------------------------------------
   BSP API
   -------------------------------------------------------------------------- */

/**
  * @brief  Full board initialization.
  *         HAL_Init → SystemClock_Config → GPIO → UART → TIM → SPI
  */
void board_init(void);

/* Individual initializers — exposed for testing and documentation */
void board_clock_config(void);
void board_gpio_init(void);
void board_uart_init(void);
void board_tim_init(void);
void board_spi_init(void);

/* --------------------------------------------------------------------------
   Interrupt flags — set by ISR callbacks, consumed by application loop
   -------------------------------------------------------------------------- */

/** Set by HAL_GPIO_EXTI_Callback on PA0 falling edge. Cleared by app. */
extern volatile uint8_t board_fault_irq_pending;

#endif /* __BOARD_H */
