/* trace_log.c — Append-only JSON Lines trace logger */
#include "trace_log.h"
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

static FILE *trace_file = NULL;
static uint32_t event_counter = 0;

void trace_log_init(const char *filename)
{
    trace_file = fopen(filename, "w");
    if (trace_file) {
        /* JSON Lines has no header, but we can write a comment-like marker */
        fprintf(trace_file, "# EmberSim trace v1.0\n");
    }
}

void trace_log_close(void)
{
    if (trace_file) {
        fclose(trace_file);
        trace_file = NULL;
    }
}

void trace_log(const char *func, const char *fmt, ...)
{
    if (!trace_file) return;

    /* Derive peripheral from function name prefix */
    const char *peripheral = "UNKNOWN";
    if (strncmp(func, "HAL_GPIO", 8) == 0) peripheral = "GPIO";
    else if (strncmp(func, "HAL_UART", 8) == 0) peripheral = "UART";
    else if (strncmp(func, "HAL_I2C", 7) == 0) peripheral = "I2C";
    else if (strncmp(func, "HAL_SPI", 7) == 0) peripheral = "SPI";
    else if (strncmp(func, "HAL_TIM", 7) == 0) peripheral = "TIM";
    else if (strncmp(func, "HAL_DMA", 7) == 0) peripheral = "DMA";
    else if (strncmp(func, "HAL_ADC", 7) == 0) peripheral = "ADC";

    /* Build args string from fmt + va_list (simplified: just log the fmt string as args) */
    char args_buf[256];
    va_list args;
    va_start(args, fmt);
    vsnprintf(args_buf, sizeof(args_buf), fmt, args);
    va_end(args);

    fprintf(trace_file,
        "{\"ts_ms\":%u,\"peripheral\":\"%s\",\"func\":\"%s\",\"args\":\"%s\"}\n",
        event_counter++, peripheral, func, args_buf);

    fflush(trace_file);
}