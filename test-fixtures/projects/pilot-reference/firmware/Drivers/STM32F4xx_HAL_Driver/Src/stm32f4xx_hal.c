/**
  ******************************************************************************
  * @file    stm32f4xx_hal.c
  * @brief   HAL source stubs — empty implementations for native gcc build.
  *
  *          These are NOT the real STM32 HAL. They provide function bodies
  *          so the firmware links successfully when built with gcc.
  *
  *          When running under EmberSim, these functions are NOT used —
  *          EmberSim's mock implementations (mock_hal.c, mock_uart.c, etc.)
  *          provide the actual behavior.
  ******************************************************************************
  */

#include "stm32f4xx_hal.h"

/* GPIO */
void HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init)           { (void)GPIOx; (void)GPIO_Init; }
void HAL_GPIO_DeInit(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin)                   { (void)GPIOx; (void)GPIO_Pin; }
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)         { (void)GPIOx; (void)GPIO_Pin; return GPIO_PIN_RESET; }
void HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState s){ (void)GPIOx; (void)GPIO_Pin; (void)s; }
void HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)                { (void)GPIOx; (void)GPIO_Pin; }
HAL_StatusTypeDef HAL_GPIO_LockPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin)     { (void)GPIOx; (void)GPIO_Pin; return HAL_OK; }
void HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin)                                { (void)GPIO_Pin; }
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)                                  { (void)GPIO_Pin; }

/* UART */
HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart)                      { (void)huart; return HAL_OK; }
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
    { (void)huart; (void)pData; (void)Size; (void)Timeout; return HAL_OK; }
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout)
    { (void)huart; (void)pData; (void)Size; (void)Timeout; return HAL_OK; }
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
    { (void)huart; (void)pData; (void)Size; return HAL_OK; }
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size)
    { (void)huart; (void)pData; (void)Size; return HAL_OK; }
void HAL_UART_IRQHandler(UART_HandleTypeDef *huart)                             { (void)huart; }
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart)                         { (void)huart; }
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)                         { (void)huart; }
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)                          { (void)huart; }

/* TIM */
HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef *htim)                    { (void)htim; return HAL_OK; }
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim)                { (void)htim; return HAL_OK; }
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *htim)                 { (void)htim; return HAL_OK; }
HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim)                     { (void)htim; return HAL_OK; }
HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef *htim, TIM_OC_InitTypeDef *cfg, uint32_t ch)
    { (void)htim; (void)cfg; (void)ch; return HAL_OK; }
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel)  { (void)htim; (void)Channel; return HAL_OK; }
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel)   { (void)htim; (void)Channel; return HAL_OK; }
void HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim)                                 { (void)htim; }
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)                      { (void)htim; }
void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)                  { (void)htim; }

/* SPI */
HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *hspi)                         { (void)hspi; return HAL_OK; }
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
    { (void)hspi; (void)pData; (void)Size; (void)Timeout; return HAL_OK; }
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout)
    { (void)hspi; (void)pData; (void)Size; (void)Timeout; return HAL_OK; }
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi, uint8_t *tx, uint8_t *rx, uint16_t sz, uint32_t timeout)
    { (void)hspi; (void)tx; (void)rx; (void)sz; (void)timeout; return HAL_OK; }
HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
    { (void)hspi; (void)pData; (void)Size; return HAL_OK; }
HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size)
    { (void)hspi; (void)pData; (void)Size; return HAL_OK; }
HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef *hspi, uint8_t *tx, uint8_t *rx, uint16_t sz)
    { (void)hspi; (void)tx; (void)rx; (void)sz; return HAL_OK; }
void HAL_SPI_IRQHandler(SPI_HandleTypeDef *hspi)                                 { (void)hspi; }
void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi)                             { (void)hspi; }
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi)                             { (void)hspi; }
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi)                           { (void)hspi; }
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi)                              { (void)hspi; }

/* RCC */
HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *cfg)                     { (void)cfg; return HAL_OK; }
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *cfg, uint32_t lat)     { (void)cfg; (void)lat; return HAL_OK; }

/* NVIC */
void HAL_NVIC_SetPriority(IRQn_Type IRQn, uint32_t pre, uint32_t sub)            { (void)IRQn; (void)pre; (void)sub; }
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn)                                           { (void)IRQn; }
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn)                                          { (void)IRQn; }

/* System */
HAL_StatusTypeDef HAL_Init(void)                                                 { return HAL_OK; }
