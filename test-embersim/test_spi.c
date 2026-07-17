#include <stdio.h>
#include <string.h>
#include "mock_hal.h"
#include "mock_spi.h"
#include "trace_log.h"

/* SPI error codes (from mock_hal.h) */
#ifndef HAL_SPI_ERROR_CRC
#define HAL_SPI_ERROR_CRC 0x00000001U
#endif

/* mock control prototypes */
extern void mock_spi_init(void);
extern void mock_spi_add_device(uintptr_t base, VirtualSPIDevice *dev);
extern void mock_spi_inject_error(uintptr_t base, uint32_t error);
extern void ember_sim_spi_tick(void);

/* ---------- simple virtual flash device ---------- */
static uint8_t flash_id[] = {0xEF, 0x40, 0x18};
static int flash_state = 0; /* 0 = waiting for command */

static void flash_on_select(VirtualSPIDevice *dev) {
    (void)dev;
    flash_state = 0;
}

static void flash_on_deselect(VirtualSPIDevice *dev) {
    (void)dev;
    flash_state = 0;
}

static uint8_t flash_on_byte(VirtualSPIDevice *dev, uint8_t tx) {
    (void)dev;
    if (flash_state == 0 && tx == 0x9F) {
        /* JEDEC ID command */
        flash_state = 1;
        return 0x00; /* dummy while command byte shifts */
    }
    if (flash_state == 1) {
        static int idx = 0;
        uint8_t ret = flash_id[idx];
        idx++;
        if (idx >= 3) { idx = 0; flash_state = 0; }
        return ret;
    }
    return 0xFF; /* default MISO */
}

static VirtualSPIDevice flash_dev = {
    .name = "W25Q32",
    .user_data = NULL,
    .on_select = flash_on_select,
    .on_deselect = flash_on_deselect,
    .on_byte = flash_on_byte
};

/* ---------- overridden callbacks ---------- */
static int tx_cplt = 0, rx_cplt = 0, txrx_cplt = 0, error_cb = 0;

void HAL_SPI_TxCpltCallback(SPI_HandleTypeDef *hspi) { tx_cplt++; }
void HAL_SPI_RxCpltCallback(SPI_HandleTypeDef *hspi) { rx_cplt++; }
void HAL_SPI_TxRxCpltCallback(SPI_HandleTypeDef *hspi) { txrx_cplt++; }
void HAL_SPI_ErrorCallback(SPI_HandleTypeDef *hspi) { error_cb++; }

int main(void) {
    int failures = 0;
    trace_log_init("trace_spi.jsonl");
    mock_spi_init();

    SPI_HandleTypeDef hspi1 = {
        .Instance = (uint32_t)0x40013000, /* SPI1 */
        .ErrorCode = 0
    };

    /* register virtual flash on SPI1 */
    mock_spi_add_device(0x40013000, &flash_dev);

    /* Test 1: JEDEC ID read using TransmitReceive */
    {
        uint8_t tx[] = {0x9F, 0x00, 0x00, 0x00};
        uint8_t rx[4];
        HAL_StatusTypeDef st = HAL_SPI_TransmitReceive(&hspi1, tx, rx, 4, 100);
        if (st != HAL_OK || rx[1] != 0xEF || rx[2] != 0x40 || rx[3] != 0x18) {
            printf("FAIL: JEDEC ID read\n"); failures++;
        } else {
            printf("PASS: JEDEC ID read — got EF 40 18\n");
        }
    }

    /* Test 2: IT TransmitReceive with callback */
    {
        txrx_cplt = 0;
        uint8_t tx[] = {0x9F, 0x00, 0x00, 0x00};
        uint8_t rx[4];
        HAL_StatusTypeDef st = HAL_SPI_TransmitReceive_IT(&hspi1, tx, rx, 4);
        if (st != HAL_OK) { printf("FAIL: IT TxRx start\n"); failures++; }
        else {
            ember_sim_spi_tick(); ember_sim_spi_tick();
            if (!txrx_cplt || rx[1] != 0xEF) {
                printf("FAIL: IT TxRx callback\n"); failures++;
            } else printf("PASS: IT TxRx with callback\n");
        }
    }

    /* Test 3: Error injection (CRC error) */
    {
        error_cb = 0;
        mock_spi_inject_error(0x40013000, HAL_SPI_ERROR_CRC);
        uint8_t tx[] = {0xFF};
        HAL_StatusTypeDef st = HAL_SPI_Transmit(&hspi1, tx, 1, 100);
        if (st != HAL_ERROR || !error_cb) { printf("FAIL: CRC error injection\n"); failures++; }
        else printf("PASS: CRC error injection\n");
    }

    trace_log_close();
    FILE *f = fopen("trace_spi.jsonl", "r");
    if (f) { char line[512]; printf("\n--- Trace ---\n"); while (fgets(line,sizeof(line),f)) printf("%s",line); fclose(f); }

    if (!failures) printf("\nALL SPI TESTS PASSED\n");
    else printf("\n%d TEST(S) FAILED\n", failures);
    return failures;
}