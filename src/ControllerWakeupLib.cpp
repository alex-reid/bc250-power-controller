#include "ControllerWakeupLib.h"

#include "esp_bt.h"
#include "esp_bt_main.h"
#include "esp_gap_bt_api.h"
#include "esp_err.h"
#include "esp_system.h"
#include "esp32-hal-bt.h"
#include <Preferences.h>

ControllerWakeupLib *ControllerWakeupLib::instance_ = nullptr;

namespace {
constexpr MacAddress kDefaultPcMac = {{0, 0, 0, 0, 0, 0}}; //{{0xAC, 0xA7, 0xF1, 0xE4, 0x55, 0xB9}};
constexpr MacAddress kDefaultPs5Mac = {{0, 0, 0, 0, 0, 0}}; //{{0x0C, 0x27, 0x56, 0x3D, 0xB1, 0x44}};
constexpr MacAddress kDefaultSwitchProMac = {{0, 0, 0, 0, 0, 0}}; //{{0xD0, 0x55, 0x09, 0x29, 0xCB, 0x92}};

void macMinusOffset(const uint8_t *inMac, uint8_t *outMac, uint8_t offset) {
    // ESP32 BT MAC is base MAC + 2, so derive the required base MAC.
    for (int i = 0; i < 6; ++i) {
        outMac[i] = inMac[i];
    }

    int borrow = offset;
    for (int i = 5; i >= 0 && borrow > 0; --i) {
        int val = static_cast<int>(outMac[i]) - (borrow & 0xFF);
        if (val < 0) {
            outMac[i] = static_cast<uint8_t>(val + 256);
            borrow = 1;
        } else {
            outMac[i] = static_cast<uint8_t>(val);
            borrow = 0;
        }
    }
}
} // namespace

ControllerWakeupLib::ControllerWakeupLib(bool debug)
    : debug_(debug),
      initialized_(false),
      btEnabled_(false),
      wakePending_(false),
      pcMac_(kDefaultPcMac),
      wakeCallback_(nullptr) {
}

ControllerWakeupLib::ControllerWakeupLib(uint8_t powerPin, uint32_t powerPulseMs, bool debug)
    : ControllerWakeupLib(debug) {
    (void)powerPin;
    (void)powerPulseMs;
}

bool ControllerWakeupLib::begin() {
    // Try persisted PC MAC first; write default value on first boot.
    if (!loadPcMac()) {
        pcMac_ = kDefaultPcMac;
        if (!storePcMac()) {
            return false;
        }
    }

    // Seed common controller MACs if user has not populated a custom list.
    if (allowedControllers_.empty()) {
        allowedControllers_.push_back(kDefaultPs5Mac);
        allowedControllers_.push_back(kDefaultSwitchProMac);
    }

    initialized_ = true;
    return true;
}

bool ControllerWakeupLib::enableBluetooth() {
    if (btEnabled_) {
        return true;
    }

    if (!initialized_ && !begin()) {
        return false;
    }

    if (!applyBaseMacForPc()) {
        return false;
    }

    if (!ensureBtStackReady()) {
        return false;
    }

    // Static thunk forwards BT GAP events to this instance.
    instance_ = this;

    esp_err_t err = esp_bt_gap_register_callback(gapEventHandlerThunk);
    if (err != ESP_OK) {
        log("[BT] esp_bt_gap_register_callback failed");
        return false;
    }

    err = esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
    if (err != ESP_OK) {
        log("[BT] esp_bt_gap_set_scan_mode failed");
        return false;
    }

    btEnabled_ = true;
    log("[BT] Enabled and listening for controller wake attempts");
    return true;
}

bool ControllerWakeupLib::disableBluetooth() {
    if (!btEnabled_) {
        return true;
    }

    esp_err_t err;

    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED) {
        err = esp_bluedroid_disable();
        if (err != ESP_OK) {
            log("[BT] esp_bluedroid_disable failed");
            return false;
        }
    }

    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_INITIALIZED) {
        err = esp_bluedroid_deinit();
        if (err != ESP_OK) {
            log("[BT] esp_bluedroid_deinit failed");
            return false;
        }
    }

    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) {
        err = esp_bt_controller_disable();
        if (err != ESP_OK) {
            log("[BT] esp_bt_controller_disable failed");
            return false;
        }
    }

    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED) {
        err = esp_bt_controller_deinit();
        if (err != ESP_OK) {
            log("[BT] esp_bt_controller_deinit failed");
            return false;
        }
    }

    // Clear runtime state so next enable starts from a clean edge-trigger state.
    btEnabled_ = false;
    instance_ = nullptr;
    wakePending_ = false;
    log("[BT] Disabled");
    return true;
}

bool ControllerWakeupLib::updatePcMAC(const MacAddress &mac) {
    if (!initialized_ && !begin()) {
        return false;
    }

    if (mac == pcMac_) {
        return false;
    }

    pcMac_ = mac;
    if (!storePcMac()) {
        return false;
    }

    // Base MAC can only be set while controller is idle. Restart BT if currently active.
    if (btEnabled_) {
        if (!disableBluetooth()) {
            return false;
        }
        if (!enableBluetooth()) {
            return false;
        }
    }

    return true;
}

bool ControllerWakeupLib::addController(const MacAddress &mac) {
    for (const auto &allowed : allowedControllers_) {
        if (allowed == mac) {
            return false;
        }
    }

    allowedControllers_.push_back(mac);
    return true;
}

bool ControllerWakeupLib::removeController(const MacAddress &mac) {
    for (auto it = allowedControllers_.begin(); it != allowedControllers_.end(); ++it) {
        if (*it == mac) {
            allowedControllers_.erase(it);
            return true;
        }
    }
    return false;
}

void ControllerWakeupLib::clearControllers() {
    allowedControllers_.clear();
}

bool ControllerWakeupLib::isControllerAllowed(const MacAddress &mac) const {
    for (const auto &allowed : allowedControllers_) {
        if (allowed == mac) {
            return true;
        }
    }
    return false;
}

bool ControllerWakeupLib::powerOn() {
    if (wakePending_) {
        wakePending_ = false;
        return true;
    }
    return false;
}

void ControllerWakeupLib::setWakeCallback(WakeCallback callback) {
    wakeCallback_ = callback;
}

void ControllerWakeupLib::process() {
    // No-op in pure wake detection mode.
}

bool ControllerWakeupLib::isBluetoothEnabled() const {
    return btEnabled_;
}

MacAddress ControllerWakeupLib::getPcMAC() const {
    return pcMac_;
}

void ControllerWakeupLib::gapEventHandlerThunk(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    if (instance_ != nullptr) {
        instance_->handleGapEvent(event, param);
    }
}

void ControllerWakeupLib::handleGapEvent(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param) {
    if (event == ESP_BT_GAP_ACL_DISCONN_CMPL_STAT_EVT) {
        if (debug_) {
            Serial.print("[BT] ACL disconnect, reason=");
            Serial.println(static_cast<int>(param->acl_disconn_cmpl_stat.reason));
        }
        return;
    }

    if (event != ESP_BT_GAP_ACL_CONN_CMPL_STAT_EVT) {
        return;
    }

    // Read incoming controller address from ACL completion payload.
    MacAddress incoming = {{0}};
    memcpy(incoming.bytes, param->acl_conn_cmpl_stat.bda, sizeof(incoming.bytes));

    if (debug_) {
        Serial.print("[BT] ACL connect attempt, status=");
        Serial.print(static_cast<int>(param->acl_conn_cmpl_stat.stat));
        Serial.print(", mac=");
        printMac(Serial, incoming.bytes);
        Serial.println();
    }

    if (!isControllerAllowed(incoming)) {
        if (debug_) {
            Serial.print("[BT] Rejected controller (not in allow list): ");
            printMac(Serial, incoming.bytes);
            Serial.println();
        }
        return;
    }

    // Mark one-shot wake event before user callback for polling consumers.
    wakePending_ = true;

    if (debug_ && param->acl_conn_cmpl_stat.stat != 0) {
        Serial.println("[BT] Wake triggered from connection attempt (non-success status)");
    }

    if (wakeCallback_ != nullptr) {
        wakeCallback_(incoming);
    }

    if (debug_) {
        Serial.print("[BT] Allowed controller wake from ");
        printMac(Serial, incoming.bytes);
        Serial.println();
    }
}

bool ControllerWakeupLib::ensureBtStackReady() {
    esp_err_t err;

    // Force-link Arduino BT HAL object, preventing BT memory release at startup.
    (void)btStarted();

    // Bring each BT layer up only when needed so repeated enable calls are safe.
    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_IDLE) {
        esp_bt_controller_config_t cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();
        err = esp_bt_controller_init(&cfg);
        if (err != ESP_OK) {
            log("[BT] esp_bt_controller_init failed");
            return false;
        }
    }

    if (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_INITED) {
        err = esp_bt_controller_enable(ESP_BT_MODE_CLASSIC_BT);
        if (err != ESP_OK) {
            log("[BT] esp_bt_controller_enable failed");
            return false;
        }
    }

    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_UNINITIALIZED) {
        err = esp_bluedroid_init();
        if (err != ESP_OK) {
            log("[BT] esp_bluedroid_init failed");
            return false;
        }
    }

    if (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_INITIALIZED) {
        err = esp_bluedroid_enable();
        if (err != ESP_OK) {
            log("[BT] esp_bluedroid_enable failed");
            return false;
        }
    }

    return (esp_bt_controller_get_status() == ESP_BT_CONTROLLER_STATUS_ENABLED) &&
           (esp_bluedroid_get_status() == ESP_BLUEDROID_STATUS_ENABLED);
}

bool ControllerWakeupLib::applyBaseMacForPc() {
    // Base MAC can only be applied when the controller is idle.
    if (esp_bt_controller_get_status() != ESP_BT_CONTROLLER_STATUS_IDLE) {
        return true;
    }

    uint8_t baseMacForBt[6];
    macMinusOffset(pcMac_.bytes, baseMacForBt, 2);
    esp_err_t err = esp_base_mac_addr_set(baseMacForBt);
    if (err != ESP_OK) {
        log("[CFG] esp_base_mac_addr_set failed");
        return false;
    }

    return true;
}

bool ControllerWakeupLib::storePcMac() {
    // Persist only the spoofed PC MAC; controller allow list is runtime-managed.
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, false)) {
        log("[NVS] open failed");
        return false;
    }

    size_t written = prefs.putBytes(NVS_KEY_PC_MAC, pcMac_.bytes, sizeof(pcMac_.bytes));
    prefs.end();
    if (written != sizeof(pcMac_.bytes)) {
        log("[NVS] write failed");
        return false;
    }

    return true;
}

bool ControllerWakeupLib::loadPcMac() {
    Preferences prefs;
    if (!prefs.begin(NVS_NAMESPACE, true)) {
        log("[NVS] open readonly failed");
        return false;
    }

    // Length guard avoids reading partially written/legacy values.
    size_t len = prefs.getBytesLength(NVS_KEY_PC_MAC);
    if (len != sizeof(pcMac_.bytes)) {
        prefs.end();
        return false;
    }

    size_t read = prefs.getBytes(NVS_KEY_PC_MAC, pcMac_.bytes, sizeof(pcMac_.bytes));
    prefs.end();
    return read == sizeof(pcMac_.bytes);
}

void ControllerWakeupLib::log(const char *msg) const {
    if (debug_) {
        Serial.println(msg);
    }
}

void ControllerWakeupLib::printMac(Stream &stream, const uint8_t *mac) {
    char buf[18];
    snprintf(buf, sizeof(buf), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    stream.print(buf);
}
