#pragma once

#include <Arduino.h>
#include "esp_gap_bt_api.h"
#include <cstring>
#include <vector>

struct MacAddress {
    uint8_t bytes[6];

    bool operator==(const MacAddress &other) const {
        return memcmp(bytes, other.bytes, sizeof(bytes)) == 0;
    }

    bool operator!=(const MacAddress &other) const {
        return !(*this == other);
    }
};

// Lightweight controller-wake detector:
// - listens for Classic BT ACL connection attempts
// - matches against an allow list
// - reports wake via callback and one-shot polling
class ControllerWakeupLib {
public:
    using WakeCallback = void (*)(const MacAddress &controllerMac);

    ControllerWakeupLib(bool debug = true);
    // Legacy compatibility overload; pin/pulse args are ignored in detection-only mode.
    ControllerWakeupLib(uint8_t powerPin, uint32_t powerPulseMs, bool debug = true);

    // Initialize pins and load persisted configuration.
    // Loads persisted PC MAC and seeds default allowed controllers.
    bool begin();

    // Enable BT stack and listen for controller connection attempts.
    // Starts BT controller + Bluedroid and registers GAP callback.
    bool enableBluetooth();

    // Typo-compatible alias for callers already using this spelling.
    bool enableBluettoth() { return enableBluetooth(); }

    // Disable BT stack so it no longer competes for BT ownership.
    // Stops callback delivery and tears down BT stack ownership.
    bool disableBluetooth();

    // Persist and apply PC BT MAC (returns true only when value changes).
    bool updatePcMAC(const MacAddress &mac);

    // Add allowed controller if not present (returns true when added).
    bool addController(const MacAddress &mac);

    // Optional utility methods.
    bool removeController(const MacAddress &mac);
    void clearControllers();
    bool isControllerAllowed(const MacAddress &mac) const;

    // One-shot signal: true once per detected wake attempt.
    bool powerOn();

    // Optional event callback called once per wake attempt.
    void setWakeCallback(WakeCallback callback);

    // Kept for API compatibility; no-op when using pure wake detection mode.
    void process();

    bool isBluetoothEnabled() const;
    MacAddress getPcMAC() const;

private:
    static constexpr const char *NVS_NAMESPACE = "ctrlwake";
    static constexpr const char *NVS_KEY_PC_MAC = "pcmac";

    static void gapEventHandlerThunk(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    void handleGapEvent(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);

    bool ensureBtStackReady();
    bool applyBaseMacForPc();
    bool storePcMac();
    bool loadPcMac();

    void log(const char *msg) const;
    static void printMac(Stream &stream, const uint8_t *mac);

    bool debug_;
    bool initialized_;
    bool btEnabled_;

    volatile bool wakePending_;

    MacAddress pcMac_;
    std::vector<MacAddress> allowedControllers_;
    WakeCallback wakeCallback_;

    static ControllerWakeupLib *instance_;
};
