#include "mock_hal.h"
#include "trace_log.h"
#include "ember_sim_kernel.h"

extern void app_init(void);
extern void app_run(void);

int main(int argc, char **argv) {
    const char *trace_path = (argc > 1) ? argv[1] : "trace.jsonl";
    trace_log_init(trace_path);
    kernel_init();

    app_init();

    // Run for 1 second, stepping the kernel in 100µs chunks
    uint64_t deadline = 1000000;
    uint64_t step = 100;
    for (uint64_t t = 0; t < deadline; t += step) {
        kernel_run_until(t + step);
        app_run();
    }

    trace_log_close();
    return 0;
}