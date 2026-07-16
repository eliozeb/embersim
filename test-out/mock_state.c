/* mock_state.c — Stateful GPIO simulation for host-native testing */
#include "mock_hal.h"
#include "trace_log.h"   // <-- ADD THIS
#include <string.h>
#include <stdint.h>

/* ========== GPIO State ========== */
#define MAX_GPIO_PORTS  8

static uint16_t gpio_pin_states[MAX_GPIO_PORTS];

/* Dummy base addresses for x86 */
#define GPIOA_BASE  ((uintptr_t)0x10000000U)
#define GPIOB_BASE  ((uintptr_t)0x10000400U)
#define GPIOC_BASE  ((uintptr_t)0x10000800U)
#define GPIOD_BASE  ((uintptr_t)0x10000C00U)
#define GPIOE_BASE  ((uintptr_t)0x10001000U)
#define GPIOF_BASE  ((uintptr_t)0x10001400U)
#define GPIOG_BASE  ((uintptr_t)0x10001800U)
#define GPIOH_BASE  ((uintptr_t)0x10001C00U)

/* Map peripheral base address to port index */
static int gpio_port_index(uintptr_t base)
{
    switch (base) {
        case GPIOA_BASE: return 0;
        case GPIOB_BASE: return 1;
        case GPIOC_BASE: return 2;
        case GPIOD_BASE: return 3;
        case GPIOE_BASE: return 4;
        case GPIOF_BASE: return 5;
        case GPIOG_BASE: return 6;
        case GPIOH_BASE: return 7;
        default:         return 0;
    }
}

/* Public API for test setup */
void mock_gpio_set_input(uintptr_t port_base, uint16_t pin, uint8_t value)
{
    int port = gpio_port_index(port_base);
    if (value) {
        gpio_pin_states[port] |= pin;
    } else {
        gpio_pin_states[port] &= ~pin;
    }
}

/* Initialize all GPIO state to LOW */
void mock_gpio_init(void)
{
    memset(gpio_pin_states, 0, sizeof(gpio_pin_states));
}

/* ========== Stateful HAL Implementations ========== */

void HAL_GPIO_WritePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin, GPIO_PinState PinState)
{
    trace_log("HAL_GPIO_WritePin", "GPIOx,GPIO_Pin,PinState");  // <-- ADD THIS
    int port = gpio_port_index((uintptr_t)GPIOx);
    if (PinState == GPIO_PIN_SET) {
        gpio_pin_states[port] |= GPIO_Pin;
    } else {
        gpio_pin_states[port] &= ~GPIO_Pin;
    }
}

GPIO_PinState HAL_GPIO_ReadPin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    trace_log("HAL_GPIO_ReadPin", "GPIOx,GPIO_Pin");  // <-- ADD THIS
    int port = gpio_port_index((uintptr_t)GPIOx);
    return (gpio_pin_states[port] & GPIO_Pin) ? GPIO_PIN_SET : GPIO_PIN_RESET;
}

void HAL_GPIO_TogglePin(GPIO_TypeDef* GPIOx, uint16_t GPIO_Pin)
{
    trace_log("HAL_GPIO_TogglePin", "GPIOx,GPIO_Pin");  // <-- ADD THIS
    int port = gpio_port_index((uintptr_t)GPIOx);
    gpio_pin_states[port] ^= GPIO_Pin;
}