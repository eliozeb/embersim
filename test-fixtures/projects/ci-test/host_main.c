/* host_main.c — EmberSim host runner for real-firmware project */
#include "mock_hal.h"
#include "ember_sim_kernel.h"
#include "trace_log.h"
#include <stdio.h>

extern void app_init(void);
extern void app_run(void);

int main(int argc, char **argv) {
    const char *trace_path = (argc > 1) ? argv[1] : "trace_firmware.jsonl";
    trace_log_init(trace_path);
    kernel_init();

    /* NVIC must be enabled AFTER kernel_init (which resets NVIC state) */
    nvic_enable(28);   /* TIM2_IRQn */
    nvic_enable(38);   /* USART2_IRQn */

    /* Run firmware */
    app_init();

    /* Simulate 50ms of runtime */
    kernel_run_until(50000);

    trace_log_close();

    /* Print trace summary */
    FILE *f = fopen(trace_path, "r");
    if (f) {
        int lines = 0;
        char buf[512];
        while (fgets(buf, sizeof(buf), f)) lines++;
        fclose(f);
        printf("Trace: %d lines in %s\n", lines, trace_path);
    }
    return 0;
}
