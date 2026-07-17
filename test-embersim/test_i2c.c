#include <stdio.h>
#include <string.h>
#include "mock_hal.h"
#include "trace_log.h"

#ifndef HAL_I2C_ERROR_NONE
#define HAL_I2C_ERROR_NONE  0x00000000U
#endif
#ifndef HAL_I2C_ERROR_AF
#define HAL_I2C_ERROR_AF    0x00000004U
#endif

/* mock control prototypes */
extern void mock_i2c_init(void);
extern void mock_i2c_add_device(uintptr_t i2c_base, uint8_t dev_addr,
                                const uint8_t *reg_data, uint16_t reg_size);
extern void mock_i2c_inject_error(uintptr_t i2c_base, uint32_t error);
extern void ember_sim_i2c_tick(void);

/* overridden callbacks */
static int tx_cplt = 0, rx_cplt = 0, error_cb = 0;

void HAL_I2C_MasterTxCpltCallback(I2C_HandleTypeDef *hi2c) { tx_cplt++; }
void HAL_I2C_MasterRxCpltCallback(I2C_HandleTypeDef *hi2c) { rx_cplt++; }
void HAL_I2C_MemTxCpltCallback(I2C_HandleTypeDef *hi2c)  { tx_cplt++; }
void HAL_I2C_MemRxCpltCallback(I2C_HandleTypeDef *hi2c)  { rx_cplt++; }
void HAL_I2C_ErrorCallback(I2C_HandleTypeDef *hi2c)       { error_cb++; }

int main(void) {
    int failures = 0;
    trace_log_init("trace_i2c.jsonl");
    mock_i2c_init();

    I2C_HandleTypeDef hi2c1 = {
        .Instance = (uint32_t)0x40005400,
        .ErrorCode = 0
    };

    /* virtual sensor with 3 registers: reg0=0x1A, reg1=0x3B, reg2=0x00 */
    uint8_t sensor_regs[3] = {0x1A, 0x3B, 0x00};
    mock_i2c_add_device(0x40005400, 0x48, sensor_regs, sizeof(sensor_regs));

    /* Test 1: Mem Write a single byte, then read it back */
    {
        uint8_t tx_data[] = {0xAB};  // write 0xAB to register 0x01
        HAL_StatusTypeDef st = HAL_I2C_Mem_Write(&hi2c1, 0x48, 0x01, 1, tx_data, 1, 100);
        if (st != HAL_OK) {
            printf("FAIL: Mem Write\n"); failures++;
        } else {
            uint8_t rx[1];
            st = HAL_I2C_Mem_Read(&hi2c1, 0x48, 0x01, 1, rx, 1, 100);
            if (st != HAL_OK || rx[0] != 0xAB) {
                printf("FAIL: Mem Read after write — expected 0xAB, got 0x%02X\n", rx[0]);
                failures++;
            } else {
                printf("PASS: Mem Write/Read single register\n");
            }
        }
    }

    /* Test 2: IT Mem Read the original register 0x00 (should still be 0x1A) */
    {
        rx_cplt = 0;
        uint8_t rx[1];
        HAL_StatusTypeDef st = HAL_I2C_Mem_Read_IT(&hi2c1, 0x48, 0x00, 1, rx, 1);
        if (st != HAL_OK) {
            printf("FAIL: Mem Read IT start\n"); failures++;
        } else {
            ember_sim_i2c_tick(); ember_sim_i2c_tick();
            if (rx_cplt == 0 || rx[0] != 0x1A) {
                printf("FAIL: Mem Read IT callback — expected 0x1A, got 0x%02X\n", rx[0]);
                failures++;
            } else {
                printf("PASS: IT Mem Read original register\n");
            }
        }
    }

    /* Test 3: NACK error */
    {
        error_cb = 0;
        mock_i2c_inject_error(0x40005400, HAL_I2C_ERROR_AF);
        uint8_t dummy[] = {0xFF};
        HAL_StatusTypeDef st = HAL_I2C_Master_Transmit(&hi2c1, 0x48, dummy, 1, 100);
        if (st != HAL_ERROR || error_cb == 0) {
            printf("FAIL: NACK error\n"); failures++;
        } else {
            printf("PASS: NACK error callback\n");
        }
    }

    trace_log_close();
    FILE *f = fopen("trace_i2c.jsonl", "r");
    if (f) {
        char line[512];
        printf("\n--- Trace ---\n");
        while (fgets(line, sizeof(line), f)) printf("%s", line);
        fclose(f);
    }

    if (!failures) printf("\nALL I2C TESTS PASSED\n");
    else printf("\n%d TEST(S) FAILED\n", failures);
    return failures;
}