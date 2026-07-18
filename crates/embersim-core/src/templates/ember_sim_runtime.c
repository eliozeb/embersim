#include "ember_sim_runtime.h"
#include "trace_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* ---------- global state ---------- */
static EmberEvent  *event_queue = NULL;
static uint32_t     now_us      = 0;
static uint32_t     seq_counter = 0;
static NVIC_Channel nvic_channels[MAX_IRQS];

/* ---------- trace helpers ---------- */
void trace_hardware_event(const char *component, const char *event, const char *details_json) {
    char payload[512];
    snprintf(payload, sizeof(payload),
             "\"origin\":\"hardware\",\"component\":\"%s\",\"event\":\"%s\",\"details\":%s",
             component, event, details_json ? details_json : "{}");
    trace_log("HARDWARE_EVENT", payload);
}

void trace_software_event(const char *component, const char *event, const char *details_json) {
    char payload[512];
    snprintf(payload, sizeof(payload),
             "\"origin\":\"software\",\"component\":\"%s\",\"event\":\"%s\",\"details\":%s",
             component, event, details_json ? details_json : "{}");
    trace_log("SOFTWARE_EVENT", payload);
}

/* ---------- runtime initialisation ---------- */
void ember_runtime_init(uint32_t start_time_us) {
    now_us = start_time_us;
    seq_counter = 0;
    while (event_queue) {
        EmberEvent *ev = event_queue;
        event_queue = ev->next;
        free(ev);
    }
    memset(nvic_channels, 0, sizeof(nvic_channels));
    /* default priorities: 15 (lowest) */
    for (int i = 0; i < MAX_IRQS; i++) nvic_channels[i].priority = 15;
}

uint32_t ember_runtime_now_us(void) {
    return now_us;
}

void ember_runtime_schedule(uint32_t delay_us, EmberEventType type,
                            uint32_t source, uint32_t param) {
    EmberEvent *ev = malloc(sizeof(EmberEvent));
    ev->due_us = now_us + delay_us;
    ev->type   = type;
    ev->source = source;
    ev->param  = param;
    ev->next   = NULL;

    /* insert sorted by due_us, then by seq if same time */
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

/* ---------- find highest priority pending IRQ ---------- */
static int highest_pending_irq(void) {
    int best = -1;
    uint32_t best_prio = 16;
    for (int i = 0; i < MAX_IRQS; i++) {
        if (nvic_channels[i].enabled && nvic_channels[i].pending &&
            nvic_channels[i].priority < best_prio) {
            best = i;
            best_prio = nvic_channels[i].priority;
        }
    }
    return best;
}

/* ---------- process one event + any resulting IRQs ---------- */
bool ember_runtime_step(void) {
    if (!event_queue) return false;

    EmberEvent *ev = event_queue;
    event_queue = ev->next;

    /* advance time and assign sequence */
    now_us = ev->due_us;
    seq_counter++;
    ev->seq = seq_counter;

    /* Log hardware event */
    switch (ev->type) {
        case EVENT_TIM_UPDATE: {
            char details[128];
            snprintf(details, sizeof(details),
                     "{\"timer\":\"%08x\",\"cause\":\"update\"}", ev->source);
            trace_hardware_event("TIM", "timer_update", details);
            /* raise interrupt */
            NVIC_SetPendingIRQ(TIM2_IRQn);   // temporary: map TIM2 base to TIM2_IRQn
            break;
        }
        case EVENT_UART_TX_DONE: {
            char details[128];
            snprintf(details, sizeof(details), "{\"uart\":\"%08x\"}", ev->source);
            trace_hardware_event("UART", "tx_done", details);
            NVIC_SetPendingIRQ(USART1_IRQn);
            break;
        }
        // ... other cases ...
        default:
            break;
    }

    free(ev);

    /* dispatch all pending IRQs (no pre‑emption for now) */
    int irq;
    while ((irq = highest_pending_irq()) >= 0) {
        nvic_channels[irq].pending = false;
        nvic_channels[irq].active = true;

        /* log IRQ entry */
        char details[64];
        snprintf(details, sizeof(details), "{\"irq\":%d}", irq);
        trace_software_event("NVIC", "irq_enter", details);

        /* call the user's dispatcher */
        HAL_IRQ_Dispatch((IRQn_Type)irq);

        nvic_channels[irq].active = false;
    }

    return true;
}

void ember_runtime_run_until(uint32_t deadline_us) {
    while (event_queue && event_queue->due_us <= deadline_us) {
        ember_runtime_step();
    }
    now_us = deadline_us;
}

/* ---------- NVIC implementation ---------- */
void NVIC_EnableIRQ(IRQn_Type irq)  { nvic_channels[(uint32_t)irq].enabled = true; }
void NVIC_DisableIRQ(IRQn_Type irq) { nvic_channels[(uint32_t)irq].enabled = false; }
void NVIC_SetPendingIRQ(IRQn_Type irq) {
    nvic_channels[(uint32_t)irq].pending = true;
}
void NVIC_ClearPendingIRQ(IRQn_Type irq) {
    nvic_channels[(uint32_t)irq].pending = false;
}
bool NVIC_GetPendingIRQ(IRQn_Type irq) {
    return nvic_channels[(uint32_t)irq].pending;
}
void NVIC_SetPriority(IRQn_Type irq, uint32_t priority) {
    if ((uint32_t)irq < MAX_IRQS) nvic_channels[(uint32_t)irq].priority = priority;
}
uint32_t NVIC_GetPriority(IRQn_Type irq) {
    return ((uint32_t)irq < MAX_IRQS) ? nvic_channels[(uint32_t)irq].priority : 15;
}