#include "tim_regs.h"
#include "ember_sim_runtime.h"
#include "trace_log.h"
#include <string.h>
#include <stdio.h>

#define MAX_TIM_INSTANCES 4

typedef struct {
    uint32_t       base_address;
    TIM_Registers  regs;
    uint32_t       prescaler_counter;  /* current prescaler value */
    bool           has_updated;        /* did an update event occur this tick? */
} TIM_Instance_Regs;

static TIM_Instance_Regs tim_instances[MAX_TIM_INSTANCES];
static uint32_t          instance_count = 0;

static int find_or_add(uint32_t base) {
    for (int i = 0; i < instance_count; i++) {
        if (tim_instances[i].base_address == base) return i;
    }
    if (instance_count >= MAX_TIM_INSTANCES) return -1;
    memset(&tim_instances[instance_count].regs, 0, sizeof(TIM_Registers));
    tim_instances[instance_count].base_address = base;
    return instance_count++;
}

void tim_regs_init(uint32_t base_address) {
    int idx = find_or_add(base_address);
    if (idx < 0) return;
    memset(&tim_instances[idx].regs, 0, sizeof(TIM_Registers));
    tim_instances[idx].prescaler_counter = 0;
    tim_instances[idx].has_updated = false;
}

TIM_Registers* tim_regs_get(uint32_t base_address) {
    int idx = find_or_add(base_address);
    if (idx < 0) return NULL;
    return &tim_instances[idx].regs;
}

/* Called once per microsecond (or whatever the scheduler step is) */
void tim_regs_tick(uint32_t base_address) {
    int idx = find_or_add(base_address);
    if (idx < 0) return;

    TIM_Registers *regs = &tim_instances[idx].regs;
    if (!(regs->CR1 & TIM_CR1_CEN)) return;   /* counter disabled */

    /* Prescaler logic */
    uint32_t psc = regs->PSC;
    if (psc == 0) psc = 1;  /* avoid infinite loop */
    tim_instances[idx].prescaler_counter++;
    if (tim_instances[idx].prescaler_counter >= psc) {
        tim_instances[idx].prescaler_counter = 0;

        /* Increment counter */
        uint32_t arr = regs->ARR;
        regs->CNT++;

        /* Capture / compare channels */
        for (int ch = 0; ch < 4; ch++) {
            volatile uint32_t *ccr = (volatile uint32_t*)((uintptr_t)&regs->CCR1 + ch * sizeof(uint32_t));
            uint32_t ccr_val = *ccr;
            if (ccr_val > 0 && regs->CNT == ccr_val) {
                uint32_t flag = TIM_SR_CC1IF << ch;
                regs->SR |= flag;
                /* raise interrupt if enabled */
                if (regs->DIER & (TIM_DIER_CC1IE << ch)) {
                    /* schedule an event for this channel */
                    // We'll just set the flag; the scheduler will pick it up via the update event
                    // For now we only handle update interrupts fully.
                }
            }
        }

        /* Update (overflow) */
        if (regs->CNT >= arr) {
            uint32_t old_sr = regs->SR;
            regs->CNT = 0;
            regs->SR |= TIM_SR_UIF;
            tim_instances[idx].has_updated = true;
            trace_reg_change("TIM", base_address, "SR", old_sr, regs->SR);
        }
    }
}

/* Check and raise update IRQ if flags are set */
void tim_regs_check_irq(uint32_t base_address) {
    int idx = find_or_add(base_address);
    if (idx < 0) return;
    TIM_Registers *regs = &tim_instances[idx].regs;

    if ((regs->SR & TIM_SR_UIF) && (regs->DIER & TIM_DIER_UIE)) {
        /* Only schedule an event – runtime will trace and set NVIC */
        ember_runtime_schedule(0, EVENT_TIM_UPDATE, base_address, 0);
    }
}