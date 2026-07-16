/* mock_uart.c — Stateful UART simulation */
#include "mock_hal.h"
#include "trace_log.h"
#include <string.h>

#define UART_MAX_PORTS  4
#define UART_BUF_SIZE   256

typedef struct {
    uint8_t tx_buf[UART_BUF_SIZE];
    uint16_t tx_len;
    uint8_t rx_buf[UART_BUF_SIZE];
    uint16_t rx_len;
    uint16_t rx_pos;
} MockUART_State;

static MockUART_State uart_states[UART_MAX_PORTS];

/* Dummy base addresses */
#define USART1_BASE  ((uintptr_t)0x20000000)
#define USART2_BASE  ((uintptr_t)0x20000400)
#define USART3_BASE  ((uintptr_t)0x20000800)
#define UART4_BASE   ((uintptr_t)0x20000C00)

static int uart_index(uintptr_t base) {
    switch (base) {
        case USART1_BASE: return 0;
        case USART2_BASE: return 1;
        case USART3_BASE: return 2;
        case UART4_BASE:  return 3;
        default:          return 0;
    }
}

void mock_uart_init(void) {
    memset(uart_states, 0, sizeof(uart_states));
}

/* Pre-load RX bytes from test config */
void mock_uart_set_rx(uintptr_t base, const uint8_t *bytes, uint16_t len) {
    int idx = uart_index(base);
    if (len > UART_BUF_SIZE) len = UART_BUF_SIZE;
    memcpy(uart_states[idx].rx_buf, bytes, len);
    uart_states[idx].rx_len = len;
    uart_states[idx].rx_pos = 0;
}

/* Get captured TX bytes */
const uint8_t *mock_uart_get_tx(uintptr_t base, uint16_t *len) {
    int idx = uart_index(base);
    *len = uart_states[idx].tx_len;
    return uart_states[idx].tx_buf;
}

/* ========== Stateful HAL Implementations ========== */

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    trace_log("HAL_UART_Transmit", "huart,pData,Size,Timeout");
    (void)Timeout;
    int idx = uart_index((uintptr_t)huart->Instance);
    if (Size > UART_BUF_SIZE) Size = UART_BUF_SIZE;
    memcpy(uart_states[idx].tx_buf, pData, Size);
    uart_states[idx].tx_len = Size;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    trace_log("HAL_UART_Receive", "huart,pData,Size,Timeout");
    (void)Timeout;
    int idx = uart_index((uintptr_t)huart->Instance);
    uint16_t available = uart_states[idx].rx_len - uart_states[idx].rx_pos;
    if (Size > available) Size = available;
    memcpy(pData, uart_states[idx].rx_buf + uart_states[idx].rx_pos, Size);
    uart_states[idx].rx_pos += Size;
    return HAL_OK;
}