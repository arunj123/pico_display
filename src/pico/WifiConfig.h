#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include "pico/stdlib.h"
#include "hardware/flash.h"
#include "hardware/sync.h"
#include <cstring>
#include <cstdio> // For printf

// Define Flash Size explicitly if missing (Standard Pico W is 2MB)
#ifndef PICO_FLASH_SIZE_BYTES
#define PICO_FLASH_SIZE_BYTES (2 * 1024 * 1024)
#endif

// Magic bytes to ensure we are reading valid config
constexpr uint32_t CONFIG_MAGIC = 0xCAFEBABE;
// --- Use the 2nd to last sector to avoid BTstack NVM conflict ---
// BTstack uses (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)
constexpr size_t FLASH_TARGET_OFFSET = (PICO_FLASH_SIZE_BYTES - (4 * FLASH_SECTOR_SIZE));

struct WifiCredentials {
    uint32_t magic;
    char ssid[33];
    char password[65];
};

class WifiConfig {
public:
    static bool load(WifiCredentials& creds) {
        const uint8_t *flash_target_contents = (const uint8_t *) (XIP_BASE + FLASH_TARGET_OFFSET);
        
        // Debug: Print what we are reading
        printf("[WifiConfig] Reading from Address: 0x%08X (Offset: 0x%08X)\n", (uint32_t)flash_target_contents, FLASH_TARGET_OFFSET);
        
        memcpy(&creds, flash_target_contents, sizeof(WifiCredentials));
        
        if (creds.magic == CONFIG_MAGIC) {
            printf("[WifiConfig] Valid Config Found! SSID: %s\n", creds.ssid);
            return true;
        } else {
            printf("[WifiConfig] No valid config (Magic: 0x%08X, Expected: 0x%08X)\n", creds.magic, CONFIG_MAGIC);
            return false;
        }
    }

    static void save(const char* ssid, const char* password) {
        printf("[WifiConfig] Saving Credentials... SSID: %s\n", ssid);

        WifiCredentials creds;
        creds.magic = CONFIG_MAGIC;
        strncpy(creds.ssid, ssid, 32);
        creds.ssid[32] = 0; 
        strncpy(creds.password, password, 64);
        creds.password[64] = 0;

        // Writing to flash requires disabling interrupts on BOTH cores in some cases,
        // but primarily we need to ensure XIP is not accessed.
        uint32_t ints = save_and_disable_interrupts();
        
        // 1. Erase the sector
        flash_range_erase(FLASH_TARGET_OFFSET, FLASH_SECTOR_SIZE);
        
        // 2. Program the page
        flash_range_program(FLASH_TARGET_OFFSET, (const uint8_t*)&creds, FLASH_PAGE_SIZE);
        
        restore_interrupts(ints);
        
        printf("[WifiConfig] Save Complete.\n");
    }
};

#endif