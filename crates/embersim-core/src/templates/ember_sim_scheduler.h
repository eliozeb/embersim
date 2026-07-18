#ifndef EMBER_SIM_SCHEDULER_H
#define EMBER_SIM_SCHEDULER_H

#include <stdint.h>
#include <stdbool.h>

/* ---------- event types ---------- */
typedef enum {
    EVENT_TIM_UPDATE,
    EVENT_UART_TX_DONE,
    EVENT_UART_RX_DONE,
    EVENT_SPI_TX_DONE,
    EVENT_SPI_RX_DONE,
    EVENT_SPI_TXRX_DONE,
    EVENT_I2C_MASTER_TX_DONE,
    EVENT_I2C_MASTER_RX_DONE,
    EVENT_I2C_MEM_TX_DONE,
    EVENT_I2C_MEM_RX_DONE,
    EVENT_DMA_DONE,
    EVENT_EXTI,
    EVENT_USER,           /* for custom injected events */
    EVENT_COUNT
} EmberEventType;

/* ---------- event structure ---------- */
typedef struct EmberEvent {
    uint32_t        due_us;        /* absolute time in µs */
    EmberEventType  type;
    void           *peripheral;    /* pointer to handle, instance, etc. */
    uint32_t        param;         /* extra data (channel, IRQ number, …) */
    struct EmberEvent *next;
} EmberEvent;

/* ---------- scheduler API ---------- */
void        ember_sim_init(uint32_t start_time_us);
uint32_t    ember_sim_now_us(void);
void        ember_sim_schedule_event(uint32_t delay_us, EmberEventType type,
                                     void *peripheral, uint32_t param);
bool        ember_sim_run_step(void);   /* execute next event, return false if queue empty */
void        ember_sim_run_until(uint32_t deadline_us);

/* ---------- NVIC stubs ---------- */
typedef enum {
    TIM2_IRQn = 28,
    USART1_IRQn = 37,
    SPI1_IRQn = 35,
    I2C1_IRQn = 31,
    DMA1_Stream0_IRQn = 11,
} IRQn_Type;

void NVIC_SetPendingIRQ(IRQn_Type irq);
void NVIC_ClearPendingIRQ(IRQn_Type irq);
bool NVIC_GetPendingIRQ(IRQn_Type irq);

/* User implemented: map IRQ to handler */
void HAL_IRQ_Handler(IRQn_Type irq);

#endif