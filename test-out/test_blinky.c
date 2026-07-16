/* test_blinky.c — Verify stateful GPIO mocks work on x86 */
#include "mock_hal.h"
#include <stdio.h>

/* Declared in mock_state.c */
extern void mock_gpio_init(void);
extern void mock_gpio_set_input(uint32_t port_base, uint16_t pin, uint8_t value);

int main(void)
{
    int failures = 0;

    mock_gpio_init();

    /* Test 1: Write HIGH, read back HIGH */
    HAL_GPIO_WritePin((GPIO_TypeDef*)0x10000000, 1 << 5, GPIO_PIN_SET);
    if (HAL_GPIO_ReadPin((GPIO_TypeDef*)0x10000000, 1 << 5) != GPIO_PIN_SET) {
        printf("FAIL: Test 1 — PA5 should be HIGH\n");
        failures++;
    } else {
        printf("PASS: Test 1 — PA5 is HIGH\n");
    }

    /* Test 2: Write LOW, read back LOW */
    HAL_GPIO_WritePin((GPIO_TypeDef*)0x10000000, 1 << 5, GPIO_PIN_RESET);
    if (HAL_GPIO_ReadPin((GPIO_TypeDef*)0x10000000, 1 << 5) != GPIO_PIN_RESET) {
        printf("FAIL: Test 2 — PA5 should be LOW\n");
        failures++;
    } else {
        printf("PASS: Test 2 — PA5 is LOW\n");
    }

    /* Test 3: Toggle */
    HAL_GPIO_WritePin((GPIO_TypeDef*)0x10000000, 1 << 5, GPIO_PIN_RESET);
    HAL_GPIO_TogglePin((GPIO_TypeDef*)0x10000000, 1 << 5);
    if (HAL_GPIO_ReadPin((GPIO_TypeDef*)0x10000000, 1 << 5) != GPIO_PIN_SET) {
        printf("FAIL: Test 3 — PA5 should be HIGH after toggle\n");
        failures++;
    } else {
        printf("PASS: Test 3 — PA5 toggled to HIGH\n");
    }

    /* Test 4: Pre-set input via mock helper */
    mock_gpio_set_input(0x10000000, 1 << 0, 1); /* PA0 = HIGH */
    if (HAL_GPIO_ReadPin((GPIO_TypeDef*)0x10000000, 1 << 0) != GPIO_PIN_SET) {
        printf("FAIL: Test 4 — PA0 should be HIGH (pre-set)\n");
        failures++;
    } else {
        printf("PASS: Test 4 — PA0 pre-set to HIGH\n");
    }

    if (failures == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    } else {
        printf("\n%d TEST(S) FAILED\n", failures);
        return 1;
    }
}