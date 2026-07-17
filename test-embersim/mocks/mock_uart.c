/* mock_uart.c — Professional UART mock with tick-driven async completion */
#include "mock_hal.h"
#include "trace_log.h"
#include <string.h>
#include <stdio.h>

/* Forward declarations for HAL callbacks (weak defaults defined later) */
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart);
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart);

/* -----------------------------------------------
   Internal state storage
   ----------------------------------------------- */
#define UART_MAX_INSTANCES  4
#define UART_BUF_SIZE       256

typedef struct {
    uint8_t tx_buf[UART_BUF_SIZE];
    uint16_t tx_len;
    uint8_t rx_buf[UART_BUF_SIZE];
    uint16_t rx_len, rx_pos;

    struct {
        int enabled;
        uint32_t flags;         // HAL_UART_ERROR_xxx
        uint16_t trigger_after;
    } error_inject;

    int tx_mode;  /* 0=none, 1=blocking, 2=IT, 3=DMA */
    int rx_mode;
    uint32_t tx_end_tick;
    uint32_t rx_end_tick;
    int tx_cplt_pending;
    int rx_cplt_pending;

    UART_HandleTypeDef *tx_huart;  // handle for TX callback
    UART_HandleTypeDef *rx_huart;  // handle for RX callback
} UartInstance;

static UartInstance uart_instances[UART_MAX_INSTANCES];
static uint32_t s_current_tick = 0;

static int uart_index(uintptr_t base) {
    switch (base) {
        case 0x40011000: return 0;
        case 0x40004400: return 1;
        case 0x40004800: return 2;
        case 0x40004C00: return 3;
        default: return 0;
    }
}

/* ---------- Public control API ---------- */
void mock_uart_init(void) {
    memset(uart_instances, 0, sizeof(uart_instances));
    s_current_tick = 0;
}
void mock_uart_set_rx(uintptr_t base, const uint8_t *bytes, uint16_t len) {
    int idx = uart_index(base);
    if (len > UART_BUF_SIZE) len = UART_BUF_SIZE;
    memcpy(uart_instances[idx].rx_buf, bytes, len);
    uart_instances[idx].rx_len = len;
    uart_instances[idx].rx_pos = 0;
}
const uint8_t *mock_uart_get_tx(uintptr_t base, uint16_t *len) {
    int idx = uart_index(base);
    *len = uart_instances[idx].tx_len;
    return uart_instances[idx].tx_buf;
}
void mock_uart_inject_error(uintptr_t base, const char *error) {
    int idx = uart_index(base);
    uart_instances[idx].error_inject.enabled = 1;
    if (strcmp(error, "PE") == 0) uart_instances[idx].error_inject.flags = 0x01;
    else if (strcmp(error, "FE") == 0) uart_instances[idx].error_inject.flags = 0x02;
    else if (strcmp(error, "NE") == 0) uart_instances[idx].error_inject.flags = 0x04;
    else if (strcmp(error, "ORE") == 0) uart_instances[idx].error_inject.flags = 0x08;
}

/* ---------- Tick function (must be called from host) ---------- */
void ember_sim_uart_tick(void) {
    s_current_tick++;
    for (int i = 0; i < UART_MAX_INSTANCES; i++) {
        UartInstance *u = &uart_instances[i];
        if (u->tx_cplt_pending && s_current_tick >= u->tx_end_tick) {
            u->tx_cplt_pending = 0;
            u->tx_mode = 0;
            if (u->tx_huart) {
                HAL_UART_TxCpltCallback(u->tx_huart);
            }
        }
        if (u->rx_cplt_pending && s_current_tick >= u->rx_end_tick) {
            u->rx_cplt_pending = 0;
            u->rx_mode = 0;
            if (u->rx_huart) {
                HAL_UART_RxCpltCallback(u->rx_huart);
            }
        }
    }
}

/* ---------- HAL UART Implementations ---------- */

HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    (void)Timeout;
    int idx = uart_index((uintptr_t)huart->Instance);
    UartInstance *u = &uart_instances[idx];
    if (Size > UART_BUF_SIZE) Size = UART_BUF_SIZE;
    memcpy(u->tx_buf, pData, Size);
    u->tx_len = Size;
    u->tx_mode = 1; // BLOCKING
    char hex[UART_BUF_SIZE*2+1];
    for (int i=0; i<Size; i++) sprintf(hex+2*i, "%02X", pData[i]);
    hex[2*Size] = '\0';
    char payload[512];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"mode\":\"BLOCK\"",
             (uint32_t)huart->Instance, hex, Size);
    trace_log("HAL_UART_Transmit", payload);
    if (u->error_inject.enabled) {
        huart->ErrorCode = u->error_inject.flags;
        u->error_inject.enabled = 0;
        HAL_UART_ErrorCallback(huart);
        return HAL_ERROR;
    }
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
    (void)Timeout;
    int idx = uart_index((uintptr_t)huart->Instance);
    UartInstance *u = &uart_instances[idx];
    uint16_t available = u->rx_len - u->rx_pos;
    char hex[UART_BUF_SIZE*2+1];
    if (Size > available) {
        memcpy(pData, u->rx_buf + u->rx_pos, available);
        u->rx_pos += available;
        for (int i=0; i<available; i++) sprintf(hex+2*i, "%02X", pData[i]);
        hex[2*available] = '\0';
        char payload[512];
        snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"requested\":%u,\"status\":\"TIMEOUT\"",
                 (uint32_t)huart->Instance, hex, available, Size);
        trace_log("HAL_UART_Receive", payload);
        return HAL_TIMEOUT;
    }
    memcpy(pData, u->rx_buf + u->rx_pos, Size);
    u->rx_pos += Size;
    for (int i=0; i<Size; i++) sprintf(hex+2*i, "%02X", pData[i]);
    hex[2*Size] = '\0';
    char payload[512];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u",
             (uint32_t)huart->Instance, hex, Size);
    trace_log("HAL_UART_Receive", payload);
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size) {
    int idx = uart_index((uintptr_t)huart->Instance);
    UartInstance *u = &uart_instances[idx];
    if (Size > UART_BUF_SIZE) Size = UART_BUF_SIZE;
    memcpy(u->tx_buf, pData, Size);
    u->tx_len = Size;
    u->tx_mode = 2; // IT
    u->tx_huart = huart;  // store handle for later callback
    char hex[UART_BUF_SIZE*2+1];
    for (int i=0; i<Size; i++) sprintf(hex+2*i, "%02X", pData[i]);
    hex[2*Size] = '\0';
    char payload[512];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"mode\":\"IT\"",
             (uint32_t)huart->Instance, hex, Size);
    trace_log("HAL_UART_Transmit_IT", payload);
    u->tx_cplt_pending = 1;
    u->tx_end_tick = s_current_tick + 1;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size) {
    int idx = uart_index((uintptr_t)huart->Instance);
    UartInstance *u = &uart_instances[idx];
    uint16_t available = u->rx_len - u->rx_pos;
    if (Size > available) Size = available;
    memcpy(pData, u->rx_buf + u->rx_pos, Size);
    u->rx_pos += Size;
    u->rx_mode = 2; // IT
    u->rx_huart = huart;
    char hex[UART_BUF_SIZE*2+1];
    for (int i=0; i<Size; i++) sprintf(hex+2*i, "%02X", pData[i]);
    hex[2*Size] = '\0';
    char payload[512];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"mode\":\"IT\"",
             (uint32_t)huart->Instance, hex, Size);
    trace_log("HAL_UART_Receive_IT", payload);
    u->rx_cplt_pending = 1;
    u->rx_end_tick = s_current_tick + 1;
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size) {
    int idx = uart_index((uintptr_t)huart->Instance);
    UartInstance *u = &uart_instances[idx];
    if (Size > UART_BUF_SIZE) Size = UART_BUF_SIZE;
    memcpy(u->tx_buf, pData, Size);
    u->tx_len = Size;
    u->tx_mode = 3; // DMA
    u->tx_huart = huart;
    char hex[UART_BUF_SIZE*2+1];
    for (int i=0; i<Size; i++) sprintf(hex+2*i, "%02X", pData[i]);
    hex[2*Size] = '\0';
    char payload[512];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"mode\":\"DMA\"",
             (uint32_t)huart->Instance, hex, Size);
    trace_log("HAL_UART_Transmit_DMA", payload);
    u->tx_cplt_pending = 1;
    u->tx_end_tick = s_current_tick; // immediate
    return HAL_OK;
}

HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size) {
    int idx = uart_index((uintptr_t)huart->Instance);
    UartInstance *u = &uart_instances[idx];
    uint16_t available = u->rx_len - u->rx_pos;
    if (Size > available) Size = available;
    memcpy(pData, u->rx_buf + u->rx_pos, Size);
    u->rx_pos += Size;
    u->rx_mode = 3; // DMA
    u->rx_huart = huart;
    char hex[UART_BUF_SIZE*2+1];
    for (int i=0; i<Size; i++) sprintf(hex+2*i, "%02X", pData[i]);
    hex[2*Size] = '\0';
    char payload[512];
    snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"mode\":\"DMA\"",
             (uint32_t)huart->Instance, hex, Size);
    trace_log("HAL_UART_Receive_DMA", payload);
    u->rx_cplt_pending = 1;
    u->rx_end_tick = s_current_tick;
    return HAL_OK;
}

/* Default weak callbacks (overridden by user) */
__attribute__((weak)) void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {}
__attribute__((weak)) void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {}
__attribute__((weak)) void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {}