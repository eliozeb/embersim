#include "mock_hal.h"
#include <stdio.h>

static void trace_log(const char *func, const char *fmt, ...) {
    (void)func; (void)fmt;
    /* TODO: implement trace logging in Day 5 */
}

__attribute__((weak)) HAL_StatusTypeDef HAL_GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_Init)
{
    trace_log("HAL_GPIO_Init", "GPIOx,GPIO_Init");
    return HAL_OK;
}

__attribute__((weak)) void HAL_GPIO_DeInit(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin)
{
    trace_log("HAL_GPIO_DeInit", "GPIOx,GPIO_Pin");
    /* void */
}

__attribute__((weak)) void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    trace_log("HAL_GPIO_WritePin", "GPIOx,GPIO_Pin,PinState");
    /* void */
}

__attribute__((weak)) GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    trace_log("HAL_GPIO_ReadPin", "GPIOx,GPIO_Pin");
    return 0; /* default */
}

__attribute__((weak)) void HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    trace_log("HAL_GPIO_TogglePin", "GPIOx,GPIO_Pin");
    /* void */
}

__attribute__((weak)) HAL_StatusTypeDef HAL_GPIO_LockPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    trace_log("HAL_GPIO_LockPin", "GPIOx,GPIO_Pin");
    return HAL_OK;
}

__attribute__((weak)) void HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin)
{
    trace_log("HAL_GPIO_EXTI_IRQHandler", "GPIO_Pin");
    /* void */
}

__attribute__((weak)) void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin)
{
    trace_log("HAL_GPIO_EXTI_Callback", "GPIO_Pin");
    /* void */
}

__attribute__((weak)) HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout)
{
    trace_log("HAL_UART_Transmit", "huart,pData,Size,Timeout");
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout)
{
    trace_log("HAL_UART_Receive", "huart,pData,Size,Timeout");
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size)
{
    trace_log("HAL_UART_Transmit_IT", "huart,pData,Size");
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size)
{
    trace_log("HAL_UART_Receive_IT", "huart,pData,Size");
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size)
{
    trace_log("HAL_UART_Transmit_DMA", "huart,pData,Size");
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size)
{
    trace_log("HAL_UART_Receive_DMA", "huart,pData,Size");
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_UART_DMAPause(UART_HandleTypeDef* huart)
{
    trace_log("HAL_UART_DMAPause", "huart");
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_UART_DMAResume(UART_HandleTypeDef* huart)
{
    trace_log("HAL_UART_DMAResume", "huart");
    return HAL_OK;
}

__attribute__((weak)) HAL_StatusTypeDef HAL_UART_DMAStop(UART_HandleTypeDef* huart)
{
    trace_log("HAL_UART_DMAStop", "huart");
    return HAL_OK;
}

