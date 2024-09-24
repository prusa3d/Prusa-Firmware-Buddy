#include "parser.hpp"

#include <iterator>
#include <algorithm>
#include <common/crc32.h>

#if __has_include("lwip/def.h")
    #include "lwip/def.h"
#elif __has_include("arpa/inet.h")
    #include "arpa/inet.h"
#else
    #error "Uart parser requires some type of ntohl/ntohs function. Feel free to add some extra header if needed"
#endif

namespace esp {

void RxParserBase::set_intron(std::span<const uint8_t, INTRON_SIZE> new_intron) {
    intron = new_intron;
}

void RxParserBase::reset() {
    if (msg.header.type == esp::MessageType::PACKET_V2) {
        reset_packet();
    }

    msg = {};
    crc = 0;
    buffer = {};
    read = 0;
    state = State::Intron;
    checksum_valid = true;
}

void RxParserBase::process_data(std::span<const uint8_t> input) {
    curr = input.begin();
    end = input.end();

    while (curr != end) {
        if (state == State::Intron) {
            if (wait_for_intron()) {
                crc = crc32_calc(intron.data(), intron.size());
                state = State::Header;
                buffer = std::span { reinterpret_cast<uint8_t *>(&msg.header), sizeof(msg) - sizeof(Intron) };
                read = 0;
            } else {
                return;
            }
        }
        if (state == State::Header) {
            if (wait_for_buffer()) {
                crc = crc32_calc_ex(crc, reinterpret_cast<uint8_t *>(&msg.header), sizeof(msg.header));
                msg.header.size = ntohs(msg.header.size);
                msg.data_checksum = ntohl(msg.data_checksum);

                if (!validate_length_with_type()) {
                    process_invalid_message();
                    reset();
                    continue;
                }

                // For packets this could be bigger then SMALL_BUFFER_SIZE, but the packets are handled by own buffer
                buffer = std::span { small_buffer.data(), msg.header.size };
                read = 0;
                if (msg.header.type == esp::MessageType::PACKET_V2 && !start_packet()) {
                    state = State::ThrowAwayData;
                } else {
                    state = State::Data;
                }
            } else {
                return;
            }
        }
        if (wait_for_data()) {
            on_parsed();
            reset();
        } else {
            return;
        }
    }
}

bool RxParserBase::wait_for_intron() {
    while (curr != end) {
        if (read == INTRON_SIZE) {
            std::shift_left(msg.intron.begin(), msg.intron.end(), 1);
            --read;
        }

        msg.intron.at(read) = *curr++;
        ++read;

        if (read == INTRON_SIZE) {
            if (std::ranges::equal(msg.intron, intron)) {
                return true;
            }
            // TODO: check for default intron to detect esp reset
        }
    }
    return false; // No intron found yet!!
}

bool RxParserBase::wait_for_buffer() {
    const auto remaining_data = std::distance(curr, end);
    const auto to_read = buffer.size() - read;
    const auto to_process = std::min<size_t>(remaining_data, to_read);

    // Don't update the checksum calculation here!!!
    // We are also loading the checksum itself so we don't want to include it in the calculation
    std::copy(curr, std::next(curr, to_process), std::next(buffer.begin(), read));
    std::advance(curr, to_process);
    read += to_process;

    return buffer.size() == read;
}

bool RxParserBase::wait_for_data() {
    if (msg.header.size == 0) {
        return true;
    }

    const auto remaining_data = std::distance(curr, end);
    const auto to_read = buffer.size() - read;
    const auto to_process = std::min<size_t>(remaining_data, to_read);
    if (state == State::Data) {
        const auto to_be_processed_data = std::span { &*curr, to_process };

        crc = crc32_calc_ex(crc, to_be_processed_data.data(), to_be_processed_data.size());
        switch (msg.header.type) {
        case MessageType::PACKET_V2:
            update_packet(to_be_processed_data);
            break;
        default:
            std::copy(to_be_processed_data.begin(), to_be_processed_data.end(), std::next(buffer.begin(), read));
            break;
        }
    }
    std::advance(curr, to_process);
    read += to_process;

    return buffer.size() == read;
}

bool RxParserBase::validate_length_with_type() const {
    switch (msg.header.type) {
    case MessageType::SCAN_AP_CNT:
        return msg.header.size == 0;
    case MessageType::PACKET_V2:
        return true;
    case MessageType::DEVICE_INFO_V2:
        return msg.header.size == sizeof(data::MacAddress);
    case MessageType::SCAN_AP_GET:
        return msg.header.size == sizeof(data::APInfo);
    default:
        return false;
    }
}

void RxParserBase::on_parsed() {
    checksum_valid = (crc == msg.data_checksum);
    switch (msg.header.type) {
    case MessageType::SCAN_AP_GET:
        process_scan_ap_info();
        break;
    case MessageType::SCAN_AP_CNT:
        process_scan_ap_count();
        break;
    case MessageType::DEVICE_INFO_V2:
        process_esp_device_info();
        break;
    case MessageType::PACKET_V2:
        if (state == State::Data) {
            process_packet();
        }
        break;
    default:
        std::abort();
    }
}

} // namespace esp
