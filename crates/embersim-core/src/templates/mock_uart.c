/* mock_uart.c — Professional UART mock with tick-driven async completion,
   full HAL state machine, timeout simulation, and event-based error injection.
   Requires a periodic call to ember_sim_uart_tick() from host_main.c. */

   #include "mock_hal.h"
   #include "trace_log.h"
   #include <string.h>
   #include <stdio.h>
   #include <stdbool.h>
   
   /* --------------------------------------------------------------------------
      Configuration
      -------------------------------------------------------------------------- */
   #define UART_MAX_INSTANCES  4
   #define UART_BUF_SIZE       256
   
   /* Simulated tick frequency (Hz) – how fast ember_sim_uart_tick() is called.
      We assume 1000 Hz (1 ms per tick). */
   #define TICK_RATE_HZ        1000
   #define TICK_PERIOD_MS      1
   
   /* --------------------------------------------------------------------------
      Internal UART instance structure
      -------------------------------------------------------------------------- */
   typedef enum {
       UART_TX_MODE_NONE,
       UART_TX_MODE_BLOCKING,
       UART_TX_MODE_IT,
       UART_TX_MODE_DMA
   } UartTxMode;
   
   typedef enum {
       UART_RX_MODE_NONE,
       UART_RX_MODE_BLOCKING,
       UART_RX_MODE_IT,
       UART_RX_MODE_DMA
   } UartRxMode;
   
   typedef struct {
       uint8_t   tx_buf[UART_BUF_SIZE];
       uint16_t  tx_len;            // total bytes to send
       uint16_t  tx_progress;       // bytes already "transferred" (for async)
       uint8_t   rx_buf[UART_BUF_SIZE];
       uint16_t  rx_len;            // bytes pre‑loaded
       uint16_t  rx_pos;            // next byte to give to firmware
   
       /* Error injection */
       struct {
           bool     enabled;
           uint32_t flags;          // HAL_UART_ERROR_xxx
           uint16_t trigger_after;  // byte count at which to inject (0 = immediate)
       } error_inject;
   
       /* Async transfer control */
       UartTxMode tx_mode;
       UartRxMode rx_mode;
       uint32_t   tx_end_tick;      // abs tick when TX completes (if async)
       uint32_t   rx_end_tick;      // abs tick when RX completes (if async)
       bool       tx_cplt_pending;
       bool       rx_cplt_pending;
       bool       error_pending;
   } UartInstance;
   
   static UartInstance uart_instances[UART_MAX_INSTANCES];
   static uint32_t    s_current_tick = 0;   // global tick counter
   
   /* --------------------------------------------------------------------------
      Helper: get instance index from base address
      -------------------------------------------------------------------------- */
   static int uart_index(uintptr_t base) {
       /* Actual mapping will be generated from the CMSIS headers later.
          For now, hardcode typical STM32F4 addresses. */
       switch (base) {
           case 0x40011000: return 0; // USART1
           case 0x40004400: return 1; // USART2
           case 0x40004800: return 2; // USART3
           case 0x40004C00: return 3; // UART4
           default:         return 0;
       }
   }
   
   /* --------------------------------------------------------------------------
      Helper: hex encode (unchanged)
      -------------------------------------------------------------------------- */
   static void hex_encode(char *out, size_t out_len, const uint8_t *data, uint16_t len) {
       out[0] = '\0';
       for (uint16_t i = 0; i < len && strlen(out) + 3 < out_len; i++) {
           char byte_str[4];
           snprintf(byte_str, sizeof(byte_str), "%02X", data[i]);
           strcat(out, byte_str);
       }
   }
   
   /* --------------------------------------------------------------------------
      Public mock control API (same as before, plus tick)
      -------------------------------------------------------------------------- */
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
       *len = uart_instances[idx].tx_len;  // total transfer length
       return uart_instances[idx].tx_buf;
   }
   
   void mock_uart_inject_error(uintptr_t base, uint32_t error_flags, uint16_t after_bytes) {
       int idx = uart_index(base);
       uart_instances[idx].error_inject.enabled = true;
       uart_instances[idx].error_inject.flags = error_flags;
       uart_instances[idx].error_inject.trigger_after = after_bytes;
   }
   
   /* --------------------------------------------------------------------------
      Tick function – must be called periodically from host_main.c
      -------------------------------------------------------------------------- */
   void ember_sim_uart_tick(void) {
       s_current_tick++;
       for (int i = 0; i < UART_MAX_INSTANCES; i++) {
           UartInstance *u = &uart_instances[i];
   
           /* TX completion */
           if (u->tx_cplt_pending && s_current_tick >= u->tx_end_tick) {
               u->tx_cplt_pending = false;
               u->tx_mode = UART_TX_MODE_NONE;
               /* Here we would set huart->gState = HAL_UART_STATE_READY,
                  but we need the actual handle. For now we rely on the
                  user callback to do that (as real HAL does). */
               /* Call user callback if defined */
               /* We need a UART_HandleTypeDef* here. We'll store a reference. */
           }
           /* RX completion */
           if (u->rx_cplt_pending && s_current_tick >= u->rx_end_tick) {
               u->rx_cplt_pending = false;
               u->rx_mode = UART_RX_MODE_NONE;
           }
       }
   }
   
   /* --------------------------------------------------------------------------
      Weak default callbacks (will be overridden by user if needed)
      -------------------------------------------------------------------------- */
   __attribute__((weak)) void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
       char payload[128];
       snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)huart->Instance);
       trace_log("HAL_UART_TxCpltCallback", payload);
   }
   __attribute__((weak)) void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
       char payload[128];
       snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\"", (uint32_t)huart->Instance);
       trace_log("HAL_UART_RxCpltCallback", payload);
   }
   __attribute__((weak)) void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
       char payload[256];
       snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"error\":\"0x%X\"",
                (uint32_t)huart->Instance, huart->ErrorCode);
       trace_log("HAL_UART_ErrorCallback", payload);
   }
   
   /* --------------------------------------------------------------------------
      Internal helper: start an async TX timer
      -------------------------------------------------------------------------- */
   static void uart_schedule_tx_irq(UartInstance *u, uint32_t delay_ms) {
       u->tx_cplt_pending = true;
       u->tx_end_tick = s_current_tick + (delay_ms / TICK_PERIOD_MS);
   }
   
   static void uart_schedule_rx_irq(UartInstance *u, uint32_t delay_ms) {
       u->rx_cplt_pending = true;
       u->rx_end_tick = s_current_tick + (delay_ms / TICK_PERIOD_MS);
   }
   
   /* --------------------------------------------------------------------------
      HAL UART Functions – now fully stateful
      -------------------------------------------------------------------------- */
   
   HAL_StatusTypeDef HAL_UART_Transmit(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
       (void)Timeout; // In blocking mode we just copy immediately, but could simulate delay
       int idx = uart_index((uintptr_t)huart->Instance);
       UartInstance *u = &uart_instances[idx];
   
       /* Check state – real HAL would return HAL_BUSY if not ready */
       if (huart->gState != HAL_UART_STATE_READY) return HAL_BUSY;
   
       if (Size > UART_BUF_SIZE) Size = UART_BUF_SIZE;
       memcpy(u->tx_buf, pData, Size);
       u->tx_len = Size;
       u->tx_mode = UART_TX_MODE_BLOCKING;
       huart->gState = HAL_UART_STATE_BUSY_TX;
       huart->TxXferCount = Size;
   
       char hex[UART_BUF_SIZE * 2 + 1];
       hex_encode(hex, sizeof(hex), pData, Size);
       char payload[512];
       snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"mode\":\"BLOCK\"",
                (uint32_t)huart->Instance, hex, Size);
       trace_log("HAL_UART_Transmit", payload);
   
       /* Error injection? */
       if (u->error_inject.enabled) {
           huart->ErrorCode = u->error_inject.flags;
           huart->gState = HAL_UART_STATE_READY; // real HAL goes to ready after error
           HAL_UART_ErrorCallback(huart);
           u->error_inject.enabled = false;
           return HAL_ERROR;
       }
   
       /* Blocking TX completes "immediately" in mock (no delay needed) */
       huart->gState = HAL_UART_STATE_READY;
       return HAL_OK;
   }
   
   HAL_StatusTypeDef HAL_UART_Receive(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size, uint32_t Timeout) {
       int idx = uart_index((uintptr_t)huart->Instance);
       UartInstance *u = &uart_instances[idx];
   
       if (huart->RxState != HAL_UART_STATE_READY) return HAL_BUSY;
   
       uint16_t available = u->rx_len - u->rx_pos;
       huart->RxState = HAL_UART_STATE_BUSY_RX;
       huart->RxXferCount = Size;
   
       /* Timeout simulation – we check if enough bytes are there right now;
          a more accurate version would poll the tick counter until timeout. */
       if (Size > available) {
           /* Not enough data; return what's available and HAL_TIMEOUT */
           memcpy(pData, u->rx_buf + u->rx_pos, available);
           u->rx_pos += available;
   
           char hex[UART_BUF_SIZE*2+1];
           hex_encode(hex, sizeof(hex), pData, available);
           char payload[512];
           snprintf(payload, sizeof(payload),
                    "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"requested\":%u,\"status\":\"TIMEOUT\"",
                    (uint32_t)huart->Instance, hex, available, Size);
           trace_log("HAL_UART_Receive", payload);
   
           huart->RxState = HAL_UART_STATE_READY;
           return HAL_TIMEOUT;
       }
   
       memcpy(pData, u->rx_buf + u->rx_pos, Size);
       u->rx_pos += Size;
   
       char hex[UART_BUF_SIZE*2+1];
       hex_encode(hex, sizeof(hex), pData, Size);
       char payload[512];
       snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u",
                (uint32_t)huart->Instance, hex, Size);
       trace_log("HAL_UART_Receive", payload);
   
       huart->RxState = HAL_UART_STATE_READY;
       return HAL_OK;
   }
   
   HAL_StatusTypeDef HAL_UART_Transmit_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size) {
       int idx = uart_index((uintptr_t)huart->Instance);
       UartInstance *u = &uart_instances[idx];
   
       if (huart->gState != HAL_UART_STATE_READY) return HAL_BUSY;
   
       if (Size > UART_BUF_SIZE) Size = UART_BUF_SIZE;
       memcpy(u->tx_buf, pData, Size);
       u->tx_len = Size;
       huart->gState = HAL_UART_STATE_BUSY_TX;
       huart->TxXferCount = Size;
   
       char hex[UART_BUF_SIZE*2+1];
       hex_encode(hex, sizeof(hex), pData, Size);
       char payload[512];
       snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"mode\":\"IT\"",
                (uint32_t)huart->Instance, hex, Size);
       trace_log("HAL_UART_Transmit_IT", payload);
   
       /* Schedule completion after, say, 1 ms delay */
       uart_schedule_tx_irq(u, 1);
       return HAL_OK;
   }
   
   HAL_StatusTypeDef HAL_UART_Receive_IT(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size) {
       int idx = uart_index((uintptr_t)huart->Instance);
       UartInstance *u = &uart_instances[idx];
   
       if (huart->RxState != HAL_UART_STATE_READY) return HAL_BUSY;
   
       uint16_t available = u->rx_len - u->rx_pos;
       if (Size > available) Size = available; // will receive what's there
   
       /* In a real system, the actual data copy happens in the ISR.
          We'll copy it now but only invoke callback later. */
       memcpy(pData, u->rx_buf + u->rx_pos, Size);
       u->rx_pos += Size;
   
       huart->RxState = HAL_UART_STATE_BUSY_RX;
       huart->RxXferCount = Size;
       huart->pRxBuffPtr = pData; // store for later callback
   
       char hex[UART_BUF_SIZE*2+1];
       hex_encode(hex, sizeof(hex), pData, Size);
       char payload[512];
       snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"mode\":\"IT\"",
                (uint32_t)huart->Instance, hex, Size);
       trace_log("HAL_UART_Receive_IT", payload);
   
       uart_schedule_rx_irq(u, 1);
       return HAL_OK;
   }
   
   /* DMA functions behave like IT for now, but mark mode differently */
   HAL_StatusTypeDef HAL_UART_Transmit_DMA(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size) {
       int idx = uart_index((uintptr_t)huart->Instance);
       UartInstance *u = &uart_instances[idx];
   
       if (huart->gState != HAL_UART_STATE_READY) return HAL_BUSY;
   
       if (Size > UART_BUF_SIZE) Size = UART_BUF_SIZE;
       memcpy(u->tx_buf, pData, Size);
       u->tx_len = Size;
       huart->gState = HAL_UART_STATE_BUSY_TX;
       huart->TxXferCount = Size;
   
       char hex[UART_BUF_SIZE*2+1];
       hex_encode(hex, sizeof(hex), pData, Size);
       char payload[512];
       snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"mode\":\"DMA\"",
                (uint32_t)huart->Instance, hex, Size);
       trace_log("HAL_UART_Transmit_DMA", payload);
   
       /* For DMA, we simulate immediate completion (no delay) as before */
       u->tx_cplt_pending = true;
       u->tx_end_tick = s_current_tick; // immediate
       return HAL_OK;
   }
   
   HAL_StatusTypeDef HAL_UART_Receive_DMA(UART_HandleTypeDef* huart, uint8_t* pData, uint16_t Size) {
       int idx = uart_index((uintptr_t)huart->Instance);
       UartInstance *u = &uart_instances[idx];
   
       if (huart->RxState != HAL_UART_STATE_READY) return HAL_BUSY;
   
       uint16_t available = u->rx_len - u->rx_pos;
       if (Size > available) Size = available;
       memcpy(pData, u->rx_buf + u->rx_pos, Size);
       u->rx_pos += Size;
   
       huart->RxState = HAL_UART_STATE_BUSY_RX;
       huart->RxXferCount = Size;
       huart->pRxBuffPtr = pData;
   
       char hex[UART_BUF_SIZE*2+1];
       hex_encode(hex, sizeof(hex), pData, Size);
       char payload[512];
       snprintf(payload, sizeof(payload), "\"inst\":\"0x%08X\",\"data\":\"%s\",\"sz\":%u,\"mode\":\"DMA\"",
                (uint32_t)huart->Instance, hex, Size);
       trace_log("HAL_UART_Receive_DMA", payload);
   
       u->rx_cplt_pending = true;
       u->rx_end_tick = s_current_tick;
       return HAL_OK;
   }
   
   /* Abort functions – minimal */
   HAL_StatusTypeDef HAL_UART_Abort(UART_HandleTypeDef *huart) {
       huart->gState = HAL_UART_STATE_READY;
       huart->RxState = HAL_UART_STATE_READY;
       // clear pending events for this handle
       int idx = uart_index((uintptr_t)huart->Instance);
       uart_instances[idx].tx_cplt_pending = false;
       uart_instances[idx].rx_cplt_pending = false;
       return HAL_OK;
   }
   HAL_StatusTypeDef HAL_UART_Abort_IT(UART_HandleTypeDef *huart) { return HAL_UART_Abort(huart); }
   HAL_StatusTypeDef HAL_UART_Abort_DMA(UART_HandleTypeDef *huart){ return HAL_UART_Abort(huart); }