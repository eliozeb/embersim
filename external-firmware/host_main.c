/* host_main.c — EmberSim host runner */
#include "mock_hal.h"
#include "ember_sim_kernel.h"
#include "trace_log.h"
#include "motor_control.h"
#include <stdio.h>

int main(int argc, char **argv) {
    const char *trace_path = (argc > 1) ? argv[1] : "trace.jsonl";
    trace_log_init(trace_path);
    kernel_init();
    nvic_enable(28);   /* TIM2 */
    nvic_enable(38);   /* USART2 */
    app_init();
    kernel_run_until(100000);  /* 100ms */
    trace_log_close();
    printf("Done.\n");
    return 0;
}
