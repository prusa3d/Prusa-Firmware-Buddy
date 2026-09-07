#include "can_driver_uart.hpp"

#include <cobs/cobs.hpp>
#include <cstring>
#include <array>
#include <cstddef>
#include <timing.h>
#include <crc/crc.hpp>
#include <bsod/bsod.h>
#include <utils/byte_utils.hpp>

namespace {
template <typename T, typename E>
inline void enforce_bsod(const std::expected<T, E> &result) {
    if (!result.has_value()) {
        bsod_unreachable();
    }
}
} // namespace

namespace can {
static UartDriver *instance = nullptr;

UartDriver::UartDriver(UART_HandleTypeDef &huart)
    : curr_recv_buffer(receive_buffers.allocate())
    , decoder(curr_recv_buffer->get_buffer(), cobs::CobsStreamDecoder::Mode::recovering)
    , huart(huart) {
    if (instance != nullptr) {
        bsod("UART supports only one driver");
    }
    instance = this;
}

UartDriver &UartDriver::get_driver([[maybe_unused]] UART_HandleTypeDef *huart_isr) {
    UartDriver *driver = instance;
    debug_assert(driver != nullptr && &driver->huart == huart_isr);
    return *driver;
}

bool UartDriver::send(const CanardFrame &frame, bool store_timestamp) {
    UNUSED(store_timestamp);
    // Check maximal length
    debug_assert(frame.payload_size <= uart::MAX_PAYLOAD_SIZE); // 64 is max size of CAN FD payload, 2 is the start & stop byte

    // Check if tx process is not already ongoing (before send buffer is overwritten)
    if (this->huart.gState != HAL_UART_STATE_READY) {
        error_log++;
        return false;
    }
    // Prepare the raw payload for COBS encoding and CRC calculation.
    // - Format: [CAN ID (4B)] [payload length (1B)] [payload (0-64B)] [CRC16 (2B)]
    // Pack the 29-bit CAN ID (assuming little-endian byte order)
    static_assert(std::endian::native == std::endian::little);

    // Reset the state of the buffer after previous message
    send_buffer.reset();

    // --- 1. Pack CAN ID (4B) ---
    enforce_bsod(send_buffer.add_bytes(trivial_as_bytes(frame.extended_can_id)));

    // --- 2. Pack payload length (1B) ---
    // Convert and pack payload size
    const uint8_t payload_size = static_cast<uint8_t>(frame.payload_size);
    enforce_bsod(send_buffer.add_bytes(trivial_as_bytes(payload_size)));

    // --- 3. Pack payload data (0-64B) ---
    debug_assert(frame.payload != nullptr);
    enforce_bsod(send_buffer.add_bytes(Bytes { reinterpret_cast<const std::byte *>(frame.payload), payload_size }));

    // --- 4. Calculate and pack CRC16 (2B) ---
    // Get the data added so far for CRC calculation
    auto data = send_buffer.get_used_input_buffer();
    uint16_t crc = Crc16CcittFalse().update(data).get();

    // add CRC and return the result of the addition
    static_assert(sizeof(crc) == uart::CRC_SIZE);
    enforce_bsod(send_buffer.add_bytes(trivial_as_bytes(crc)));

    auto encoded_message = send_buffer.finalize();
    enforce_bsod(encoded_message);

    if (HAL_UART_Transmit_IT(&this->huart, reinterpret_cast<const uint8_t *>(encoded_message->data()), encoded_message->size()) != HAL_OK) {
        error_log++;
        return false;
    }

    return true;
}

void UartDriver::start_listening() {
    HAL_StatusTypeDef status = HAL_UART_Receive_IT(&huart, &uart_rx_byte, 1);

    if (status != HAL_OK) {
        bsod("UART start listening failed");
    }
}

bool UartDriver::receive(CanardFrame &frame, std::array<uint8_t, CANARD_MTU_CAN_FD> &rx_buffer, CanardMicrosecond *timestamp_us) {

    if (receive_buffers.isEmpty()) {
        return false;
    }

    ReceiveBuffer full_recv_buffer = receive_buffers.peek();

    debug_assert(full_recv_buffer.message().size() != 0);
    Bytes decoded_message = full_recv_buffer.message();
    size_t decoded_size = decoded_message.size();

    std::array<std::byte, uart::MAX_FRAME_SIZE> decoded_data;
    std::copy(decoded_message.begin(), decoded_message.end(), decoded_data.data());
    // copied, we can remove from the queue to prepare it for rx_callbacks ASAP
    receive_buffers.dequeue();

    *timestamp_us = get_timestamp_us();
    // extract message
    memcpy(&frame.extended_can_id, &decoded_data[0], sizeof(frame.extended_can_id));
    frame.payload_size = std::to_integer<uint8_t>(decoded_data[4]);
    if (decoded_size != static_cast<size_t>(uart::HEADER_SIZE + frame.payload_size + uart::CRC_SIZE)) {
        debug_assert(false);
        return false;
    }
    memcpy(rx_buffer.data(), &decoded_data[uart::HEADER_SIZE], frame.payload_size);
    frame.payload = rx_buffer.data();

    // Extract received CRC (little-endian)
    const size_t crc_pos = uart::HEADER_SIZE + frame.payload_size;
    uint16_t received_crc;
    memcpy(&received_crc, &decoded_data[crc_pos], uart::CRC_SIZE);

    // Calculate and check CRC of the extracted message (excluding the CRC bytes)
    const uint16_t calculated_crc = Crc16CcittFalse().update(Bytes { decoded_data.data(), uart::HEADER_SIZE + frame.payload_size }).get();
    if (received_crc != calculated_crc) {
        debug_assert(false);
        return false;
    }

    return true;
}

void UartDriver::rx_callback() {

    // if decoder has no valid buffer, attempt allocation now
    if (!curr_recv_buffer) {
        // if allocation fails, drop the current incoming byte and wait for the next isr
        // the decoder is left in a state that will fail until allocation succeeds.
        curr_recv_buffer = receive_buffers.allocate();

        // if successful, reset decoder into a recovery state to restart the stream cleanly
        if (curr_recv_buffer) {
            decoder.reset(cobs::CobsStreamDecoder::Mode::recovering, curr_recv_buffer->get_buffer());
        } else {
            // otherwise, keep it in error mode
            // (this is theoretically not necessary, decoder should already be in error state, but just to be sure)
            decoder.reset(cobs::CobsStreamDecoder::Mode::error);
        }
    }

    auto finish_cb = [this](Bytes result) -> void {
        // ignore empty messages
        if (result.empty()) {
            decoder.reset(cobs::CobsStreamDecoder::Mode::decoding);
            return;
        }

        curr_recv_buffer->set_message(result);

        receive_buffers.commit(curr_recv_buffer);
        curr_recv_buffer = receive_buffers.allocate();

        // if new buffer was sucessfuly allocated we can reset the decoder straight away
        if (curr_recv_buffer) {
            decoder.reset(cobs::CobsStreamDecoder::Mode::decoding, curr_recv_buffer->get_buffer());
        } else {
            // the decoder is put into error mode because it has invalid destination buffer.
            // eventually some buffer should free up, we will check for that at the start of rx_callback()
            // we will miss atleast one message - we can live with that
            decoder.reset(cobs::CobsStreamDecoder::Mode::error);
        }

        isr_notify(Driver::Notification::RxDone);
    };

    auto error_cb = [this]([[maybe_unused]] cobs::CobsError error) -> void {
        decoder.reset(cobs::CobsStreamDecoder::Mode::recovering);
    };

    decoder.add_byte(
        std::byte { uart_rx_byte },
        finish_cb,
        error_cb);

    start_listening();
}

void UartDriver::tx_callback() {
    isr_notify(Driver::Notification::TxDone);
    tx_timestamp.set(get_timestamp_us());
}

void UartDriver::error_callback() {
    // Note: The 'tec' counter is not handled here. In an interrupt-driven UART
    // implementation, transmit errors would typically be managed in the send()
    // method itself (e.g., if HAL_UART_Transmit_IT() returns a failure status)
    // and not via this global callback.
    error_log++;
}

extern "C" {

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
    auto &driver = UartDriver::get_driver(huart);
    driver.rx_callback();
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart) {
    auto &driver = UartDriver::get_driver(huart);
    driver.tx_callback();
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
    auto &driver = UartDriver::get_driver(huart);
    driver.error_callback();
}
}

} // namespace can
