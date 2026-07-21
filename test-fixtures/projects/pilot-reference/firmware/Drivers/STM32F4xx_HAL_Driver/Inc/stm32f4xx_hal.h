/**
  ******************************************************************************
  * @file    stm32f4xx_hal.h
  * @brief   STM32F4xx HAL master header — representative subset for Pilot 0.
  *
  *          This is NOT the full STM32Cube HAL. It includes the types and
  *          function declarations needed by the Pilot 0 reference firmware:
  *          GPIO, UART, TIM (base + PWM), SPI, and minimal RCC.
  *
  *          Purpose: provide a realistic HAL header that EmberSim can parse
  *          to generate mock stubs, without pulling in the full Cube package.
  ******************************************************************************
  */

#ifndef __STM32F4XX_HAL_H
#define __STM32F4XX_HAL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

/* ==========================================================================
   HAL Status
   ========================================================================== */

typedef enum {
    HAL_OK       = 0x00U,
    HAL_ERROR    = 0x01U,
    HAL_BUSY     = 0x02U,
    HAL_TIMEOUT  = 0x03U
} HAL_StatusTypeDef;

/* ==========================================================================
   HAL Lock
   ========================================================================== */

typedef enum {
    HAL_UNLOCKED = 0x00U,
    HAL_LOCKED   = 0x01U
} HAL_LockTypeDef;

/* ==========================================================================
   GPIO
   ========================================================================== */

#define GPIO_PIN_0   ((uint16_t)0x0001)
#define GPIO_PIN_1   ((uint16_t)0x0002)
#define GPIO_PIN_2   ((uint16_t)0x0004)
#define GPIO_PIN_3   ((uint16_t)0x0008)
#define GPIO_PIN_4   ((uint16_t)0x0010)
#define GPIO_PIN_5   ((uint16_t)0x0020)
#define GPIO_PIN_6   ((uint16_t)0x0040)
#define GPIO_PIN_7   ((uint16_t)0x0080)
#define GPIO_PIN_8   ((uint16_t)0x0100)
#define GPIO_PIN_9   ((uint16_t)0x0200)
#define GPIO_PIN_10  ((uint16_t)0x0400)
#define GPIO_PIN_11  ((uint16_t)0x0800)
#define GPIO_PIN_12  ((uint16_t)0x1000)
#define GPIO_PIN_13  ((uint16_t)0x2000)
#define GPIO_PIN_14  ((uint16_t)0x4000)
#define GPIO_PIN_15  ((uint16_t)0x8000)
#define GPIO_PIN_ALL ((uint16_t)0xFFFF)

#define GPIO_MODE_INPUT         0x00000000U
#define GPIO_MODE_OUTPUT_PP     0x00000001U
#define GPIO_MODE_OUTPUT_OD     0x00000011U
#define GPIO_MODE_AF_PP         0x00000002U
#define GPIO_MODE_AF_OD         0x00000012U
#define GPIO_MODE_ANALOG        0x00000003U
#define GPIO_MODE_IT_RISING     0x10110000U
#define GPIO_MODE_IT_FALLING    0x10210000U
#define GPIO_MODE_IT_RISING_FALLING 0x10310000U

#define GPIO_NOPULL        0x00000000U
#define GPIO_PULLUP        0x00000001U
#define GPIO_PULLDOWN      0x00000002U

#define GPIO_SPEED_FREQ_LOW    0x00000000U
#define GPIO_SPEED_FREQ_MEDIUM 0x00000001U
#define GPIO_SPEED_FREQ_HIGH   0x00000002U
#define GPIO_SPEED_FREQ_VERY_HIGH 0x00000003U

typedef struct {
    volatile uint32_t MODER;
    volatile uint32_t OTYPER;
    volatile uint32_t OSPEEDR;
    volatile uint32_t PUPDR;
    volatile uint32_t IDR;
    volatile uint32_t ODR;
    volatile uint32_t BSRR;
    volatile uint32_t LCKR;
    volatile uint32_t AFR[2];
} GPIO_TypeDef;

typedef struct {
    uint32_t Pin;
    uint32_t Mode;
    uint32_t Pull;
    uint32_t Speed;
    uint32_t Alternate;
} GPIO_InitTypeDef;

typedef enum {
    GPIO_PIN_RESET = 0U,
    GPIO_PIN_SET
} GPIO_PinState;

#define GPIOA  ((GPIO_TypeDef *)0x40020000U)
#define GPIOB  ((GPIO_TypeDef *)0x40020400U)
#define GPIOC  ((GPIO_TypeDef *)0x40020800U)

void              HAL_GPIO_Init(GPIO_TypeDef *GPIOx, GPIO_InitTypeDef *GPIO_Init);
void              HAL_GPIO_DeInit(GPIO_TypeDef *GPIOx, uint32_t GPIO_Pin);
GPIO_PinState     HAL_GPIO_ReadPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void              HAL_GPIO_WritePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState);
void              HAL_GPIO_TogglePin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
HAL_StatusTypeDef HAL_GPIO_LockPin(GPIO_TypeDef *GPIOx, uint16_t GPIO_Pin);
void              HAL_GPIO_EXTI_IRQHandler(uint16_t GPIO_Pin);
void              HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin);

/* ==========================================================================
   UART
   ========================================================================== */

#define UART_WORDLENGTH_8B   0x00000000U
#define UART_WORDLENGTH_9B   0x00001000U

#define UART_STOPBITS_1      0x00000000U
#define UART_STOPBITS_2      0x00002000U

#define UART_PARITY_NONE     0x00000000U
#define UART_PARITY_EVEN     0x00000400U
#define UART_PARITY_ODD      0x00000600U

#define UART_MODE_TX         0x00000008U
#define UART_MODE_RX         0x00000004U
#define UART_MODE_TX_RX      0x0000000CU

#define UART_HWCONTROL_NONE  0x00000000U
#define UART_HWCONTROL_RTS   0x00000100U
#define UART_HWCONTROL_CTS   0x00000200U
#define UART_HWCONTROL_RTS_CTS 0x00000300U

#define UART_OVERSAMPLING_16 0x00000000U
#define UART_OVERSAMPLING_8  0x00008000U

typedef struct {
    uint32_t BaudRate;
    uint32_t WordLength;
    uint32_t StopBits;
    uint32_t Parity;
    uint32_t Mode;
    uint32_t HwFlowCtl;
    uint32_t OverSampling;
} UART_InitTypeDef;

typedef struct {
    uint32_t Instance;
    UART_InitTypeDef Init;
    uint8_t  *pTxBuffPtr;
    uint16_t TxXferSize;
    uint16_t TxXferCount;
    uint8_t  *pRxBuffPtr;
    uint16_t RxXferSize;
    uint16_t RxXferCount;
    HAL_LockTypeDef Lock;
    uint32_t ErrorCode;
} UART_HandleTypeDef;

#define USART2  ((uint32_t)0x40004400U)

HAL_StatusTypeDef HAL_UART_Init(UART_HandleTypeDef *huart);
HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef *huart, uint8_t *pData, uint16_t Size);
void              HAL_UART_IRQHandler(UART_HandleTypeDef *huart);
void              HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void              HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void              HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);

/* ==========================================================================
   TIM
   ========================================================================== */

#define TIM_CHANNEL_1  0x00000000U
#define TIM_CHANNEL_2  0x00000004U
#define TIM_CHANNEL_3  0x00000008U
#define TIM_CHANNEL_4  0x0000000CU

#define TIM_COUNTERMODE_UP   0x00000000U
#define TIM_COUNTERMODE_DOWN 0x00000010U

#define TIM_OCMODE_TIMING    0x00000000U
#define TIM_OCMODE_ACTIVE    0x00000010U
#define TIM_OCMODE_INACTIVE  0x00000020U
#define TIM_OCMODE_TOGGLE    0x00000030U
#define TIM_OCMODE_PWM1      0x00000060U
#define TIM_OCMODE_PWM2      0x00000070U

#define TIM_OCPOLARITY_HIGH 0x00000000U
#define TIM_OCPOLARITY_LOW  0x00000002U

typedef struct {
    uint32_t Prescaler;
    uint32_t CounterMode;
    uint32_t Period;
    uint32_t ClockDivision;
    uint32_t RepetitionCounter;
    uint32_t AutoReloadPreload;
} TIM_Base_InitTypeDef;

typedef struct {
    uint32_t OCMode;
    uint32_t Pulse;
    uint32_t OCPolarity;
    uint32_t OCNPolarity;
    uint32_t OCFastMode;
    uint32_t OCIdleState;
    uint32_t OCNIdleState;
} TIM_OC_InitTypeDef;

typedef struct {
    uint32_t Instance;
    TIM_Base_InitTypeDef Init;
    uint32_t Channel;
    HAL_LockTypeDef Lock;
    uint32_t State;
} TIM_HandleTypeDef;

#define TIM2  ((uint32_t)0x40000400U)

HAL_StatusTypeDef HAL_TIM_Base_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_Base_Start_IT(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_Base_Stop_IT(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_PWM_Init(TIM_HandleTypeDef *htim);
HAL_StatusTypeDef HAL_TIM_PWM_ConfigChannel(TIM_HandleTypeDef *htim, TIM_OC_InitTypeDef *sConfig, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_PWM_Start(TIM_HandleTypeDef *htim, uint32_t Channel);
HAL_StatusTypeDef HAL_TIM_PWM_Stop(TIM_HandleTypeDef *htim, uint32_t Channel);
void              HAL_TIM_IRQHandler(TIM_HandleTypeDef *htim);
void              HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim);
void              HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim);

/* ==========================================================================
   SPI
   ========================================================================== */

#define SPI_MODE_MASTER  0x00000004U
#define SPI_MODE_SLAVE   0x00000000U

#define SPI_DIRECTION_2LINES      0x00000000U
#define SPI_DIRECTION_2LINES_RXONLY 0x00000400U
#define SPI_DIRECTION_1LINE       0x00008000U

#define SPI_DATASIZE_8BIT  0x00000000U
#define SPI_DATASIZE_16BIT 0x00000800U

#define SPI_CPOL_LOW  0x00000000U
#define SPI_CPOL_HIGH 0x00000002U
#define SPI_CPHA_1EDGE 0x00000000U
#define SPI_CPHA_2EDGE 0x00000001U

#define SPI_NSS_SOFT  0x00000200U
#define SPI_NSS_HARD_INPUT  0x00000000U
#define SPI_NSS_HARD_OUTPUT 0x00000400U

#define SPI_BAUDRATEPRESCALER_2   0x00000000U
#define SPI_BAUDRATEPRESCALER_4   0x00000008U
#define SPI_BAUDRATEPRESCALER_8   0x00000010U
#define SPI_BAUDRATEPRESCALER_16  0x00000018U
#define SPI_BAUDRATEPRESCALER_32  0x00000020U
#define SPI_BAUDRATEPRESCALER_64  0x00000028U
#define SPI_BAUDRATEPRESCALER_128 0x00000030U
#define SPI_BAUDRATEPRESCALER_256 0x00000038U

#define SPI_FIRSTBIT_MSB 0x00000000U
#define SPI_FIRSTBIT_LSB 0x00000080U

typedef struct {
    uint32_t Mode;
    uint32_t Direction;
    uint32_t DataSize;
    uint32_t CLKPolarity;
    uint32_t CLKPhase;
    uint32_t NSS;
    uint32_t BaudRatePrescaler;
    uint32_t FirstBit;
    uint32_t TIMode;
    uint32_t CRCCalculation;
    uint32_t CRCPolynomial;
} SPI_InitTypeDef;

typedef struct {
    uint32_t Instance;
    SPI_InitTypeDef Init;
    uint8_t  *pTxBuffPtr;
    uint16_t TxXferSize;
    uint16_t TxXferCount;
    uint8_t  *pRxBuffPtr;
    uint16_t RxXferSize;
    uint16_t RxXferCount;
    HAL_LockTypeDef Lock;
    uint32_t ErrorCode;
} SPI_HandleTypeDef;

#define SPI1  ((uint32_t)0x40013000U)

HAL_StatusTypeDef HAL_SPI_Init(SPI_HandleTypeDef *hspi);
HAL_StatusTypeDef HAL_SPI_Transmit(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_Receive(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_TransmitReceive(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size, uint32_t Timeout);
HAL_StatusTypeDef HAL_SPI_Transmit_IT(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_Receive_IT(SPI_HandleTypeDef *hspi, uint8_t *pData, uint16_t Size);
HAL_StatusTypeDef HAL_SPI_TransmitReceive_IT(SPI_HandleTypeDef *hspi, uint8_t *pTxData, uint8_t *pRxData, uint16_t Size);
void              HAL_SPI_IRQHandler(SPI_HandleTypeDef *hspi);
void              HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi);
void              HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi);
void              HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi);
void              HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi);

/* ==========================================================================
   RCC (minimal — needed by SystemClock_Config)
   ========================================================================== */

typedef struct {
    uint32_t OscillatorType;
    uint32_t HSEState;
    uint32_t LSEState;
    uint32_t HSIState;
    uint32_t HSICalibrationValue;
    uint32_t LSIState;
    uint32_t PLL;
    uint32_t PLLSource;
    uint32_t PLLM;
    uint32_t PLLN;
    uint32_t PLLP;
    uint32_t PLLQ;
    uint32_t SYSCLKSource;
    uint32_t AHBCLKDivider;
    uint32_t APB1CLKDivider;
    uint32_t APB2CLKDivider;
} RCC_OscInitTypeDef;

typedef struct {
    uint32_t ClockType;
    uint32_t SYSCLKSource;
    uint32_t AHBCLKDivider;
    uint32_t APB1CLKDivider;
    uint32_t APB2CLKDivider;
} RCC_ClkInitTypeDef;

HAL_StatusTypeDef HAL_RCC_OscConfig(RCC_OscInitTypeDef *RCC_OscInitStruct);
HAL_StatusTypeDef HAL_RCC_ClockConfig(RCC_ClkInitTypeDef *RCC_ClkInitStruct, uint32_t FLatency);

/* ==========================================================================
   NVIC (minimal — interrupt controller)
   ========================================================================== */

typedef enum {
    EXTI0_IRQn     = 6,
    EXTI1_IRQn     = 7,
    USART2_IRQn    = 38,
    TIM2_IRQn      = 28,
    SPI1_IRQn      = 35,
} IRQn_Type;

void HAL_NVIC_SetPriority(IRQn_Type IRQn, uint32_t PreemptPriority, uint32_t SubPriority);
void HAL_NVIC_EnableIRQ(IRQn_Type IRQn);
void HAL_NVIC_DisableIRQ(IRQn_Type IRQn);

/* ==========================================================================
   System
   ========================================================================== */

HAL_StatusTypeDef HAL_Init(void);

#ifdef __cplusplus
}
#endif

#endif /* __STM32F4XX_HAL_H */
