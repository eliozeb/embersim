#ifndef MOCK_HAL_H
#define MOCK_HAL_H
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

typedef enum { HAL_OK = 0, HAL_ERROR = 1, HAL_BUSY = 2, HAL_TIMEOUT = 3 } HAL_StatusTypeDef;
typedef struct { volatile uint32_t dummy; } GPIO_TypeDef;
typedef struct { uint32_t dummy; } GPIO_InitTypeDef;
typedef enum { GPIO_PIN_RESET = 0, GPIO_PIN_SET = 1 } GPIO_PinState;
typedef struct { uint32_t Instance; } UART_HandleTypeDef;
typedef struct { uint32_t dummy; } I2C_HandleTypeDef;
typedef struct { uint32_t dummy; } SPI_HandleTypeDef;
typedef struct { uint32_t dummy; } TIM_HandleTypeDef;
typedef struct { uint32_t dummy; } DMA_HandleTypeDef;
typedef struct { uint32_t dummy; } ADC_HandleTypeDef;
typedef struct { uint32_t dummy; } DAC_HandleTypeDef;
typedef struct { uint32_t dummy; } RTC_HandleTypeDef;
typedef struct { uint32_t dummy; } CAN_HandleTypeDef;
typedef struct { uint32_t dummy; } CRC_HandleTypeDef;
typedef struct { uint32_t dummy; } ETH_HandleTypeDef;
typedef struct { uint32_t dummy; } HRTIM_HandleTypeDef;
typedef struct { uint32_t dummy; } SMBUS_HandleTypeDef;
typedef struct { uint32_t dummy; } WWDG_HandleTypeDef;
typedef struct { uint32_t dummy; } IWDG_HandleTypeDef;
typedef struct { uint32_t dummy; } LPTIM_HandleTypeDef;
typedef struct { uint32_t dummy; } OPAMP_HandleTypeDef;
typedef struct { uint32_t dummy; } COMP_HandleTypeDef;
typedef struct { uint32_t dummy; } CRYP_HandleTypeDef;
typedef struct { uint32_t dummy; } HASH_HandleTypeDef;
typedef struct { uint32_t dummy; } RNG_HandleTypeDef;
typedef struct { uint32_t dummy; } DCMI_HandleTypeDef;
typedef struct { uint32_t dummy; } DMA2D_HandleTypeDef;
typedef struct { uint32_t dummy; } LTDC_HandleTypeDef;
typedef struct { uint32_t dummy; } DSI_HandleTypeDef;
typedef struct { uint32_t dummy; } JPEG_HandleTypeDef;
typedef struct { uint32_t dummy; } MDIOS_HandleTypeDef;
typedef struct { uint32_t dummy; } SWPMI_HandleTypeDef;
typedef struct { uint32_t dummy; } SAI_HandleTypeDef;
typedef struct { uint32_t dummy; } SD_HandleTypeDef;
typedef struct { uint32_t dummy; } MMC_HandleTypeDef;
typedef struct { uint32_t dummy; } NAND_HandleTypeDef;
typedef struct { uint32_t dummy; } NOR_HandleTypeDef;
typedef struct { uint32_t dummy; } SRAM_HandleTypeDef;
typedef struct { uint32_t dummy; } SDRAM_HandleTypeDef;
typedef struct { uint32_t dummy; } CEC_HandleTypeDef;
typedef struct { uint32_t dummy; } QSPI_HandleTypeDef;
typedef struct { uint32_t dummy; } FMPSMBUS_HandleTypeDef;
typedef struct { uint32_t dummy; } HCD_HandleTypeDef;
typedef struct { uint32_t dummy; } PCD_HandleTypeDef;
typedef struct { uint32_t dummy; } IRDA_HandleTypeDef;
typedef struct { uint32_t dummy; } SMARTCARD_HandleTypeDef;
typedef struct { uint32_t dummy; } PCDEx_HandleTypeDef;
typedef struct { uint32_t dummy; } HCDEx_HandleTypeDef;

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
