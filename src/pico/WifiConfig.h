#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <cstring>

// Magic bytes to ensure we are reading valid config, not empty flash (0xFF)
constexpr uint32_t CONFIG_MAGIC = 0xCAFEBABE;
constexpr size_t FLASH_TARGET_OFFSET = (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE);

struct WifiCredentials {
    uint32_t magic;
    char ssid[33];
    char password[65];
};

class WifiConfig {
public:
    static bool load(WifiCredentials& creds) {
        const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
        memcpy(&creds, flash_target_contents, sizeof(WifiCredentials));
        return (creds.magic == CONFIG_MAGIC);
    }

    static void save(const char* ssid, const char* password) {
        WifiCredentials creds;
        creds.magic = CONFIG_MAGIC;
        strncpy(creds.ssid, ssid, 32);
        creds.ssid[32] = 0; // Ensure null termination
        strncpy(creds.password, password, 64);
        creds.password[64] = 0;

        // Writing to flash requires disabling interrupts on the core
        uint32_t ints = save_and_disable_interrupts();
        flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
        flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*)&creds, FLASH_PAGE_SIZE); // Page size alignment is usually enough
        restore_interrupts(ints);
    }
};

#endif