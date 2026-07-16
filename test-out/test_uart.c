#include "mock_hal.h"
#include "trace_log.h"
#include <stdio.h>
#include <string.h>

/* Access to internal tick for test */
extern void ember_sim_uart_tick(void);
extern void mock_uart_init(void);
extern void mock_uart_set_rx(uintptr_t base, const uint8_t *bytes, uint16_t len);
extern const uint8_t *mock_uart_get_tx(uintptr_t base, uint16_t *len);
extern void mock_uart_inject_error(uintptr_t base, uint32_t flags, uint16_t after_bytes);

/* Override callbacks for testing */
static int tx_cplt_called = 0;
static int rx_cplt_called = 0;
static int error_cb_called = 0;

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    tx_cplt_called = 1;
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)huart->Instance);
    trace_log("HAL_UART_TxCpltCallback", payload);
}
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    rx_cplt_called = 1;
    char payload[128];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)huart->Instance);
    trace_log("HAL_UART_RxCpltCallback", payload);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    error_cb_called = 1;
    char payload[256];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"error\":\"0x%X\"",
             (uint32_t)huart->Instance, huart->ErrorCode);
    trace_log("HAL_UART_ErrorCallback", payload);
}

int main(void) {
    int failures = 0;
    trace_log_init("trace_uart_pro.jsonl");
    mock_uart_init();

    UART_HandleTypeDef huart1 = {
        .Instance = (uint32_t)0x40011000,
        .gState = HAL_UART_STATE_READY,
        .RxState = HAL_UART_STATE_READY
    };

    /* Test 1: Blocking TX */
    {
        uint8_t tx_data[] = "Hello";
        HAL_StatusTypeDef st = HAL_UART_Transmit(&huart1, tx_data, 5, 100);
        uint16_t tx_len;
        const uint8_t *tx = mock_uart_get_tx(0x40011000, &tx_len);
        if (st != HAL_OK || tx_len != 5 || memcmp(tx, "Hello", 5) != 0) { failures++; printf("FAIL 1\n"); }
        else printf("PASS: Blocking TX\n");
    }

    /* Test 2: Blocking RX */
    {
        uint8_t rx[] = {0x48, 0x49};
        mock_uart_set_rx(0x40011000, rx, 2);
        uint8_t buf[8] = {0};
        HAL_StatusTypeDef st = HAL_UART_Receive(&huart1, buf, 2, 100);
        if (st != HAL_OK || memcmp(buf, rx, 2) != 0) { failures++; printf("FAIL 2\n"); }
        else printf("PASS: Blocking RX\n");
    }

    /* Test 3: Timeout */
    {
        uint8_t rx[] = {0x41};
        mock_uart_set_rx(0x40011000, rx, 1);
        uint8_t buf[8] = {0};
        HAL_StatusTypeDef st = HAL_UART_Receive(&huart1, buf, 5, 100);
        if (st != HAL_TIMEOUT || buf[0] != 0x41) { failures++; printf("FAIL 3\n"); }
        else printf("PASS: Timeout with partial read\n");
    }

    /* Test 4: IT TX (async) */
    {
        tx_cplt_called = 0;
        uint8_t tx_data[] = "X";
        HAL_StatusTypeDef st = HAL_UART_Transmit_IT(&huart1, tx_data, 1);
        if (st != HAL_OK) { failures++; printf("FAIL 4a\n"); goto skip4; }
        /* Callback must NOT be called yet */
        if (tx_cplt_called) { failures++; printf("FAIL 4b: callback called too early\n"); goto skip4; }
        /* Advance ticks by 2 ms */
        ember_sim_uart_tick(); ember_sim_uart_tick();
        /* Now callback should have been invoked */
        if (!tx_cplt_called) { failures++; printf("FAIL 4c: callback not invoked\n"); }
        else printf("PASS: IT TX callback after tick\n");
    }
    skip4: ;

    /* Test 5: Error injection during transfer */
    {
        error_cb_called = 0;
        mock_uart_inject_error(0x40011000, HAL_UART_ERROR_PE, 0);
        uint8_t tx[] = "E";
        HAL_StatusTypeDef st = HAL_UART_Transmit(&huart1, tx, 1, 100);
        if (st != HAL_ERROR || !error_cb_called || huart1.ErrorCode != HAL_UART_ERROR_PE) {
            failures++; printf("FAIL 5\n");
        } else printf("PASS: Error injection with HAL_ERROR and ErrorCode\n");
    }

    /* Test 6: Busy state check */
    {
        huart1.gState = HAL_UART_STATE_BUSY_TX;
        uint8_t tx[] = "Z";
        HAL_StatusTypeDef st = HAL_UART_Transmit(&huart1, tx, 1, 100);
        if (st != HAL_BUSY) { failures++; printf("FAIL 6: didn't return HAL_BUSY\n"); }
        else printf("PASS: HAL_BUSY when gState not READY\n");
        huart1.gState = HAL_UART_STATE_READY;
    }

    trace_log_close();

    FILE *f = fopen("trace_uart_pro.jsonl", "r");
    if (f) {
        char line[512];
        printf("\n--- Trace ---\n");
        while (fgets(line, sizeof(line), f)) printf("%s", line);
        fclose(f);
    }

    if (failures == 0) printf("\nALL PROFESSIONAL UART TESTS PASSED\n");
    else printf("\n%d TEST(S) FAILED\n", failures);
    return failures;
}