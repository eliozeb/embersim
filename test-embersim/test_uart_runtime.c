#include <stdio.h>
#include <string.h>
#include "mock_hal.h"
#include "mock_uart.h"
#include "ember_sim_kernel.h"
#include "trace_log.h"

extern void mock_uart_init(void);
extern void mock_uart_set_rx(uintptr_t base, const uint8_t *bytes, uint16_t len);
extern const uint8_t *mock_uart_get_tx(uintptr_t base, uint16_t *len);

static int tx_cplt = 0, rx_cplt = 0;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    tx_cplt++;
    trace_software_event("UART", "tx_complete", "{}");
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    rx_cplt++;
    trace_software_event("UART", "rx_complete", "{}");
}

int main(void) {
    int failures = 0;
    trace_log_init("trace_uart.jsonl");
    mock_uart_init();
    kernel_init();
    nvic_enable(38);

    UART_HandleTypeDef huart = { .Instance = 0x40004400, .ErrorCode = 0 };
    HAL_UART_Init(&huart);

    /* Test 1: Blocking TX */
    {
        uint8_t data[] = "Hello";
        HAL_UART_Transmit(&huart, data, 5, 100);
        kernel_run_until(10);
        uint16_t len;
        const uint8_t *tx = mock_uart_get_tx(0x40004400, &len);
        if (len == 5 && memcmp(tx, "Hello", 5) == 0)
            printf("PASS: Blocking TX\n");
        else { printf("FAIL: Blocking TX\n"); failures++; }
    }

    /* Test 2: Blocking RX */
    {
        uint8_t rx_data[] = {0x48, 0x49}; // "HI"
        mock_uart_set_rx(0x40004400, rx_data, 2);
        uint8_t buf[8] = {0};
        HAL_StatusTypeDef st = HAL_UART_Receive(&huart, buf, 2, 100);
        kernel_run_until(10);
        if (st == HAL_OK && buf[0] == 0x48 && buf[1] == 0x49)
            printf("PASS: Blocking RX\n");
        else { printf("FAIL: Blocking RX\n"); failures++; }
    }

    /* Test 3: IT TX with callback */
    {
        tx_cplt = 0;
        uint8_t data[] = "X";
        HAL_UART_Transmit_IT(&huart, data, 1);
        kernel_run_until(5);
        if (tx_cplt == 1)
            printf("PASS: IT TX callback\n");
        else { printf("FAIL: IT TX callback (tx_cplt=%d)\n", tx_cplt); failures++; }
    }

    /* Test 4: IT RX with callback */
    {
        rx_cplt = 0;
        uint8_t rx_data[] = {0x4A};
        mock_uart_set_rx(0x40004400, rx_data, 1);
        HAL_UART_Receive_IT(&huart, NULL, 1); // Size param ignored in this simplified mock
        kernel_run_until(5);
        if (rx_cplt == 1)
            printf("PASS: IT RX callback\n");
        else { printf("FAIL: IT RX callback (rx_cplt=%d)\n", rx_cplt); failures++; }
    }

    trace_log_close();
    FILE *f = fopen("trace_uart.jsonl", "r");
    if (f) { char line[256]; printf("\n--- Trace ---\n"); while (fgets(line,sizeof(line),f)) printf("%s",line); fclose(f); }

    if (!failures) printf("\nALL UART TESTS PASSED\n");
    else printf("\n%d TEST(S) FAILED\n", failures);
    return failures;
}