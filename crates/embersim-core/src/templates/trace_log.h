#ifndef TRACE_LOG_H
#define TRACE_LOG_H

#include <stdint.h>

void trace_log_init(const char *filename);
void trace_log_close(void);

/* json_payload: valid JSON key-value fragment, e.g. "\"inst\":\"0x40011000\",\"sz\":5" */
void trace_log(const char *func, const char *json_payload);

void trace_reg_change(const char *peripheral, uint32_t addr, const char *reg, uint32_t old_val, uint32_t new_val, const char *reason);

#endif