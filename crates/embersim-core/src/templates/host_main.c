/* host_main.c — EmberSim runtime host runner */
#include "mock_hal.h"
#include "trace_log.h"
#include "ember_sim_runtime.h"

extern void app_init(void);
extern void app_run(void);

int main(int argc, char **argv) {
    const char *trace_path = (argc > 1) ? argv[1] : "trace.jsonl";
    trace_log_init(trace_path);

    ember_runtime_init(0);

    app_init();

    /* Run simulation for 1 second, stepping in 100µs chunks */
    uint32_t deadline = 1000000;
    uint32_t step = 100;
    for (uint32_t t = 0; t < deadline; t += step) {
        ember_runtime_run_until(t + step);
        app_run();
    }

    trace_log_close();
    return 0;
}