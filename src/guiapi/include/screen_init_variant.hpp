/**
 * @file screen_init_variant.hpp
 * @brief variant data type for screen initialization
 */

#pragma once

#include <cstdint>
#include <optional>

class screen_init_variant {
public:
    enum class type_t : uint8_t {
        data_not_valid,
        position,
        menu
    };

    struct menu_t {
        uint8_t index;
        uint8_t top_index;
    };

    screen_init_variant()
        : type(type_t::data_not_valid) {
    }
    void SetPosition(uint16_t pos) {
        position = pos;
        type = type_t::position;
    }
    std::optional<uint16_t> GetPosition() {
        if (type != type_t::position) {
            return std::nullopt;
        }
        return position;
    }

    void SetMenuPosition(menu_t pos) {
        menu = pos;
        type = type_t::menu;
    }
    std::optional<menu_t> GetMenuPosition() {
        if (type != type_t::menu) {
            return std::nullopt;
        }
        return menu;
    }

private:
    type_t type;
    union {
        uint16_t position;
        menu_t menu;
    };
};
