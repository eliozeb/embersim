// tim_peripheral.c (replaces mock_tim.c)
#include "ember_sim_kernel.h"
#include "ember_regs.h"

static EmberRegister tim_regs[] = {
    {"CR1",  0, 0, 0x0001, 0xFFFE, "Control 1", on_cr1_write},
    {"SR",   0, 0, 0x001F, 0xFFE0, "Status",    NULL},
    {"CNT",  0, 0, 0xFFFF, 0x0000, "Counter",   NULL},
    {"PSC",  0, 0, 0xFFFF, 0x0000, "Prescaler", NULL},
    {"ARR",  0, 0, 0xFFFF, 0x0000, "Auto-reload", NULL},
    // ... CCR1-4 ...
};

static EmberRegMap tim2_map;
static EmberPeripheral tim2_peripheral;

static void tim_init(EmberPeripheral *p) {
    ember_regs_init_map(&tim2_map, "TIM2", p->base_address, tim_regs, 5);
}

static void tim_tick(EmberPeripheral *p, uint64_t now_us) {
    uint32_t cr1 = ember_reg_read(&tim2_map, 0);
    if (!(cr1 & 1)) return;  // CEN

    uint32_t psc = ember_reg_read(&tim2_map, 3);
    uint32_t arr = ember_reg_read(&tim2_map, 4);
    uint32_t cnt = ember_reg_read(&tim2_map, 2);

    uint32_t dier = ember_reg_read(&tim_map, IDX_DIER);
    if (dier & TIM_DIER_UIE) {
        ember_bus_publish(BUS_EVT_TIMER_UPDATE, p->base_address, 0, NULL);
    }

    cnt++;
    if (cnt > arr) {
        cnt = 0;
        ember_reg_write(&tim2_map, 2, cnt, "overflow", 0);
        ember_reg_set_bits(&tim2_map, 1, 0x0001, "overflow", 0); // SR.UIF
        // schedule timer update event
        kernel_schedule_event(0, KERN_EVT_TIM_UPDATE, p->base_address, 0, 0);
    } else {
        ember_reg_write(&tim2_map, 2, cnt, "increment", 0);
    }
}

static void tim_handle_event(EmberPeripheral *p, const KernelEvent *ev) {
    if (ev->type == KERN_EVT_TIM_UPDATE) {
        // fire NVIC
        nvic_set_pending(p->irq_number);
    }
}

// register the peripheral at startup
void tim2_register(void) {
    tim2_peripheral.name = "TIM2";
    tim2_peripheral.base_address = 0x40000400;
    tim2_peripheral.irq_number = 28;
    tim2_peripheral.state = NULL;
    tim2_peripheral.init = tim_init;
    tim2_peripheral.tick = tim_tick;
    tim2_peripheral.handle_event = tim_handle_event;
    kernel_register_peripheral(&tim2_peripheral);
}