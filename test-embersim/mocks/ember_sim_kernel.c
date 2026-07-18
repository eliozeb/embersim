#include "ember_sim_kernel.h"
#include "trace_log.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_PERIPHERALS 8
#define MAX_EVENTS      64

static EmberPeripheral *peripherals[MAX_PERIPHERALS];
static int              peripheral_count = 0;

static KernelEvent      event_pool[MAX_EVENTS];
static KernelEvent*     free_list = NULL;
static KernelEvent*     queue_head = NULL;
static uint32_t         next_event_id = 1;
static uint64_t         sim_time_us = 0;

// ---------- event pool ----------
static void init_event_pool(void) {
    for (int i = 0; i < MAX_EVENTS; i++) {
        event_pool[i].next = (i < MAX_EVENTS-1) ? &event_pool[i+1] : NULL;
    }
    free_list = &event_pool[0];
}

static KernelEvent* alloc_event(void) {
    if (!free_list) return NULL;
    KernelEvent *ev = free_list;
    free_list = ev->next;
    memset(ev, 0, sizeof(KernelEvent));
    return ev;
}

static void free_event(KernelEvent *ev) {
    ev->next = free_list;
    free_list = ev;
}

// ---------- sorted insert ----------
static void insert_event(KernelEvent *ev) {
    if (!queue_head || ev->timestamp_us < queue_head->timestamp_us) {
        ev->next = queue_head;
        queue_head = ev;
        return;
    }
    KernelEvent *cur = queue_head;
    while (cur->next && cur->next->timestamp_us <= ev->timestamp_us)
        cur = cur->next;
    ev->next = cur->next;
    cur->next = ev;
}

// ---------- process all queued events due now ----------
static void process_events(void) {
    while (queue_head && queue_head->timestamp_us <= sim_time_us) {
        KernelEvent *ev = queue_head;
        queue_head = ev->next;

        // trace and dispatch
        switch (ev->type) {
            case KERN_EVT_REGISTER_CHANGED: {
                // the payload is logged inside ember_reg_write via trace_reg_change,
                // so nothing extra needed here.
                break;
            }
            case KERN_EVT_TIM_UPDATE: {
                char details[128];
                snprintf(details, sizeof(details),
                         "{\"timer\":\"%08x\",\"cause\":\"update\"}", ev->source);
                trace_log("HARDWARE_EVENT", details);
                break;
            }
            default: break;
        }

        // let the target peripheral handle the event
        for (int i = 0; i < peripheral_count; i++) {
            if (peripherals[i]->base_address == ev->source && peripherals[i]->handle_event) {
                peripherals[i]->handle_event(peripherals[i], ev);
            }
        }

        free_event(ev);
    }
}

// ---------- public kernel API ----------
void kernel_init(void) {
    init_event_pool();
    peripheral_count = 0;
    next_event_id = 1;
    sim_time_us = 0;
    queue_head = NULL;
}

uint64_t kernel_now_us(void) {
    return sim_time_us;
}

void kernel_register_peripheral(EmberPeripheral *p) {
    if (peripheral_count < MAX_PERIPHERALS) {
        peripherals[peripheral_count++] = p;
        if (p->init) p->init(p);
    }
}

void kernel_schedule_event(uint32_t delay_us, KernelEventType type,
                           uint32_t source, uint32_t param, uint32_t parent_event_id) {
    KernelEvent *ev = alloc_event();
    if (!ev) return;
    ev->event_id     = next_event_id++;
    ev->parent_id    = parent_event_id;
    ev->timestamp_us = sim_time_us + delay_us;
    ev->type         = type;
    ev->source       = source;
    ev->param        = param;
    insert_event(ev);
}

void kernel_advance_ticks(uint32_t us) {
    for (uint32_t i = 0; i < us; i++) {
        for (int j = 0; j < peripheral_count; j++) {
            if (peripherals[j]->tick) {
                peripherals[j]->tick(peripherals[j], sim_time_us);
            }
        }
        sim_time_us++;
    }
}

void kernel_dispatch_pending(void) {
    process_events();
}

void kernel_run_until(uint64_t deadline_us) {
    while (sim_time_us < deadline_us) {
        kernel_advance_ticks(1);
        kernel_dispatch_pending();
    }
}

// ---------- NVIC stubs ----------
static bool nvic_pending[64];
static bool nvic_enabled[64];

void nvic_set_pending(uint32_t irq) { nvic_pending[irq] = true; }
void nvic_clear_pending(uint32_t irq) { nvic_pending[irq] = false; }
bool nvic_is_pending(uint32_t irq) { return nvic_pending[irq]; }
void nvic_enable(uint32_t irq) { nvic_enabled[irq] = true; }
void nvic_disable(uint32_t irq) { nvic_enabled[irq] = false; }

// ---------- trace helper (for mock_tim.c) ----------
void trace_software_event(const char *component, const char *event, const char *details_json) {
    char payload[512];
    snprintf(payload, sizeof(payload),
             "\"origin\":\"software\",\"component\":\"%s\",\"event\":\"%s\",\"details\":%s",
             component, event, details_json ? details_json : "{}");
    trace_log("SOFTWARE_EVENT", payload);
}