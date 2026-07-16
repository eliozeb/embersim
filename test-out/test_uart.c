#include "mock_hal.h"
#include "trace_log.h"
#include <stdio.h>
#include <string.h>

extern void mock_uart_init(void);
extern void mock_uart_set_rx(uintptr_t base, const uint8_t *bytes, uint16_t len);
extern const uint8_t *mock_uart_get_tx(uintptr_t base, uint16_t *len);

int main(void) {
    int failures = 0;
    trace_log_init("trace_uart.jsonl");
    mock_uart_init();

    /* Test 1: Transmit captures bytes */
    UART_HandleTypeDef huart1 = { .Instance = (uint32_t)0x20000000 };
    uint8_t tx_data[] = "Hello";
    HAL_UART_Transmit(&huart1, tx_data, 5, 100);
    
    uint16_t tx_len;
    const uint8_t *tx_buf = mock_uart_get_tx(0x20000000, &tx_len);
    if (tx_len != 5 || memcmp(tx_buf, "Hello", 5) != 0) {
        printf("FAIL: Test 1 — TX buffer mismatch\n");
        failures++;
    } else {
        printf("PASS: Test 1 — TX captured 'Hello'\n");
    }

    /* Test 2: Receive pre-loaded bytes */
    uint8_t rx_preset[] = {0x48, 0x49, 0x0A}; /* "HI\n" */
    mock_uart_set_rx(0x20000000, rx_preset, 3);
    
    uint8_t rx_buf[8] = {0};
    HAL_UART_Receive(&huart1, rx_buf, 3, 100);
    
    if (memcmp(rx_buf, rx_preset, 3) != 0) {
        printf("FAIL: Test 2 — RX bytes mismatch\n");
        failures++;
    } else {
        printf("PASS: Test 2 — RX received pre-loaded bytes\n");
    }

    trace_log_close();

    FILE *f = fopen("trace_uart.jsonl", "r");
    if (f) {
        char line[512];
        printf("\n--- Trace Output ---\n");
        while (fgets(line, sizeof(line), f)) printf("%s", line);
        fclose(f);
    }

    if (failures == 0) {
        printf("\nALL UART TESTS PASSED\n");
        return 0;
    } else {
        printf("\n%d TEST(S) FAILED\n", failures);
        return 1;
    }
}