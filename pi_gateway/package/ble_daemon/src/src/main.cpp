#include "modules/ScannerModule.h"
#include "modules/ProvisioningModule.h"
#include <iostream>
#include <thread>
#include <chrono>

int main() {
    std::cout << std::unitbuf; // Disable stdout buffering
    std::cout << "Starting Minimal BLE Daemon (C++23)..." << std::endl;
    
    try {
        ble::HCIController hci;
        ble::ScannerModule scanner(hci);
        ble::ProvisioningModule provisioner(hci);
        
        hci.add_module(&scanner);
        hci.add_module(&provisioner);
        
        // Native attachment removed as kernel handles it
        
        hci.open_device(0); // Open hci0
        // hci.start_scan(); // Start Scanning - DISABLED FOR DEBUG
        provisioner.start_advertising(); // Start Advertising

        // Simple loop
        while(true) {
            hci.process_events();
            // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
