#include <stdio.h>
#include <string.h>
#include "mock_hal.h"
#include "trace_log.h"

/* mock control prototypes */
extern void mock_uart_init(void);
extern void mock_uart_set_rx(uintptr_t base, const uint8_t *bytes, uint16_t len);
extern const uint8_t *mock_uart_get_tx(uintptr_t base, uint16_t *len);
extern void mock_uart_inject_error(uintptr_t base, const char *error);
extern void ember_sim_uart_tick(void);

/* Overridden callbacks */
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
    const char *err = "UNKNOWN";
    if (huart->ErrorCode & HAL_UART_ERROR_PE) err = "PE";
    char payload[256];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"error\":\"%s\"",
             (uint32_t)huart->Instance, err);
    trace_log("HAL_UART_ErrorCallback", payload);
}

int main(void) {
    int failures = 0;
    trace_log_init("trace_uart_test.jsonl");
    mock_uart_init();

    UART_HandleTypeDef huart1 = {0};
    huart1.Instance = 0x40011000;
    huart1.gState   = HAL_UART_STATE_READY;
    huart1.RxState  = HAL_UART_STATE_READY;

    // Test 1: Blocking TX
    {
        uint8_t tx_data[] = "Hello";
        HAL_StatusTypeDef st = HAL_UART_Transmit(&huart1, tx_data, 5, 100);
        uint16_t tx_len;
        const uint8_t *tx_buf = mock_uart_get_tx(0x40011000, &tx_len);
        if (st != HAL_OK || tx_len != 5 || memcmp(tx_buf, "Hello", 5) != 0) {
            printf("FAIL: Test 1 - Blocking TX\n");
            failures++;
        } else {
            printf("PASS: Blocking TX captured 'Hello' (hex: 48656C6C6F)\n");
        }
    }

    // Test 2: Blocking RX with pre-loaded data
    {
        uint8_t rx_preset[] = {0x48, 0x49}; // "HI"
        mock_uart_set_rx(0x40011000, rx_preset, 2);
        uint8_t rx_buf[8] = {0};
        HAL_StatusTypeDef st = HAL_UART_Receive(&huart1, rx_buf, 2, 100);
        if (st != HAL_OK || memcmp(rx_buf, rx_preset, 2) != 0) {
            printf("FAIL: Test 2 - Blocking RX\n");
            failures++;
        } else {
            printf("PASS: Blocking RX received pre-loaded 'HI' (hex: 4849)\n");
        }
    }

    // Test 3: RX timeout (not enough bytes)
    {
        uint8_t rx_preset[] = {0x41, 0x42};
        mock_uart_set_rx(0x40011000, rx_preset, 2);
        uint8_t rx_buf[8] = {0};
        HAL_StatusTypeDef st = HAL_UART_Receive(&huart1, rx_buf, 5, 100);
        if (st != HAL_TIMEOUT || rx_buf[0] != 0x41 || rx_buf[1] != 0x42) {
            printf("FAIL: Test 3 - RX timeout\n");
            failures++;
        } else {
            printf("PASS: RX timeout occurred with partial data (2 bytes)\n");
        }
    }

    // Test 4: IT TX (async callback)
    {
        tx_cplt_called = 0;
        uint8_t tx_data[] = "X";
        HAL_StatusTypeDef st = HAL_UART_Transmit_IT(&huart1, tx_data, 1);
        if (st != HAL_OK) {
            printf("FAIL: Test 4a - IT TX start\n");
            failures++;
            goto skip4;
        }
        if (tx_cplt_called) {
            printf("FAIL: Test 4b - callback called too early\n");
            failures++;
            goto skip4;
        }
        ember_sim_uart_tick();
        ember_sim_uart_tick();
        if (!tx_cplt_called) {
            printf("FAIL: Test 4c - IT TX callback not invoked\n");
            failures++;
        } else {
            printf("PASS: IT TX callback invoked after tick\n");
        }
    }
    skip4: ;

    // Test 5: IT RX with callback
    {
        rx_cplt_called = 0;
        uint8_t rx_data[] = {0x4A, 0x4B, 0x4C};
        mock_uart_set_rx(0x40011000, rx_data, 3);
        uint8_t rx_buf[8] = {0};
        HAL_StatusTypeDef st = HAL_UART_Receive_IT(&huart1, rx_buf, 3);
        if (st != HAL_OK || rx_cplt_called) {
            printf("FAIL: Test 5a - IT RX start\n");
            failures++;
            goto skip5;
        }
        ember_sim_uart_tick();
        ember_sim_uart_tick();
        if (!rx_cplt_called || memcmp(rx_buf, rx_data, 3) != 0) {
            printf("FAIL: Test 5b - IT RX callback or data mismatch\n");
            failures++;
        } else {
            printf("PASS: IT RX callback invoked with correct data\n");
        }
    }
    skip5: ;

    // Test 6: DMA TX (callback after tick)
    {
        tx_cplt_called = 0;
        uint8_t tx_data[] = "DMA";
        HAL_StatusTypeDef st = HAL_UART_Transmit_DMA(&huart1, tx_data, 3);
        if (st != HAL_OK) {
            printf("FAIL: Test 6a - DMA TX start\n");
            failures++;
            goto skip6;
        }
        // DMA schedules completion at current tick — must call tick to fire callback
        ember_sim_uart_tick();
        if (!tx_cplt_called) {
            printf("FAIL: Test 6b - DMA TX callback not invoked after tick\n");
            failures++;
        } else {
            printf("PASS: DMA TX callback invoked after tick\n");
        }
    }
    skip6: ;

    // Test 7: Error injection (PE error during blocking TX)
    {
        error_cb_called = 0;
        mock_uart_inject_error(0x40011000, "PE");
        uint8_t tx_data[] = "E";
        HAL_StatusTypeDef st = HAL_UART_Transmit(&huart1, tx_data, 1, 100);
        if (st != HAL_ERROR || !error_cb_called) {
            printf("FAIL: Test 7 - Error injection\n");
            failures++;
        } else {
            printf("PASS: Error injection (PE) triggered callback\n");
        }
    }

    trace_log_close();

    FILE *f = fopen("trace_uart_test.jsonl", "r");
    if (f) {
        char line[512];
        printf("\n--- Enhanced Trace Output ---\n");
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