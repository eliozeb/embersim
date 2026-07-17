#ifndef MOCK_SPI_H
#define MOCK_SPI_H

#include <stdint.h>
#include <stddef.h>

/* Forward declaration */
typedef struct VirtualSPIDevice VirtualSPIDevice;

/* Called when chip select goes LOW */
typedef void (*SPI_OnSelect)(VirtualSPIDevice *dev);
/* Called when chip select goes HIGH */
typedef void (*SPI_OnDeselect)(VirtualSPIDevice *dev);
/* Called for each transmitted byte. Returns the byte the device sends back (MISO). */
typedef uint8_t (*SPI_OnByte)(VirtualSPIDevice *dev, uint8_t tx_byte);

struct VirtualSPIDevice {
    const char *name;         /* Human‑readable name */
    void *user_data;          /* Opaque pointer for device state */
    SPI_OnSelect   on_select;
    SPI_OnDeselect on_deselect;
    SPI_OnByte     on_byte;
};

/* Register a virtual device on a SPI instance (by base address) */
void mock_spi_add_device(uintptr_t spi_base, VirtualSPIDevice *dev);

/* YAML‑based device loading – stub for now (will be implemented later) */
void mock_spi_load_device_yaml(uintptr_t spi_base, const char *yaml_path);

#endif /* MOCK_SPI_H */