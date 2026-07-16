/* mock_state.c — Stateful peripheral simulation for host-native testing */
#include "mock_hal.h"
#include <string.h>

/* ========== GPIO State ========== */
#define MAX_GPIO_PORTS  8
#define MAX_GPIO_PINS   16

static uint16_t gpio_pin_states[MAX_GPIO_PORTS];   /* bitfield: 1 = HIGH */
static uint16_t gpio_pin_configs[MAX_GPIO_PORTS];  /* bitfield: 1 = output */

/* Map base address to port index */
static int gpio_port_index(uint32_t base) {
    /* Dummy mapping for x86: GPIOA_BASE = 0x10000000, etc. */
    switch (base) {
        case 0x10000000: return 0; /* GPIOA */
        case 0x10000400: return 1; /* GPIOB */
        case 0x10000800: return 2; /* GPIOC */
        default: return 0;
    }
}

void mock_gpio_init(void) {
    memset(gpio_pin_states, 0, sizeof(gpio_pin_states));
    memset(gpio_pin_configs, 0, sizeof(gpio_pin_configs));
}

void mock_gpio_set_input(uint32_t port_base, uint16_t pin, uint8_t value) {
    int port = gpio_port_index(port_base);
    if (value) {
        gpio_pin_states[port] |= pin;
    } else {
        gpio_pin_states[port] &= ~pin;
    }
}

/* ========== Stateful HAL Implementations ========== */

void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState) {
    int port = gpio_port_index((uint32_t)GPIOx);
    if (PinState == GPIO_PIN_SET) {
        gpio_pin_states[port] |= GPIO_Pin;
    } else {
        gpio_pin_states[port] &= ~GPIO_Pin;
    }
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    int port = gpio_port_index((uint32_t)GPIOx);
    return (gpio_pin_states[port] & GPIO_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin) {
    int port = gpio_port_index((uint32_t)GPIOx);
    gpio_pin_states[port] ^= GPIO_Pin;
}