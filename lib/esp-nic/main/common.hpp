/// @file
#pragma once

#include <atomic>
#include <cstdint>
#include "esp_wifi_types.h"

#define FW_VERSION 13

#define SCAN_MAX_STORED_SSIDS 64
#define SSID_LEN              32
#define BSSID_LEN             6

// Hack: because we don't see the beacon on some networks (and it's quite
// common), but don't want to be "flapping", we set the timeout for beacon
// inactivity to a ridiculously long time and handle the disconnect ourselves.
//
// It's not longer for the only reason the uint16_t doesn't hold as big numbers.
static const uint16_t INACTIVE_BEACON_SECONDS = 3600 * 18;
// This is the effective timeout. If we don't receive any packet for this long,
// we consider the signal lost.
//
// TODO: Shall we generate something to provoke getting some packets? Like ARP
// pings to the AP?
static const uint32_t INACTIVE_PACKET_SECONDS = 5;

// Note: Values 1..5 are deprecated and must not be used.
#define MSG_DEVINFO_V2      0
#define MSG_CLIENTCONFIG_V2 6
#define MSG_PACKET_V2       7
#define MSG_SCAN_START      8
#define MSG_SCAN_STOP       9
#define MSG_SCAN_AP_CNT     10
#define MSG_SCAN_AP_GET     11

struct __attribute__((packed)) header {
    uint8_t type;
    union {
        uint8_t version; // when type == MSG_DEVINFO_V2
        uint8_t unused; // when type == MSG_CLIENTCONFIG_V2 || type == MSG_SCAN_START || type == MSG_SCAN_STOP
        int8_t link_status; // when type == MSG_PACKET_V2
        uint8_t ap_count; // when type == MSG_SCAN_AP_GET
        uint8_t ap_index; // when type == MSG_SCAN_AP_GET
    };
    uint16_t size;
};

static const uint8_t uart_nic_protocol = WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N;

static const char *TAG = "uart_nic";

#define MAC_LEN 6
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
inline uint8_t latest_ssid[SSID_LEN];
inline uint8_t latest_bssid[BSSID_LEN];
inline int s_retry_num = 0;

typedef enum {
    SCAN_TYPE_UNKNOWN,
    SCAN_TYPE_PROBE,
    SCAN_TYPE_INCREMENTAL,
} ScanType;

typedef struct __attribute__((packed)) {
    uint8_t ssid[SSID_LEN];
    bool needs_password;
} ScanResult;

const ScanResult EMPTY_RESULT = {};

typedef void (*wifi_scan_callback)(wifi_ap_record_t *, int);

typedef struct {
    ScanType scan_type;
    wifi_scan_callback callback;
    uint32_t incremental_scan_time;
    ScanResult stored_ssids[SCAN_MAX_STORED_SSIDS];
    uint8_t stored_ssids_count;
    std::atomic_bool in_progress;
    std::atomic_bool should_reconnect;
} ScanState;

inline ScanState scan {
    .scan_type = SCAN_TYPE_UNKNOWN,
    .callback = NULL,
    .incremental_scan_time = 0,
    .stored_ssids = {},
    .stored_ssids_count = 0,
    .in_progress = false,
    .should_reconnect = false,
};

void do_wifi_scan(void *arg);
esp_err_t start_wifi_scan(wifi_scan_callback callback, ScanType scan_type);
esp_err_t force_stop_wifi_scan();
void cache_current_ap_info();
void wifi_re_connect_task(void *args);
void start_wifi_connect_task();
void probe_handler(wifi_ap_record_t *aps, int ap_count);
void probe_run();
void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data);
void check_online_status();
void clear_stored_ssids();
void handle_rx_msg_scan_start(uint8_t *data, struct header header);
void handle_rx_msg_scan_stop(uint8_t *data, struct header header);
void handle_rx_msg_scan_get(uint8_t *data, struct header header);
void store_scanned_ssids(wifi_ap_record_t *aps, int ap_count);
