/* host_main.c — Minimal host runner for EmberSim */
#include "mock_hal.h"
#include "trace_log.h"
#include <stdio.h>

extern void app_init(void);
extern void app_run(void);
extern void ember_sim_uart_tick(void);

int main(int argc, char **argv)
{
    const char *trace_path = (argc > 1) ? argv[1] : "trace.jsonl";
    trace_log_init(trace_path);

    app_init();

    for (int i = 0; i < 1000; i++) {
        ember_sim_uart_tick();
        app_run();
    }

    trace_log_close();
    return 0;
}