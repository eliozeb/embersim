#include "mock_hal.h"
#include "trace_log.h"
#include <stdio.h>

extern void mock_gpio_init(void);
extern void mock_gpio_set_input(uintptr_t port_base, uint16_t pin, uint8_t value);

int main(void)
{
    int failures = 0;

    trace_log_init("trace.jsonl");
    mock_gpio_init();

    /* Test 1: Write HIGH, read back HIGH */
    HAL_GPIO_WritePin((GPIO_TypeDef*)0x10000000, 1 << 5, GPIO_PIN_SET);
    if (HAL_GPIO_ReadPin((GPIO_TypeDef*)0x10000000, 1 << 5) != GPIO_PIN_SET) {
        printf("FAIL: Test 1\n");
        failures++;
    } else {
        printf("PASS: Test 1 — PA5 is HIGH\n");
    }

    /* Test 2: Write LOW, read back LOW */
    HAL_GPIO_WritePin((GPIO_TypeDef*)0x10000000, 1 << 5, GPIO_PIN_RESET);
    if (HAL_GPIO_ReadPin((GPIO_TypeDef*)0x10000000, 1 << 5) != GPIO_PIN_RESET) {
        printf("FAIL: Test 2\n");
        failures++;
    } else {
        printf("PASS: Test 2 — PA5 is LOW\n");
    }

    /* Test 3: Toggle */
    HAL_GPIO_WritePin((GPIO_TypeDef*)0x10000000, 1 << 5, GPIO_PIN_RESET);
    HAL_GPIO_TogglePin((GPIO_TypeDef*)0x10000000, 1 << 5);
    if (HAL_GPIO_ReadPin((GPIO_TypeDef*)0x10000000, 1 << 5) != GPIO_PIN_SET) {
        printf("FAIL: Test 3\n");
        failures++;
    } else {
        printf("PASS: Test 3 — PA5 toggled to HIGH\n");
    }

    /* Test 4: Pre-set input */
    mock_gpio_set_input(0x10000000, 1 << 0, 1);
    if (HAL_GPIO_ReadPin((GPIO_TypeDef*)0x10000000, 1 << 0) != GPIO_PIN_SET) {
        printf("FAIL: Test 4\n");
        failures++;
    } else {
        printf("PASS: Test 4 — PA0 pre-set to HIGH\n");
    }

    trace_log_close();

    /* Print trace file */
    FILE *f = fopen("trace.jsonl", "r");
    if (f) {
        char line[512];
        printf("\n--- Trace Output ---\n");
        while (fgets(line, sizeof(line), f)) {
            printf("%s", line);
        }
        fclose(f);
    }

    if (failures == 0) {
        printf("\nALL TESTS PASSED\n");
        return 0;
    } else {
        printf("\n%d TEST(S) FAILED\n", failures);
        return 1;
    }
}