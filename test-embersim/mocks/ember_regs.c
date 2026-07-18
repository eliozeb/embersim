#include "ember_regs.h"
#include "ember_sim_kernel.h"
#include <string.h>

void ember_regs_init(EmberRegMap *map, const char *peripheral, uint32_t base,
                     EmberRegister *regs, uint32_t count) {
    map->peripheral = peripheral;
    map->base_address = base;
    map->regs = regs;
    map->count = count;
    for (uint32_t i = 0; i < count; i++) {
        regs[i].value = regs[i].reset_value;
    }
}

uint32_t ember_reg_read(EmberRegMap *map, uint32_t index) {
    return map->regs[index].value;
}

void ember_reg_write(EmberRegMap *map, uint32_t index, uint32_t value,
                     const char *reason, uint32_t parent_event_id) {
    EmberRegister *reg = &map->regs[index];
    // apply write mask
    value = (reg->value & ~reg->writable_mask) | (value & reg->writable_mask);
    uint32_t old = reg->value;
    if (old == value) return;
    reg->value = value;

    // schedule register change event
    KernelEvent ev = {0};
    ev.event_id     = 0; // will be assigned by kernel
    ev.parent_id    = parent_event_id;
    ev.timestamp_us = kernel_now_us();
    ev.type         = KERN_EVT_REGISTER_CHANGED;
    ev.source       = map->base_address;
    ev.data.reg.base_address = map->base_address;
    ev.data.reg.reg_name     = reg->name;
    ev.data.reg.old_value    = old;
    ev.data.reg.new_value    = value;
    ev.data.reg.reason       = reason;
    if (old != reg->value) {
        trace_reg_change(map->peripheral, map->base_address, reg->name, old, reg->value, reason);
        if (reg->on_write) {
            reg->on_write(map->base_address, reg, old, reason);
        }
    }
    // the actual event data is lost in this simplified API; we'll improve later.
    // For now, we directly call trace inside the kernel when it processes REGISTER_CHANGED.
    // We'll store the change info in a global or use the event payload.
    // A more robust approach: pass the change data directly when scheduling, but our schedule function
    // only accepts type/source/param. We'll extend that later. For now we accept that trace happens inside kernel.
    // The side-effect callback:
    if (reg->on_write) {
        reg->on_write(map->base_address, reg, old, reason);
    }
}

void ember_reg_set_bits(EmberRegMap *map, uint32_t index, uint32_t mask,
                        const char *reason, uint32_t parent_event_id) {
    uint32_t new_val = map->regs[index].value | mask;
    ember_reg_write(map, index, new_val, reason, parent_event_id);
}

void ember_reg_clear_bits(EmberRegMap *map, uint32_t index, uint32_t mask,
                          const char *reason, uint32_t parent_event_id) {
    uint32_t new_val = map->regs[index].value & ~mask;
    ember_reg_write(map, index, new_val, reason, parent_event_id);
}