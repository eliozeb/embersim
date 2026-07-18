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

/* ---------- tick callbacks ---------- */
static EmberTickCallback tick_callbacks[8];
static int tick_cb_count = 0;

// global parent sequence (set before IRQ handler chain)
static uint32_t g_parent_seq = 0;

void ember_runtime_set_parent_seq(uint32_t seq) { g_parent_seq = seq; }
uint32_t ember_runtime_get_parent_seq(void) { return g_parent_seq; }

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
    if (g_parent_seq > 0) {
        snprintf(payload, sizeof(payload),
                 "\"origin\":\"software\",\"component\":\"%s\",\"event\":\"%s\",\"parent\":%u,\"depth\":1,\"details\":%s",
                 component, event, g_parent_seq, details_json ? details_json : "{}");
    } else {
        snprintf(payload, sizeof(payload),
                 "\"origin\":\"software\",\"component\":\"%s\",\"event\":\"%s\",\"details\":%s",
                 component, event, details_json ? details_json : "{}");
    }
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
    for (int i = 0; i < MAX_IRQS; i++) nvic_channels[i].priority = 15;
    tick_cb_count = 0;
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

/* ---------- highest priority pending IRQ ---------- */
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

/* ---------- step the runtime ---------- */
bool ember_runtime_step(void) {
    /* 1. Run tick callbacks – peripherals update registers and may set NVIC pending */
    for (int i = 0; i < tick_cb_count; i++) {
        tick_callbacks[i]();
    }

    /* 2. If no event, advance time by 1 µs and return (keep the loop alive) */
    if (!event_queue) {
        now_us++;
        return true;
    }

    /* 3. If the next event is in the future, advance time and return */
    if (event_queue->due_us > now_us) {
        now_us++;
        return true;
    }

    /* 4. Process the next event */
    EmberEvent *ev = event_queue;
    event_queue = ev->next;
    now_us = ev->due_us;
    seq_counter++;
    ev->seq = seq_counter;

    switch (ev->type) {
        case EVENT_TIM_UPDATE: {
            char details[128];
            snprintf(details, sizeof(details),
                     "{\"timer\":\"%08x\",\"cause\":\"update\"}", ev->source);
            trace_hardware_event("TIM", "timer_update", details);
            NVIC_SetPendingIRQ(TIM2_IRQn);
            break;
        }
        default: break;
    }
    free(ev);
    g_parent_seq = ev->seq;
    /* 5. Dispatch all pending IRQs */
    int irq;
    while ((irq = highest_pending_irq()) >= 0) {
        nvic_channels[irq].pending = false;
        nvic_channels[irq].active = true;
        char details[64];
        snprintf(details, sizeof(details), "{\"irq\":%d}", irq);
        trace_software_event("NVIC", "irq_enter", details);
        HAL_IRQ_Dispatch((IRQn_Type)irq);
        nvic_channels[irq].active = false;
    }
    // after the IRQ dispatch loop
    g_parent_seq = 0;

    return true;
}

void ember_runtime_run_until(uint32_t deadline_us) {
    while (now_us < deadline_us) {
        ember_runtime_step();
    }
}

/* ---------- tick callback registration ---------- */
void ember_runtime_on_tick(EmberTickCallback cb) {
    if (tick_cb_count < 8) {
        tick_callbacks[tick_cb_count++] = cb;
    }
}

/* ---------- peripheral registry ---------- */
void ember_runtime_register_peripheral(EmberPeripheral *p) {
    (void)p;
}
void ember_runtime_register_irq(uint32_t base_address, IRQn_Type irq) {
    (void)base_address;
    (void)irq;
}

/* ---------- NVIC implementation ---------- */
void NVIC_EnableIRQ(IRQn_Type irq)  { nvic_channels[(uint32_t)irq].enabled = true; }
void NVIC_DisableIRQ(IRQn_Type irq) { nvic_channels[(uint32_t)irq].enabled = false; }
void NVIC_SetPendingIRQ(IRQn_Type irq) { nvic_channels[(uint32_t)irq].pending = true; }
void NVIC_ClearPendingIRQ(IRQn_Type irq) { nvic_channels[(uint32_t)irq].pending = false; }
bool NVIC_GetPendingIRQ(IRQn_Type irq) { return nvic_channels[(uint32_t)irq].pending; }
void NVIC_SetPriority(IRQn_Type irq, uint32_t priority) {
    if ((uint32_t)irq < MAX_IRQS) nvic_channels[(uint32_t)irq].priority = priority;
}
uint32_t NVIC_GetPriority(IRQn_Type irq) {
    return ((uint32_t)irq < MAX_IRQS) ? nvic_channels[(uint32_t)irq].priority : 15;
}