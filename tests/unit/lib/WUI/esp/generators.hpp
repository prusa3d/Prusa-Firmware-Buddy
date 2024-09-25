#pragma once
#include "esp_protocol/messages.hpp"
#include <catch2/catch.hpp>

struct RandomBytesGenerator final : public Catch::Generators::IGenerator<std::vector<uint8_t>> {
    RandomBytesGenerator(size_t min, size_t max)
        : min(min)
        , max(max) {
        current.reserve(max);
        next();
    }
    explicit RandomBytesGenerator(size_t length)
        : RandomBytesGenerator(length, length) {}

    bool next() override {
        size_t new_size = min;
        if (max > min) {
            new_size += rng() % (max - min + 1);
        }

        current.clear();
        for (size_t i = 0; i < new_size; ++i) {
            current.push_back(rng() % 256);
        }

        return true;
    }

    const std::vector<uint8_t> &get() const override {
        return current;
    }

    std::string stringifyImpl() const {
        std::ostringstream oss {};
        bool first = true;
        oss << "{";
        for (const auto val : current) {
            if (first) {
                first = false;
            } else {
                oss << ", ";
            }
            oss << val;
        }
        oss << "}";
        return oss.str();
    }

protected:
    std::mt19937_64 rng {};
    size_t min, max;
    std::vector<uint8_t> current {};
};

auto random_bytes(size_t min, size_t max) {
    return Catch::Generators::GeneratorWrapper<std::vector<uint8_t>>(Catch::Generators::pf::make_unique<RandomBytesGenerator>(min, max));
}

auto random_bytes(size_t length) {
    return Catch::Generators::GeneratorWrapper<std::vector<uint8_t>>(Catch::Generators::pf::make_unique<RandomBytesGenerator>(length));
}

struct RandomEspMessagesGenerator final : public Catch::Generators::IGenerator<std::vector<uint8_t>> {
    static constexpr auto valid_message_types = std::to_array({ esp::MessageType::DEVICE_INFO_V2,
        esp::MessageType::PACKET_V2,
        esp::MessageType::SCAN_AP_CNT,
        esp::MessageType::SCAN_AP_GET });
    explicit RandomEspMessagesGenerator(std::span<const uint8_t, esp::INTRON_SIZE> default_intron) {
        std::copy_n(default_intron.begin(), intron.size(), intron.begin());
        next();
    }

    bool next() override {
        current.clear();
        std::copy(intron.begin(), intron.end(), std::back_inserter(current));

        const auto message_type = valid_message_types[rng() % valid_message_types.size()];
        current.push_back(static_cast<uint8_t>(message_type));

        size_t data_size = 0;
        switch (message_type) {
        case esp::MessageType::DEVICE_INFO_V2:
            data_size = sizeof(esp::data::MacAddress);
            current.push_back(rng() & 0xFF);
            break;
        case esp::MessageType::PACKET_V2:
            data_size = rng() & 0xFFFF;
            current.push_back(rng() & 0x1);
            break;
        case esp::MessageType::SCAN_AP_CNT:
            current.push_back(rng() & 0xFF);
            break;
        case esp::MessageType::SCAN_AP_GET:
            data_size = sizeof(esp::data::APInfo);
            current.push_back(rng() & 0xFF);
            break;
        default:
            assert(false && "Invalid message type");
        }
        current.push_back((data_size & 0xFF00) >> 8);
        current.push_back(data_size & 0xFF);

        for (size_t i = 0; i < data_size + 4; ++i) {
            current.push_back(rng() & 0xFF);
        }

        return true;
    }

    const std::vector<uint8_t> &get() const override {
        return current;
    }

protected:
    esp::Intron intron;
    std::mt19937_64 rng {};
    std::vector<uint8_t> current {};
};

auto random_messages(std::span<const uint8_t, esp::INTRON_SIZE> default_intron) {
    return Catch::Generators::GeneratorWrapper<std::vector<uint8_t>>(Catch::Generators::pf::make_unique<RandomEspMessagesGenerator>(default_intron));
}
