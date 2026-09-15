/// @file
#pragma once

#include <atomic>
#include <cstdint>
#include "esp_wifi_types.h"

inline constexpr uint8_t FW_VERSION = 13;

inline constexpr uint8_t SCAN_MAX_STORED_SSIDS = 64;
inline constexpr size_t SSID_MAX_LEN = 32;
inline constexpr size_t BSSID_MAX_LEN = 6;

// Hack: because we don't see the beacon on some networks (and it's quite
// common), but don't want to be "flapping", we set the timeout for beacon
// inactivity to a ridiculously long time and handle the disconnect ourselves.
//
// It's not longer for the only reason the uint16_t doesn't hold as big numbers.
inline constexpr uint16_t INACTIVE_BEACON_SECONDS = 3600 * 18;

// This is the effective timeout. If we don't receive any packet for this long,
// we consider the signal lost.
//
// TODO: Shall we generate something to provoke getting some packets? Like ARP
// pings to the AP?
inline constexpr uint32_t INACTIVE_PACKET_SECONDS = 5;

// Note: Values 1..5 are deprecated and must not be used.
enum class MsgType : uint8_t {
    devinfo = 0,
    client_config = 6,
    packet = 7,
    scan_start = 8,
    scan_stop = 9,
    scan_ap_count = 10,
    scan_ap_get = 11
};

struct __attribute__((packed)) Header {
    uint8_t type;
    union {
        uint8_t version; // when type == MsgType::devinfo
        uint8_t unused; // when type == MsgType::client_config || type == MsgType::scan_start || type == MsgType::scan_stop
        int8_t link_status; // when type == MsgType::packet
        uint8_t ap_count; // when type == MsgType::scan_ap_count
        uint8_t ap_index; // when type == MsgType::scan_ap_get
    };
    uint16_t size;
};

inline constexpr uint8_t UART_NIC_PROTOCOL = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N;

inline constexpr const char *TAG = "uart_nic";

inline constexpr size_t MAC_LEN = 6;
inline uint8_t mac[MAC_LEN];

uint32_t now_seconds();

inline std::atomic_uint_least32_t last_inbound_seen = 0;
inline std::atomic_bool associated = false;
inline std::atomic_bool wifi_running = false;
inline std::atomic_bool connecting = false;

inline bool beacon_quirk;
inline uint8_t probe_max_reties = 3;
inline std::atomic_bool probe_in_progress = false;
inline uint8_t probe_retry_count;
inline uint8_t latest_ssid[SSID_MAX_LEN];
inline uint8_t latest_bssid[BSSID_MAX_LEN];
inline int s_retry_num = 0;

enum class ScanType {
    unknown,
    probe,
    incremental
};

struct __attribute__((packed)) ScanResult {
    uint8_t ssid[SSID_MAX_LEN];
    bool needs_password;
};

inline constexpr ScanResult EMPTY_RESULT = {};

typedef void (*WifiScanCallback)(wifi_ap_record_t *, int);

struct ScanState {
    ScanType scan_type;
    WifiScanCallback callback;
    uint32_t incremental_scan_time;
    ScanResult stored_ssids[SCAN_MAX_STORED_SSIDS];
    uint8_t stored_ssids_count;
    std::atomic_bool in_progress;
    std::atomic_bool should_reconnect;
};

inline ScanState scan {
    .scan_type = ScanType::unknown,
    .callback = nullptr,
    .incremental_scan_time = 0,
    .stored_ssids = {},
    .stored_ssids_count = 0,
    .in_progress = false,
    .should_reconnect = false,
};

void do_wifi_scan(void *arg);
esp_err_t start_wifi_scan(WifiScanCallback callback, ScanType scan_type);
esp_err_t force_stop_wifi_scan();
void cache_current_ap_info();
void wifi_re_connect_task(void *args);
void start_wifi_connect_task();
void probe_handler(wifi_ap_record_t *aps, int ap_count);
void probe_run();
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void check_online_status();
void clear_stored_ssids();
void handle_rx_msg_scan_start(uint8_t *data, Header header);
void handle_rx_msg_scan_stop(uint8_t *data, Header header);
void handle_rx_msg_scan_get(uint8_t *data, Header header);
void store_scanned_ssids(wifi_ap_record_t *aps, int ap_count);
