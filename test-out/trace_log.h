/* trace_log.h */
#ifndef TRACE_LOG_H
#define TRACE_LOG_H

#include <stdint.h>

void trace_log_init(const char *filename);
void trace_log_close(void);
void trace_log(const char *func, const char *fmt, ...);

#endif