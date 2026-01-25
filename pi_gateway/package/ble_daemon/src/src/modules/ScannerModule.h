#ifndef MODULES_SCANNER_MODULE_H
#define MODULES_SCANNER_MODULE_H

#include "../core/Module.h"
#include "../core/HCIController.h"

namespace ble {

class ScannerModule : public Module {
public:
    explicit ScannerModule(HCIController& hci);
    
    void on_le_meta_event(uint8_t subevent_code, std::span<const uint8_t> data) override;

private:
    HCIController& hci_;
};

} // namespace ble

#endif // MODULES_SCANNER_MODULE_H
