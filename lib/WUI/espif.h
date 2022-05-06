#pragma once

#include "stm32f4xx_hal.h"
#include "lwip/netif.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

////////////////////////////////////////////////////////////////////////////
/// @brief Called from interrupt that data are ready in the DMA buffer
///        Setting the event
/// @param[in] device UART device
void espif_receive_data(UART_HandleTypeDef *);

////////////////////////////////////////////////////////////////////////////
/// @brief Initialize ESP for flash write
err_t espif_flash_initialize();

////////////////////////////////////////////////////////////////////////////
/// @brief Return to normal omode
void espif_flash_deinitialize();

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
/// @brief Retrieve link status
/// This return true if associated to AP regardless of interface being up or down.
bool espif_link();

void espif_input_once(struct netif *netif);

/// Perform periodic works of the esp interface.
///
/// Returns if there was any activity since last tick.
bool espif_tick();

/// Is the ESP if ready to receive info about an AP to connect to?
bool espif_need_ap();

/// Perform a reset of the ESP and bring it up again.
void espif_reset();

// UART buffer stuff
// The data received should fit into the buffer. Or, some guaratees has to be
// provided to ensure the excessive data can be copied from the RX buffer
// before the buffer overflows.
#define RX_BUFFER_LEN 0x1000

extern uint8_t dma_buffer_rx[RX_BUFFER_LEN];

#ifdef __cplusplus
}
#endif /* __cplusplus */

#ifdef __cplusplus
enum class EspFwState {
    /// Detected an ESP firmware of the right version.
    Ok,
    /// Detected our ESP firmware, but of a wrong version.
    WrongVersion,
    /// The ESP doesn't speak our protocol ‒ it either isn't flashed or is
    /// flashed with some completely different (not our) firmware.
    NoFirmware,
    /// No communication from ESP detected. Either no ESP is present or it is
    /// completely silent (can it happen?)
    NoEsp,
    /// The ESP is being flashed right now.
    Flashing,
};

/// Returns the current state of the ESP's firmware, as guessed by us.
///
/// Note that this may change. For example, this'll return NoEsp before a
/// communication from ESP happens, but eventually may reach eg. WrongVersion or
/// Ok. Similarly, if the ESP is force-reset (for various reasons), it may first
/// transition to NoEsp and then again reach Ok, or (due to flashing) go
/// WrongVersion -> Flashing -> NoEsp -> Ok.
EspFwState esp_fw_state();
#endif
