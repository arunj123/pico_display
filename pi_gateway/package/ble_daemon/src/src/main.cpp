#include "modules/ScannerModule.h"
#include "modules/ProvisioningModule.h"
#include "modules/TP357Module.h"
#include "core/UDSManager.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <csignal>

int main() {
    std::signal(SIGPIPE, SIG_IGN);
    std::cout << std::unitbuf; // Disable stdout buffering
    std::cout << "Starting Minimal BLE Daemon (C++23)..." << std::endl;
    
    try {
        ble::UDSManager uds("/tmp/ble_sensor_data.sock");
        ble::HCIController hci;
        ble::ScannerModule scanner(hci);
        ble::ProvisioningModule provisioner(hci);
        ble::TP357Module tp357(&uds);
        
        uds.set_on_client_connected([&tp357](int client_fd) {
            tp357.dump_state_to_client(client_fd);
        });

        hci.add_module(&scanner);
        hci.add_module(&provisioner);
        hci.add_module(&tp357);
        
        // Native attachment removed as kernel handles it
        
        hci.open_device(0); // Open hci0
        hci.start_scan(); // Start Scanning
        provisioner.start_advertising(); // Start Advertising

        // Simple loop
        while(true) {
            hci.process_events();
            uds.process();
            // std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

    } catch (const std::exception& e) {
        std::cerr << "Fatal Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
