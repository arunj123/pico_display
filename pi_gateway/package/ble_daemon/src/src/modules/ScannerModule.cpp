#include "ScannerModule.h"
#include "../core/compat/hci.h"
#include <iostream>
#include <iomanip>
#include <sstream>

namespace ble {

ScannerModule::ScannerModule(HCIController& hci) : hci_(hci) {}

static std::string to_hex(const uint8_t* data, size_t len) {
    std::stringstream ss;
    ss << std::hex << std::setfill('0');
    for(size_t i=0; i<len; ++i) {
        ss << std::setw(2) << (int)data[i];
    }
    return ss.str();
}

void ScannerModule::on_le_meta_event(uint8_t subevent_code, std::span<const uint8_t> data) {
    if (subevent_code == EVT_LE_ADVERTISING_REPORT) {
        if (data.empty()) return;
        
        uint8_t num_reports = data[0];
        size_t offset = 1;
        
        // This is a naive parser assuming 1 report for simplicity, 
        // real parser handles multiple reports (loops based on num_reports)
        // Note: The structure of Advertising Report Event (Subevent 0x02) is actually:
        // Num_Reports (1 byte)
        // Event_Type[i] (1 byte)
        // Address_Type[i] (1 byte)
        // Address[i] (6 bytes)
        // Length_Data[i] (1 byte)
        // Data[i] (Length bytes)
        // RSSI[i] (1 byte)
        
        // Wait, standard HCI is weird. It sends arrays of fields, NOT an array of structs.
        // But for Num_Reports=1 it's sequential. 
        // Let's implement robustly.
        
        // Actually, BlueZ kernel usually unpacks this? No, raw HCI gives it as specd.
        // Spec Says:
        // Num_Reports, Event_Type[k], Address_Type[k], Address[k], Length[k], Data[k], RSSI[k]
        // This is annoying to parse because Data is variable length.
        
        // Let's implement a loop:
        // Unlike "Struct of Arrays" where all EventTypes come first, then all AddressTypes...
        // NO, wait. 
        // Core Spec 5.3 Vol 4, Part E, 7.7.65.2:
        // Num_Reports
        // Event_Type[Num_Reports]
        // Address_Type[Num_Reports]
        // Address[Num_Reports*6]
        // Length[Num_Reports]
        // Data[Num_Reports * Length?? NO] -> Spec says "Data[i]" ...
        // Actually "Data" parameter is a concatenation??
        // 
        // Re-reading Spec:
        // Event_Type (1 octet)
        // Address_Type (1 octet)
        // Address (6 octets)
        // Length (1 octet)
        // Data (Length octets)
        // RSSI (1 octet)
        // ^ This entire block repeats Num_Reports times?
        // 
        // NO! 
        // "The event parameters ... are not grouped by report." - wait, that's wrong.
        // Let's check `libblepp` (our reference).
        // It says `num_reports` then loop `packet.pop_front()`
        // `libblepp` loops: EventType, AddrType, Addr, Length, Data, RSSI.
        // This implies the structure IS interleaved (Struct-like), NOT Struct-of-Arrays.
        // OK, I'll trust `libblepp`.

        for(int i=0; i<num_reports; ++i) {
            if (offset >= data.size()) break;
            
            uint8_t evt_type = data[offset++];
            uint8_t addr_type = data[offset++];
            
            if (offset + 6 > data.size()) break;
            const uint8_t* addr_ptr = data.data() + offset;
            offset += 6;
            
            // Format Address
            std::stringstream addr_ss;
            addr_ss << std::hex << std::setfill('0');
            for(int k=5; k>=0; --k) {
                addr_ss << std::setw(2) << (int)addr_ptr[k] << (k>0?":":"");
            }
            std::string addr_str = addr_ss.str();
            
            uint8_t data_len = data[offset++];
            if (offset + data_len > data.size()) break;
            
            std::span<const uint8_t> adv_data = data.subspan(offset, data_len);
            offset += data_len;
            
            int8_t rssi = (int8_t)data[offset++]; // RSSI is last
            
            // Print
            std::cout << "[SCAN] " << addr_str << " RSSI: " << (int)rssi << "dBm DataLen: " << (int)data_len << std::endl;
        }
    }
}

} // namespace ble
