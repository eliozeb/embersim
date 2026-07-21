/**
  ******************************************************************************
  * @file    board.c
  * @brief   Board Support Package implementation.
  *
  *          Owns all peripheral handles and MX_* initializer functions.
  *          Compiled by both the native gcc build and the EmberSim build.
  *
  *          This file replaces the earlier init.c — it adds a BSP abstraction
  *          layer that mirrors production STM32 project structure.
  ******************************************************************************
  */

#include "board.h"

/* --------------------------------------------------------------------------
   Peripheral handle definitions (single point of truth)
   -------------------------------------------------------------------------- */

UART_HandleTypeDef huart2;
TIM_HandleTypeDef  htim2;
SPI_HandleTypeDef  hspi1;

/* Interrupt flag — set by EXTI callback, consumed by app */
volatile uint8_t board_fault_irq_pending;

/* --------------------------------------------------------------------------
   board_init — master initialization entry point
   -------------------------------------------------------------------------- */

void board_init(void)
{
    HAL_Init();
    board_clock_config();
    board_gpio_init();
    board_uart_init();
    board_tim_init();
    board_spi_init();
}

/* --------------------------------------------------------------------------
   System Clock Configuration
   -------------------------------------------------------------------------- */

void board_clock_config(void)
{
    /* Minimal clock config — APB1/APB2 prescalers at reset defaults.
       In simulation, clocks don't drive real timing; the scheduler
       controls tick advancement. Struct fields are zeroed for
       reset-default HSI (16 MHz) operation. */
    RCC_OscInitTypeDef osc_init = {0};
    RCC_ClkInitTypeDef clk_init = {0};

    HAL_RCC_OscConfig(&osc_init);
    HAL_RCC_ClockConfig(&clk_init, 0U);
}

/* --------------------------------------------------------------------------
   GPIO Initialization
   -------------------------------------------------------------------------- */

void board_gpio_init(void)
{
    GPIO_InitTypeDef gpio = {0};

    /* PA5 — Status LED (output, push-pull, no pull) */
    gpio.Pin       = BOARD_LED_PIN;
    gpio.Mode      = GPIO_MODE_OUTPUT_PP;
    gpio.Pull      = GPIO_NOPULL;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = 0;
    HAL_GPIO_Init(BOARD_LED_PORT, &gpio);

    /* PA0 — Fault input (input, pull-up, interrupt on falling edge).
       The EXTI interrupt fires deterministically in simulation when
       the fault pin is asserted low. */
    gpio = (GPIO_InitTypeDef){0};
    gpio.Pin       = BOARD_FAULT_PIN;
    gpio.Mode      = GPIO_MODE_IT_FALLING;
    gpio.Pull      = GPIO_PULLUP;
    gpio.Speed     = GPIO_SPEED_FREQ_LOW;
    gpio.Alternate = 0;
    HAL_GPIO_Init(BOARD_FAULT_PORT, &gpio);

    /* Enable EXTI0 interrupt in NVIC (simulation: NVIC tracks pending state) */
    HAL_NVIC_SetPriority(BOARD_FAULT_IRQ, 2, 0);
    HAL_NVIC_EnableIRQ(BOARD_FAULT_IRQ);
}

/* --------------------------------------------------------------------------
   UART Initialization
   -------------------------------------------------------------------------- */

void board_uart_init(void)
{
    huart2.Instance          = USART2;
    huart2.Init.BaudRate     = 115200U;
    huart2.Init.WordLength   = UART_WORDLENGTH_8B;
    huart2.Init.StopBits     = UART_STOPBITS_1;
    huart2.Init.Parity       = UART_PARITY_NONE;
    huart2.Init.Mode         = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl    = UART_HWCONTROL_NONE;
    huart2.Init.OverSampling = UART_OVERSAMPLING_16;
    HAL_UART_Init(&huart2);
}

/* --------------------------------------------------------------------------
   TIM2 Initialization — 10 ms control loop + PWM Channel 1
   -------------------------------------------------------------------------- */

void board_tim_init(void)
{
    TIM_OC_InitTypeDef oc = {0};

    htim2.Instance               = TIM2;
    htim2.Init.Prescaler         = 8399U;   /* 84 MHz / 8400 = 10 kHz */
    htim2.Init.CounterMode       = TIM_COUNTERMODE_UP;
    htim2.Init.Period            = 99U;     /* 10 kHz / 100 = 100 Hz → 10 ms */
    htim2.Init.ClockDivision     = 0U;
    htim2.Init.AutoReloadPreload = 0U;
    HAL_TIM_Base_Init(&htim2);

    /* PWM channel 1 for motor output */
    oc.OCMode      = TIM_OCMODE_PWM1;
    oc.Pulse       = 0;                    /* 0% duty at init */
    oc.OCPolarity  = TIM_OCPOLARITY_HIGH;
    oc.OCFastMode  = 0;
    HAL_TIM_PWM_ConfigChannel(&htim2, &oc, TIM_CHANNEL_1);
}

/* --------------------------------------------------------------------------
   SPI1 Initialization — temperature sensor interface
   -------------------------------------------------------------------------- */

void board_spi_init(void)
{
    hspi1.Instance               = SPI1;
    hspi1.Init.Mode              = SPI_MODE_MASTER;
    hspi1.Init.Direction         = SPI_DIRECTION_2LINES;
    hspi1.Init.DataSize          = SPI_DATASIZE_8BIT;
    hspi1.Init.CLKPolarity       = SPI_CPOL_LOW;
    hspi1.Init.CLKPhase          = SPI_CPHA_1EDGE;
    hspi1.Init.NSS               = SPI_NSS_SOFT;
    hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_32;
    hspi1.Init.FirstBit          = SPI_FIRSTBIT_MSB;
    HAL_SPI_Init(&hspi1);
}

/* --------------------------------------------------------------------------
   HAL Callbacks — EXTI interrupt handler
   -------------------------------------------------------------------------- */

/**
  * @brief  GPIO EXTI callback — fires when fault pin asserts (falling edge).
  *         In simulation, this is dispatched by the NVIC model when PA0
  *         transitions from HIGH to LOW.
  */
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    if (GPIO_Pin == BOARD_FAULT_PIN) {
        /* Set flag consumed by app_run().
           This validates that NVIC EXTI dispatch correctly invokes
           the HAL callback chain. The application polls this flag
           instead of the GPIO pin directly, making the interrupt
           path the primary fault detection mechanism. */
        board_fault_irq_pending = 1U;
    }
}
