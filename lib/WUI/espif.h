#pragma once

#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

////////////////////////////////////////////////////////////////////////////
/// @brief Called from interrupt that data are ready in the DMA buffer
///        Setting the event
void espif_receive_data();

void espif_tx_callback();

////////////////////////////////////////////////////////////////////////////
/// @brief Initialize ESPIF (part of LwIP netif setup)
/// Can be used only for one interface
/// @param[in] netif Network interface to initialize
err_t espif_init(struct netif *netif);

////////////////////////////////////////////////////////////////////////////
/// @brief Join AP
/// @param [in] ssid AP SSID
/// @param [in] pass AP password
err_t espif_join_ap(const char *ssid, const char *passwd);

////////////////////////////////////////////////////////////////////////////
/// C wrapper for espif::scan::start
err_t espif_scan_start();

////////////////////////////////////////////////////////////////////////////
/// C wrapper for espif::scan::is_running
bool espif_scan_is_running();

////////////////////////////////////////////////////////////////////////////
/// C wrapper for espif::scan::stop
err_t espif_scan_stop();

////////////////////////////////////////////////////////////////////////////
/// C wrapper for espif::scan::get_ap_count
uint8_t espif_scan_get_ap_count();

////////////////////////////////////////////////////////////////////////////
/// C wrapper for espif::scan::get_ap_info
err_t espif_scan_get_ap_ssid(uint8_t ap_index, uint8_t *ssid_buffer, uint8_t buffer_len, bool *needs_password);

////////////////////////////////////////////////////////////////////////////
/// @brief Retrieve link status
/// This return true if associated to AP regardless of interface being up or down.
bool espif_link();

void espif_input_once(struct netif *netif);

void espif_reset_connection();

/// Perform periodic works of the esp interface.
///
/// Returns if there was any activity since last tick.
bool espif_tick();

/// Is the ESP if ready to receive info about an AP to connect to?
bool espif_need_ap();

/// Perform a reset of the ESP and bring it up again.
void espif_reset();

enum FlashResult {
    success = 0,
    not_connected = 1,
    failure = 2,
};

void espif_notify_flash_result(enum FlashResult result);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus

    #include <span>

enum class EspFwState {
    /// Detected an ESP firmware of the right version.
    Ok,
    /// Flashing failed due to ESP not being connected.
    FlashingErrorNotConnected,
    /// Flashing failed due to some other reason.
    FlashingErrorOther,
    /// Detected our ESP firmware, but of a wrong version.
    WrongVersion,
    /// The ESP doesn't speak our protocol ‒ it either isn't flashed or is
    /// flashed with some completely different (not our) firmware.
    NoFirmware,
    /// No communication from ESP detected. Either no ESP is present or it is
    /// completely silent (can it happen?)
    NoEsp,
    /// The ESP is scanning for available APs
    Scanning,
    /// The state is not currently (yet) known.
    Unknown,
};

namespace espif::scan {
/// @brief Sends a request to the esp over UART to start a wifi scan
///
/// If the esp is connected to AP the esp will temporarily disconnect from AP and after the scan is finished it will
/// auto reconnect.
/// The scan is done incrementally.
/// It starts with a short intervals to quickly find some APs.
/// The interval is doubled every time until the scan reaches ~30s scan time.
/// Found APs are stored in the esp memory until new scan is started.
/// To access the found aps use espif::scan::get_ap_info.
/// After every interval the esp sends the number of stored APs.
/// To get the number of found APs use espif::scan::get_ap_count.
[[nodiscard]] err_t start();
/// @brief Sends a stop request to esp over UART to stop the current wifi scan.
///
/// If the esp is scanning it will immediately stop the current scan segment and prevents from starting a new one.
/// The stopped scan still sends the results about found APs and new APs can be processed.
[[nodiscard]] err_t stop();
/// @brief Checks if the scan is running.
///
/// The check is done locally (on printer side, if the esp errors and resets,
/// then we will not have any information about it).
[[nodiscard]] bool is_running();
/// @brief Gets number of found aps.
///
/// This number is updated periodically by the esp and should be checked if new results are available.
[[nodiscard]] uint8_t get_ap_count();
/// @brief Retrieve ap info from esp.
///
/// Retrieves data from esp over UART.
/// This function blocks until the data are received (the responses are relarivelly quick - 0.8ms).
/// The parameter needs_password isn't 100% accurate since we know that the esp says that some APs doesn't need a
/// password even though they actually need it.
[[nodiscard]] err_t get_ap_info(uint8_t ap_index, std::span<uint8_t> ssid_buffer, bool &needs_password);
} // namespace espif::scan

/// Returns the current state of the ESP's firmware, as guessed by us.
///
/// Note that this may change. For example, this'll return NoEsp before a
/// communication from ESP happens, but eventually may reach eg. WrongVersion or
/// Ok. Similarly, if the ESP is force-reset (for various reasons), it may first
/// transition to NoEsp and then again reach Ok, or (due to flashing) go
/// WrongVersion -> Flashing -> NoEsp -> Ok.
EspFwState esp_fw_state();

enum class EspLinkState {
    /// Being initialized, flashed, wrong FW version... see esp_fw_state.
    Init,
    /// Not connected to an AP.
    NoAp,
    /// Up and running.
    Up,
    /// No communication from ESP for a while.
    ///
    /// Broken UART? ESP crashed?
    Silent,
};

EspLinkState esp_link_state();
#endif
