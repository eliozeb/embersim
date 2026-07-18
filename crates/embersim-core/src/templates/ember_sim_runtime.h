#ifndef EMBER_SIM_RUNTIME_H
#define EMBER_SIM_RUNTIME_H

#include <stdint.h>
#include <stdbool.h>

/* ---------- NVIC (interrupt controller) ---------- */
#define MAX_IRQS    64

typedef enum {
    TIM2_IRQn = 28,
    USART1_IRQn = 37,
    SPI1_IRQn = 35,
    I2C1_IRQn = 31,
} IRQn_Type;

/* ---------- generic event types ---------- */
typedef enum {
    EVENT_TIM_UPDATE,        // timer update (overflow, compare, input capture)
    EVENT_UART_TX_DONE,      // UART transmit complete
    EVENT_UART_RX_DONE,      // UART receive complete
    EVENT_SPI_TX_DONE,       // SPI transmit complete
    EVENT_SPI_RX_DONE,       // SPI receive complete
    EVENT_SPI_TXRX_DONE,     // SPI transmit‑receive complete
    EVENT_I2C_MASTER_TX_DONE,
    EVENT_I2C_MASTER_RX_DONE,
    EVENT_I2C_MEM_TX_DONE,
    EVENT_I2C_MEM_RX_DONE,
    EVENT_DMA_DONE,
    EVENT_EXTI,
    EVENT_COUNT
} EmberEventType;

typedef struct EmberPeripheral {
    const char *name;
    uint32_t    base_address;
    void (*init)(void);
    void (*reset)(void);
    void (*handle_irq)(void *handle);
} EmberPeripheral;

void ember_runtime_register_peripheral(EmberPeripheral *p);
void ember_runtime_register_irq(uint32_t base_address, IRQn_Type irq);

/* ---------- runtime event structure ---------- */
typedef struct EmberEvent {
    uint32_t        due_us;        // absolute time in microseconds
    uint32_t        seq;           // global sequence number
    EmberEventType  type;
    uint32_t        source;        // e.g. TIM2 base address or USART1 base
    uint32_t        param;         // e.g. channel, IRQ number
    struct EmberEvent *next;
} EmberEvent;

typedef struct {
    bool enabled;
    bool pending;
    bool active;
    uint32_t priority;      // 0 = highest, 15 = lowest
} NVIC_Channel;

/* ---------- runtime API ---------- */
void        ember_runtime_init(uint32_t start_time_us);
uint32_t    ember_runtime_now_us(void);
void        ember_runtime_schedule(uint32_t delay_us, EmberEventType type,
                                   uint32_t source, uint32_t param);

/* run one step: pop next event, set time, process NVIC, return true if event processed */
bool        ember_runtime_step(void);

/* run until absolute time */
void        ember_runtime_run_until(uint32_t deadline_us);

/* NVIC control */
void        NVIC_EnableIRQ(IRQn_Type irq);
void        NVIC_DisableIRQ(IRQn_Type irq);
void        NVIC_SetPendingIRQ(IRQn_Type irq);
void        NVIC_ClearPendingIRQ(IRQn_Type irq);
bool        NVIC_GetPendingIRQ(IRQn_Type irq);
void        NVIC_SetPriority(IRQn_Type irq, uint32_t priority);
uint32_t    NVIC_GetPriority(IRQn_Type irq);

/* user must implement: map an IRQ to the HAL handler */
void        HAL_IRQ_Dispatch(IRQn_Type irq);

/* trace envelope */
void        trace_hardware_event(const char *component, const char *event, const char *details);
void        trace_software_event(const char *component, const char *event, const char *details);

#endif