#ifndef EMBER_EVENT_BUS_H
#define EMBER_EVENT_BUS_H

#include <stdint.h>
#include <stdbool.h>

/* ---------- event types (extensible) ---------- */
typedef enum {
    EMBER_EVT_HW_TIMER_UPDATE,
    EMBER_EVT_HW_UART_TX_DONE,
    EMBER_EVT_HW_UART_RX_DONE,
    EMBER_EVT_HW_SPI_TX_DONE,
    EMBER_EVT_HW_SPI_RX_DONE,
    EMBER_EVT_HW_I2C_MASTER_TX_DONE,
    EMBER_EVT_HW_I2C_MASTER_RX_DONE,
    EMBER_EVT_REGISTER_CHANGED,     /* payload: ember_reg_change_t */
    EMBER_EVT_COUNT
} EmberEventType;

/* ---------- payload for register change events ---------- */
typedef struct {
    const char *peripheral;
    uint32_t    address;
    const char *reg_name;
    uint32_t    old_value;
    uint32_t    new_value;
    const char *reason;         /* e.g. "overflow", "HAL cleared" */
} EmberRegChange;

/* ---------- generic event payload ---------- */
typedef struct EmberEvent {
    uint32_t        timestamp_us;   /* simulated time */
    EmberEventType  type;
    uint32_t        source;         /* peripheral base address */
    uint32_t        param;          /* extra data (channel, irq, …) */
    union {
        EmberRegChange reg;         /* used for REGISTER_CHANGED */
    } data;
} EmberEvent;

/* ---------- subscriber callback ---------- */
typedef void (*EmberEventCallback)(const EmberEvent *ev);

/* ---------- bus API ---------- */
void ember_bus_init(void);
void ember_bus_subscribe(EmberEventType type, EmberEventCallback cb);
void ember_bus_publish(const EmberEvent *ev);

#endif