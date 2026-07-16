#ifndef MOCK_HAL_H
#define MOCK_HAL_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/* HAL status */
typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;

/* GPIO types */
typedef struct { volatile uint32_t dummy; } GPIO_TypeDef;
typedef struct { uint32_t dummy; } GPIO_InitTypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 } GPIO_PinState;

/* UART state machine */
typedef enum { HAL_UART_STATE_RESET = 0x00, HAL_UART_STATE_READY = 0x20, HAL_UART_STATE_BUSY = 0x24, HAL_UART_STATE_BUSY_TX = 0x21, HAL_UART_STATE_BUSY_RX = 0x22, HAL_UART_STATE_BUSY_TX_RX = 0x23, HAL_UART_STATE_TIMEOUT = 0xA0, HAL_UART_STATE_ERROR = 0xE0 } HAL_UART_StateTypeDef;
#define HAL_UART_ERROR_PE  0x00000001U
#define HAL_UART_ERROR_NE  0x00000002U
#define HAL_UART_ERROR_FE  0x00000004U
#define HAL_UART_ERROR_ORE 0x00000008U
#define HAL_UART_ERROR_DMA 0x00000010U

/* UART handle — full struct for professional mock */
typedef struct { uint32_t Instance; uint32_t Init; uint8_t *pTxBuffPtr; uint16_t TxXferSize; uint16_t TxXferCount; uint8_t *pRxBuffPtr; uint16_t RxXferSize; uint16_t RxXferCount; HAL_UART_StateTypeDef gState; HAL_UART_StateTypeDef RxState; uint32_t ErrorCode; } UART_HandleTypeDef;

/* Other peripheral handles (minimal) */
typedef struct { uint32_t dummy; } I2C_HandleTypeDef;
typedef struct { uint32_t dummy; } SPI_HandleTypeDef;
typedef struct { uint32_t dummy; } TIM_HandleTypeDef;
HAL_StatusTypeDef HAL_GPIO_Init(GPIO_TypeDef* GPIOx, GPIO_InitTypeDef* GPIO_Init);
void HAL_GPIO_DeInit(GPIO_TypeDef* GPIOx, uint32_t GPIO_Pin);
void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
HAL_StatusTypeDef HAL_GPIO_LockPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin);
void HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin);
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_DMAPause(UART_HandleTypeDef* huart);
HAL_StatusTypeDef HAL_UART_DMAResume(UART_HandleTypeDef* huart);
HAL_StatusTypeDef HAL_UART_DMAStop(UART_HandleTypeDef* huart);

#endif /* MOCK_HAL_H */
