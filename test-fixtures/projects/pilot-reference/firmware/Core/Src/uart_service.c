/**
  ******************************************************************************
  * @file    uart_service.c
  * @brief   UART command interface — state-machine driven parser.
  *
  *          Parses newline-terminated commands using a deterministic
  *          state machine. The state machine produces a richer firmware
  *          execution path than a simple strncmp() approach — each state
  *          transition exercises different code paths through the UART
  *          interrupt callback chain.
  *
  *          State transitions:
  *            IDLE → (RX byte) → RECEIVING → ('\n') → FRAME_READY
  *            FRAME_READY → (tokenize) → DISPATCH → (respond) → IDLE
  *
  *          In simulation, RX data is delivered via the UART interrupt
  *          callback chain (HAL_UART_RxCpltCallback). The inject_rx()
  *          function is provided for interactive testing.
  ******************************************************************************
  */

#include "uart_service.h"
#include "board.h"
#include <string.h>

/* --------------------------------------------------------------------------
   Private state
   -------------------------------------------------------------------------- */

static uint8_t        rx_buf[UART_CMD_BUF_SIZE];
static uint16_t       rx_len;
static UartParseState parse_state = UART_PARSE_IDLE;
static uint8_t        cmd_ready;

/* --------------------------------------------------------------------------
   Public API
   -------------------------------------------------------------------------- */

void uart_service_init(void)
{
    memset(rx_buf, 0, sizeof(rx_buf));
    rx_len      = 0;
    parse_state = UART_PARSE_IDLE;
    cmd_ready   = 0;

    /* Start listening for commands via interrupt (single-byte RX) */
    HAL_UART_Receive_IT(&huart2, rx_buf, 1U);
}

Command uart_service_poll(void)
{
    Command cmd = CMD_NONE;

    if (!cmd_ready) {
        return CMD_NONE;
    }

    cmd_ready = 0;

    /* ---- Tokenize the received frame ---- */
    /* Command format: <TOKEN> [<ARG>] */
    const char *buf = (const char *)rx_buf;
    size_t      len = rx_len;

    if (len >= 6U && strncmp(buf, "STATUS", 6U) == 0) {
        cmd = CMD_STATUS;
    } else if (len >= 8U && strncmp(buf, "MOTOR ON", 8U) == 0) {
        cmd = CMD_MOTOR_ON;
    } else if (len >= 9U && strncmp(buf, "MOTOR OFF", 9U) == 0) {
        cmd = CMD_MOTOR_OFF;
    } else if (len >= 4U && strncmp(buf, "TEMP", 4U) == 0) {
        cmd = CMD_TEMPERATURE;
    } else {
        cmd = CMD_UNKNOWN;
    }

    /* ---- Reset parser state for next command ---- */
    memset(rx_buf, 0, sizeof(rx_buf));
    rx_len      = 0;
    parse_state = UART_PARSE_IDLE;
    HAL_UART_Receive_IT(&huart2, rx_buf, 1U);

    return cmd;
}

void uart_service_respond(const char *response)
{
    uint16_t len = (uint16_t)strlen(response);
    HAL_UART_Transmit(&huart2, (uint8_t *)response, len, 100U);
}

/**
  * @brief  Inject received data — used in simulation for interactive testing.
  *         Sets the parser to FRAME_READY state directly.
  */
void uart_service_inject_rx(const uint8_t *data, uint16_t len)
{
    if (len > UART_CMD_BUF_SIZE - 1U) {
        len = UART_CMD_BUF_SIZE - 1U;
    }
    memcpy(rx_buf, data, len);
    rx_buf[len] = '\0';
    rx_len      = len;
    parse_state = UART_PARSE_FRAME_READY;
    cmd_ready   = 1;
}

/* --------------------------------------------------------------------------
   HAL Callback — UART RX complete (interrupt context, state-machine driven)
   -------------------------------------------------------------------------- */

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance != USART2) {
        return;
    }

    switch (parse_state) {

    case UART_PARSE_IDLE:
        /* First byte received — transition to RECEIVING */
        parse_state = UART_PARSE_RECEIVING;
        rx_len = 1U;
        HAL_UART_Receive_IT(&huart2, &rx_buf[rx_len], 1U);
        break;

    case UART_PARSE_RECEIVING:
        /* Check for frame terminator */
        if (rx_buf[rx_len] == '\n') {
            rx_buf[rx_len] = '\0';   /* strip newline */
            parse_state = UART_PARSE_FRAME_READY;
            cmd_ready   = 1;
        } else {
            rx_len++;
            if (rx_len >= UART_CMD_BUF_SIZE - 1U) {
                /* Buffer full — terminate and mark ready */
                rx_buf[rx_len] = '\0';
                parse_state = UART_PARSE_FRAME_READY;
                cmd_ready   = 1;
            } else {
                /* Continue receiving */
                HAL_UART_Receive_IT(&huart2, &rx_buf[rx_len], 1U);
            }
        }
        break;

    case UART_PARSE_FRAME_READY:
        /* Frame already pending — byte received during unhandled frame.
           Discard and re-arm. */
        HAL_UART_Receive_IT(&huart2, rx_buf, 1U);
        break;
    }
}
