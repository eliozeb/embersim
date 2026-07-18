#include "ember_sim_scheduler.h"
#include "trace_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---------- global state ---------- */
static EmberEvent *event_queue = NULL;
static uint32_t    now_us = 0;

/* ---------- NVIC pending table ---------- */
static bool nvic_pending[64];

/* ---------- scheduler implementation ---------- */
void ember_sim_init(uint32_t start_time_us) {
    now_us = start_time_us;
    while (event_queue) {
        EmberEvent *ev = event_queue;
        event_queue = ev->next;
        free(ev);
    }
    memset(nvic_pending, 0, sizeof(nvic_pending));
}

uint32_t ember_sim_now_us(void) {
    return now_us;
}

void ember_sim_schedule_event(uint32_t delay_us, EmberEventType type,
                              void *peripheral, uint32_t param) {
    EmberEvent *ev = malloc(sizeof(EmberEvent));
    ev->due_us     = now_us + delay_us;
    ev->type       = type;
    ev->peripheral = peripheral;
    ev->param      = param;
    ev->next       = NULL;

    /* insert sorted by due time */
    if (!event_queue || event_queue->due_us > ev->due_us) {
        ev->next = event_queue;
        event_queue = ev;
        return;
    }
    EmberEvent *cur = event_queue;
    while (cur->next && cur->next->due_us <= ev->due_us)
        cur = cur->next;
    ev->next = cur->next;
    cur->next = ev;
}

bool ember_sim_run_step(void) {
    if (!event_queue) return false;

    EmberEvent *ev = event_queue;
    event_queue = ev->next;

    /* advance time */
    now_us = ev->due_us;

    /* dispatch */
    switch (ev->type) {
        /* TIM events → NVIC */
        case EVENT_TIM_UPDATE: {
            NVIC_SetPendingIRQ(TIM2_IRQn);
            break;
        }
        /* UART, SPI, I2C events → NVIC (to be expanded) */
        case EVENT_UART_TX_DONE:
        case EVENT_UART_RX_DONE:
        case EVENT_SPI_TX_DONE:
        case EVENT_SPI_RX_DONE:
        case EVENT_SPI_TXRX_DONE:
        case EVENT_I2C_MASTER_TX_DONE:
        case EVENT_I2C_MASTER_RX_DONE:
        case EVENT_I2C_MEM_TX_DONE:
        case EVENT_I2C_MEM_RX_DONE:
            /* placeholder – will set appropriate IRQ later */
            break;
        default:
            break;
    }

    /* process any pending IRQs that have become active */
    for (int i = 0; i < 64; i++) {
        if (nvic_pending[i]) {
            nvic_pending[i] = false;
            HAL_IRQ_Handler((IRQn_Type)i);
        }
    }

    free(ev);
    return true;
}

void ember_sim_run_until(uint32_t deadline_us) {
    while (event_queue && event_queue->due_us <= deadline_us) {
        ember_sim_run_step();
    }
    now_us = deadline_us;
}

/* ---------- NVIC implementation ---------- */
void NVIC_SetPendingIRQ(IRQn_Type irq) {
    nvic_pending[(uint32_t)irq] = true;
}
void NVIC_ClearPendingIRQ(IRQn_Type irq) {
    nvic_pending[(uint32_t)irq] = false;
}
bool NVIC_GetPendingIRQ(IRQn_Type irq) {
    return nvic_pending[(uint32_t)irq];
}

/* Weak default IRQ dispatcher – user overrides per peripheral */
__attribute__((weak)) void HAL_IRQ_Handler(IRQn_Type irq) {
    char payload[64];
    snprintf(payload, sizeof(payload), "\"irq\":%d", (int)irq);
    trace_log("HAL_IRQ_Handler", payload);
}