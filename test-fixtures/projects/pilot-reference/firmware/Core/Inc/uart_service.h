/**
  ******************************************************************************
  * @file    uart_service.h
  * @brief   UART command interface — state-machine driven parser.
  *
  *          Parses newline-terminated ASCII commands using a deterministic
  *          state machine: IDLE → RECEIVING → FRAME_READY → DISPATCH.
  *
  *          Supported commands:
  *            STATUS     → status query
  *            MOTOR ON   → start motor
  *            MOTOR OFF  → stop motor
  *            TEMP       → read temperature sensor
  ******************************************************************************
  */

#ifndef __UART_SERVICE_H
#define __UART_SERVICE_H

#include <stdint.h>

#define UART_CMD_BUF_SIZE  64

/* Parser state machine */
typedef enum {
    UART_PARSE_IDLE = 0,
    UART_PARSE_RECEIVING,
    UART_PARSE_FRAME_READY,
} UartParseState;

/* Recognized commands */
typedef enum {
    CMD_NONE = 0,
    CMD_STATUS,
    CMD_MOTOR_ON,
    CMD_MOTOR_OFF,
    CMD_TEMPERATURE,
    CMD_UNKNOWN,
} Command;

/* Public API */
void    uart_service_init(void);
Command uart_service_poll(void);
void    uart_service_respond(const char *response);
void    uart_service_inject_rx(const uint8_t *data, uint16_t len);

#endif /* __UART_SERVICE_H */
