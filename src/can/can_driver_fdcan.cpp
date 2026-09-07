#include "can_driver_fdcan.hpp"

#include <device/peripherals.hpp>
#include <bsod.h>
#include <timing.h>

namespace can {

namespace {

    /**
     * @brief Get full FDCAN_Tx_location mask.
     * Different for each MCU and there is only a check macro for it.
     * @return full mask
     */
    consteval uint32_t full_tx_location_mask() {
        uint32_t mask = 0x00000000;
        for (int i = 0; i < 32; i++) {
            if (IS_FDCAN_TX_LOCATION(1 << i)) {
                mask |= 1 << i;
            }
        }
        return mask;
    }
} // namespace

FdcanDriver *FdcanDriver::list = nullptr;

void FdcanDriver::update_error_log(uint32_t add) {
    FDCAN_ErrorCountersTypeDef error_counters;
    if (auto ret = HAL_FDCAN_GetErrorCounters(&hfdcan, &error_counters); ret != HAL_OK) {
        bsod("CAN HAL error stats %i,%i", static_cast<int>(ret), static_cast<int>(hfdcan.ErrorCode));
    }
    error_log += (error_counters.ErrorLogging + add);
}

FdcanDriver &FdcanDriver::get_driver(FDCAN_HandleTypeDef *hfdcan_isr) {
    FdcanDriver *driver = list; // Get first driver in the list
    while (driver != nullptr && &driver->hfdcan != hfdcan_isr) { // Find one that matches
        driver = driver->next;
    }

    // Interrupts are enabled only for matching drivers
    /// @note If another FDCAN uses interrupts and doesn't use FdcanDriver, this needs to be changed.
    debug_assert(driver != nullptr);

    return *driver;
}

void FdcanDriver::start(bool automatic_retransmission_enable) {
    // Enable interrupts
    if (HAL_FDCAN_ActivateNotification(
            &hfdcan,
            FDCAN_IT_RX_FIFO0_NEW_MESSAGE
                | FDCAN_IT_RX_FIFO1_NEW_MESSAGE
                | FDCAN_IT_TX_EVT_FIFO_NEW_DATA
                | FDCAN_IT_TX_COMPLETE
                | FDCAN_IT_RX_HIGH_PRIORITY_MSG
                | FDCAN_IT_RX_FIFO0_MESSAGE_LOST
                | FDCAN_IT_RX_FIFO1_MESSAGE_LOST
                | FDCAN_IT_BUS_OFF
                | FDCAN_IT_ERROR_PASSIVE
                | FDCAN_IT_ERROR_WARNING
                | FDCAN_IT_ERROR_LOGGING_OVERFLOW,
            full_tx_location_mask())
        != HAL_OK) {
        debug_assert(false);
    }

    // 29 bit messages to FIFO 0, 11 bit and remote messages discard
    if (HAL_FDCAN_ConfigGlobalFilter(&hfdcan,
            FDCAN_REJECT,
            FDCAN_ACCEPT_IN_RX_FIFO0,
            FDCAN_REJECT_REMOTE,
            FDCAN_REJECT_REMOTE)
        != HAL_OK) {
        debug_assert(false);
    }

    // Set automatic retransmission of frames that are not acked or fail to transmit
    if (automatic_retransmission_enable) {
        hfdcan.Instance->CCCR &= ~FDCAN_CCCR_DAR; // Enable automatic retransmission
    } else {
        hfdcan.Instance->CCCR |= FDCAN_CCCR_DAR; // Disable automatic retransmission
    }

    // Start CAN HAL
    if (HAL_FDCAN_Start(&hfdcan) != HAL_OK) {
        debug_assert(false);
    }
}

void FdcanDriver::set_automatic_retransmission(bool enable) {
    // Stop the CAN peripheral to get to INIT state where configuration can be changed
    if (HAL_FDCAN_Stop(&hfdcan) != HAL_OK) {
        debug_assert(false);
    }

    // Set automatic retransmission of frames that are not acked or fail to transmit
    if (enable) {
        hfdcan.Instance->CCCR &= ~FDCAN_CCCR_DAR; // Enable automatic retransmission
    } else {
        hfdcan.Instance->CCCR |= FDCAN_CCCR_DAR; // Disable automatic retransmission
    }

    // Start the CAN peripheral again
    if (HAL_FDCAN_Start(&hfdcan) != HAL_OK) {
        debug_assert(false);
    }
}

bool FdcanDriver::send(const CanardFrame &frame, bool store_timestamp) {
    // Check maximal length
    debug_assert(frame.payload_size <= 64);

    // Check that the allocated buffer is rounded up, otherwise HAL would access invalid memory
    debug_assert(frame.payload_size == CanardCANDLCToLength[CanardCANLengthToDLC[frame.payload_size]]);

    // If this is the last remaining hardware queue slot, use it only if it has higher priority than last frame
    if (uint32_t free_level = HAL_FDCAN_GetTxFifoFreeLevel(&hfdcan); free_level == 0
        || (free_level == 1 && frame.extended_can_id >= last_tx_priority)) {
        return false; // Wait and try next time
    }

    // HAL Tx header
    static FDCAN_TxHeaderTypeDef TxHeader = {
        .Identifier = UINT32_MAX,
        .IdType = FDCAN_EXTENDED_ID,
        .TxFrameType = FDCAN_DATA_FRAME,
        .DataLength = 0,
        .ErrorStateIndicator = FDCAN_ESI_ACTIVE,
        .BitRateSwitch = enable_bit_rate_switch ? FDCAN_BRS_ON : FDCAN_BRS_OFF,
        .FDFormat = FDCAN_FD_CAN,
        .TxEventFifoControl = FDCAN_NO_TX_EVENTS,
        .MessageMarker = 0
    };
    TxHeader.Identifier = frame.extended_can_id;
    TxHeader.DataLength = static_cast<uint32_t>(CanardCANLengthToDLC[frame.payload_size]) * FDCAN_DLC_BYTES_1;

    // Add marker to get interrupt on Tx event and store timestamp
    if (store_timestamp) {
        TxHeader.TxEventFifoControl = FDCAN_STORE_TX_EVENTS;
        uint8_t to_write = (tx_timestamp_to_write.exchange(0) + 1) & 0xff; // Clear what to write and increment, to prevent race
        debug_assert(to_write != 0); // Wrapped around without getting one timestamp
        TxHeader.MessageMarker = to_write;
        tx_timestamp_to_write = to_write;
    } else {
        TxHeader.TxEventFifoControl = FDCAN_NO_TX_EVENTS;
        TxHeader.MessageMarker = 0;
    }

    // Send the frame
    uint8_t *payload = const_cast<uint8_t *>(reinterpret_cast<const uint8_t *>(frame.payload)); // Stupid HAL cannot use const where it should
    if (auto ret = HAL_FDCAN_AddMessageToTxFifoQ(&hfdcan, &TxHeader, payload); ret != HAL_OK) {
        if (hfdcan.ErrorCode & HAL_FDCAN_ERROR_FIFO_FULL) {
            hfdcan.ErrorCode &= ~HAL_FDCAN_ERROR_FIFO_FULL; // The queue is full, we will try again on the next iteration.
        }
        if (hfdcan.ErrorCode) {
            bsod("CAN HAL Tx failed %i,%i", static_cast<int>(ret), static_cast<int>(hfdcan.ErrorCode));
        }

        update_error_log();

        return false; // Needs to be sent next time
    } else {
        last_tx_priority = frame.extended_can_id; // Last ID put to hardware queue
        return true; // Sent
    }
}

CanardMicrosecond FdcanDriver::sanitize_timestamp(int64_t time_isr, uint32_t frame_timestamp) {
    const uint32_t sub_overflow = time_isr % TIM3_OVERFLOW; // Part of the timestamp that corresponds to TIM3 and hardware sampled frame_timestamp
    if (frame_timestamp > sub_overflow) { // Now is in past from the event
        // Timestamp crossed TIM3 boundary, we need to subtract the overflow
        time_isr -= TIM3_OVERFLOW;
    }
    return time_isr - sub_overflow + frame_timestamp;
}

/**
 * @brief  Tx Event callback.
 * @param  hfdcan pointer to an FDCAN_HandleTypeDef structure that contains
 *         the configuration information for the specified FDCAN.
 * @param  TxEventFifoITs indicates which Tx Event FIFO interrupts are signalled.
 *         This parameter can be any combination of @arg FDCAN_Tx_Event_Fifo_Interrupts.
 * @retval None
 */
extern "C" void HAL_FDCAN_TxEventFifoCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t TxEventFifoITs) {
    // First, get timestamp as close to the event as possible
    ///@warning CAN needs to have lower ISR priority than tick timer to be able to read timestamps from ISR.
    ///@note We need to be maximally TIM3_OVERFLOW microseconds away from the event to be able to fit it.
    int64_t time_isr = get_timestamp_us();

    // Get driver instance
    FdcanDriver::get_driver(hfdcan).tx_event_callback(time_isr, TxEventFifoITs);
}

void FdcanDriver::tx_event_callback(int64_t time_isr, uint32_t TxEventFifoITs) {
    // Get tx event
    FDCAN_TxEventFifoTypeDef tx_event;
    if (HAL_FDCAN_GetTxEvent(&hfdcan, &tx_event) == HAL_OK // Get event
        && tx_event.MessageMarker > 0) { // Only store timestamp for these markers

        // Store the timestamp
        if (tx_timestamp_to_write.load() == tx_event.MessageMarker) { // Store timestamp for this frame
            if (TxEventFifoITs & (FDCAN_IR_TEFL | FDCAN_IR_TEFF)) { // Tx event fifo full or element lost
                tx_timestamp = 0;
            } else if (TxEventFifoITs & FDCAN_IR_TEFN) {
                tx_timestamp = sanitize_timestamp(time_isr, tx_event.TxTimestamp);
            }
            tx_timestamp_to_write = 0; // Confirm which buffer was written
            /// @note This is run from ISR, so take load and write of tx_timestamp_to_write as atomic.
        }
    }
}

std::optional<CanardMicrosecond> FdcanDriver::get_sent_timestamp() {
    if (tx_timestamp_to_write.load() != 0) {
        return std::nullopt;
    }
    return tx_timestamp;
}

/**
 * @brief  Rx FIFO 0 callback.
 * @param  hfdcan pointer to an FDCAN_HandleTypeDef structure that contains
 *         the configuration information for the specified FDCAN.
 * @param  RxFifo0ITs indicates which Rx FIFO 0 interrupts are signalled.
 *         This parameter can be any combination of @arg FDCAN_Rx_Fifo0_Interrupts.
 * @retval None
 */
extern "C" void HAL_FDCAN_RxFifo0Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo0ITs) {
    auto &driver = FdcanDriver::get_driver(hfdcan); // Get driver instance
    if (RxFifo0ITs & FDCAN_IR_RF0N) { // New Rx message
        driver.isr_notify(Driver::Notification::RxDone); // Notify driver user
    }
    if (RxFifo0ITs & FDCAN_IR_RF0L) { // Message lost
        driver.isr_notify(Driver::Notification::RxLost); // Notify driver user
    }
}

/**
 * @brief  Rx FIFO 1 callback.
 * @param  hfdcan pointer to an FDCAN_HandleTypeDef structure that contains
 *         the configuration information for the specified FDCAN.
 * @param  RxFifo1ITs indicates which Rx FIFO 1 interrupts are signalled.
 *         This parameter can be any combination of @arg FDCAN_Rx_Fifo1_Interrupts.
 * @retval None
 */
extern "C" void HAL_FDCAN_RxFifo1Callback(FDCAN_HandleTypeDef *hfdcan, uint32_t RxFifo1ITs) {
    // First, get timestamp as close to the event as possible
    ///@warning CAN needs to have lower ISR priority than tick timer to be able to read timestamps from ISR.
    ///@note We need to be maximally TIM_OVERFLOW microseconds away from the event to be able to fit it.
    int64_t time_isr = get_timestamp_us();

    auto &driver = FdcanDriver::get_driver(hfdcan); // Get driver instance
    if (RxFifo1ITs & FDCAN_IR_RF1N) { // New Rx message
        driver.rx_fifo1_sample_time(time_isr); // Handle timing
        driver.isr_notify(Driver::Notification::RxDone); // Notify driver user
    }
    if (RxFifo1ITs & FDCAN_IR_RF1L) { // Message lost
        driver.isr_notify(Driver::Notification::RxLost); // Notify driver user
    }
}

void FdcanDriver::rx_fifo1_sample_time(int64_t time_isr) {
    if (rx1_timestamp_valid.load() == false) {
        rx1_timestamp = time_isr;
        rx1_timestamp_valid = true;
    } // else overflow, this message won't have timestamp
}

/**
 * @brief  Transmission Complete callback.
 * @param  hfdcan pointer to an FDCAN_HandleTypeDef structure that contains
 *         the configuration information for the specified FDCAN.
 * @param  BufferIndexes Indexes of the transmitted buffers.
 *         This parameter can be any combination of @arg FDCAN_Tx_location.
 * @retval None
 */
extern "C" void HAL_FDCAN_TxBufferCompleteCallback(FDCAN_HandleTypeDef *hfdcan, [[maybe_unused]] uint32_t BufferIndexes) {
    FdcanDriver::get_driver(hfdcan).isr_notify(Driver::Notification::TxDone); // Notify driver user
}

/**
 * @brief  High Priority Message callback.
 * @param  hfdcan pointer to an FDCAN_HandleTypeDef structure that contains
 *         the configuration information for the specified FDCAN.
 * @retval None
 */
extern "C" void HAL_FDCAN_HighPriorityMessageCallback(FDCAN_HandleTypeDef *hfdcan) {
    FdcanDriver::get_driver(hfdcan).isr_notify(Driver::Notification::RxHighPrio); // Notify driver user
}

bool FdcanDriver::receive(CanardFrame &frame, std::array<uint8_t, CANARD_MTU_CAN_FD> &rx_buffer, CanardMicrosecond *timestamp_us) {
    FDCAN_RxHeaderTypeDef RxHeader;

    // Try FIFO1, higher priority, with timestamp
    if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan, FDCAN_RX_FIFO1) > 0) {
        if (auto ret = HAL_FDCAN_GetRxMessage(&hfdcan, FDCAN_RX_FIFO1, &RxHeader, rx_buffer.data()); ret != HAL_OK) {
            bsod("CAN HAL Rx failed %i,%i", static_cast<int>(ret), static_cast<int>(hfdcan.ErrorCode));
        }

        if (timestamp_us) {
            if (rx1_timestamp_valid.exchange(false)) {
                *timestamp_us = sanitize_timestamp(rx1_timestamp, RxHeader.RxTimestamp);
            } else {
                *timestamp_us = 0; // No timestamp
            }
        }
    } else if (HAL_FDCAN_GetRxFifoFillLevel(&hfdcan, FDCAN_RX_FIFO0) > 0) { // Try FIFO0
        if (auto ret = HAL_FDCAN_GetRxMessage(&hfdcan, FDCAN_RX_FIFO0, &RxHeader, rx_buffer.data()); ret != HAL_OK) {
            bsod("CAN HAL Rx failed %i,%i", static_cast<int>(ret), static_cast<int>(hfdcan.ErrorCode));
        }

        if (timestamp_us) {
            *timestamp_us = get_timestamp_us(); // Approximate timestamp
        }
    } else {
        update_error_log();
        return false; // No frame in the FIFOs
    }

    frame.extended_can_id = RxHeader.Identifier;
    frame.payload_size = CanardCANDLCToLength[RxHeader.DataLength / FDCAN_DLC_BYTES_1];
    frame.payload = rx_buffer.data();

    return true;
}

void FdcanDriver::set_filter(uint32_t index, const CanardFilter &filter, bool timestamp, bool high_prio) {
    uint32_t config = FDCAN_FILTER_DISABLE;
    if (timestamp && high_prio) {
        config = FDCAN_FILTER_TO_RXFIFO1_HP;
    } else if (timestamp) {
        config = FDCAN_FILTER_TO_RXFIFO1;
    } else if (high_prio) {
        config = FDCAN_FILTER_TO_RXFIFO0_HP;
    } // else Disable filter

    FDCAN_FilterTypeDef hal_filter = {
        .IdType = FDCAN_EXTENDED_ID,
        .FilterIndex = index,
        .FilterType = FDCAN_FILTER_MASK,
        .FilterConfig = config,
        .FilterID1 = filter.extended_can_id,
        .FilterID2 = filter.extended_mask,
#if STM32MP15x || STM32MP25x
        // #error dead code found by automatic analyses (see BFW-5461)
        .RxBufferIndex = 0,
        .IsCalibrationMsg = 0,
#endif /* STM32MP15x || STM32MP25x */
    };

    if (auto ret = HAL_FDCAN_ConfigFilter(&hfdcan, &hal_filter); ret != HAL_OK) {
        bsod("CAN HAL filter failed %i,%i", static_cast<int>(ret), static_cast<int>(hfdcan.ErrorCode));
    }
}

extern "C" void HAL_FDCAN_ErrorCallback(FDCAN_HandleTypeDef *hfdcan) {
    auto &driver = FdcanDriver::get_driver(hfdcan); // Get driver instance
    if (HAL_FDCAN_GetError(hfdcan) & HAL_FDCAN_ERROR_LOG_OVERFLOW) {
        driver.update_error_log(1); // Add 1 for the overflow event
    }
}

extern "C" void HAL_FDCAN_ErrorStatusCallback(FDCAN_HandleTypeDef *hfdcan, uint32_t ErrorStatusITs) {
    auto &driver = FdcanDriver::get_driver(hfdcan); // Get driver instance
    if (ErrorStatusITs & FDCAN_FLAG_BUS_OFF) {
        // BUS OFF is active. This also sets INIT bit and that disables all operations.
        // Reset back to normal operation. The error counter should remain untouched, so the recovery will follow as per CAN spec.
        CLEAR_BIT(hfdcan->Instance->CCCR, FDCAN_CCCR_INIT);
        driver.isr_notify(Driver::Notification::ErrorBusOff);
    }
    if (ErrorStatusITs & FDCAN_FLAG_ERROR_PASSIVE) {
        driver.isr_notify(Driver::Notification::ErrorPassive);
    }
    if (ErrorStatusITs & FDCAN_FLAG_ERROR_WARNING) {
        driver.isr_notify(Driver::Notification::ErrorWarning);
    }
}

} // namespace can
