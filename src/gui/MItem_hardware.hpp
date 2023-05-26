#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include "eeprom.h"

template <size_t SZ>
class MI_SWITCH_NOZZLE_DIAMETER_t : public WI_SWITCH_t<SZ> {
    static constexpr size_t TEXT_LENGTH = 8;
    char str[SZ][TEXT_LENGTH];

    auto float_to_string(size_t i, float f) {
        snprintf(str[i], TEXT_LENGTH, "%0.2f mm", double(f));
        return string_view_utf8::MakeRAM((uint8_t *)str[i]);
    }

    template <std::size_t... I>
    MI_SWITCH_NOZZLE_DIAMETER_t(size_t index, string_view_utf8 label, const png::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden,
        const std::array<float, SZ> &diameters, std::index_sequence<I...>)
        : WI_SWITCH_t<SZ>(index, label, id_icon, enabled, hidden, float_to_string(I, diameters[I])...) {}

public:
    template <typename I = std::make_index_sequence<SZ>>
    MI_SWITCH_NOZZLE_DIAMETER_t(size_t index, string_view_utf8 label, const png::Resource *id_icon, is_enabled_t enabled, is_hidden_t hidden,
        const std::array<float, SZ> &diameters)
        : MI_SWITCH_NOZZLE_DIAMETER_t<SZ>(index, label, id_icon, enabled, hidden, diameters, I {}) {}
};

template <eevar_id EEVAR_ID>
class MI_HARDWARE_CHECK_t : public WI_SWITCH_t<3> {
    constexpr static const char *str_none = N_("None");
    constexpr static const char *str_warn = N_("Warn");
    constexpr static const char *str_strict = N_("Strict");

    size_t get_eeprom() {
        auto i = eeprom_get_ui8(EEVAR_ID);
        return i > 2 ? 1 : i; // invalid eeprom value => default
    }

public:
    MI_HARDWARE_CHECK_t(string_view_utf8 label)
        : WI_SWITCH_t(get_eeprom(), label, nullptr, is_enabled_t::yes, is_hidden_t::no,
            string_view_utf8::MakeCPUFLASH((const uint8_t *)str_none),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)str_warn),
            string_view_utf8::MakeCPUFLASH((const uint8_t *)str_strict)) {}

protected:
    void OnChange([[maybe_unused]] size_t old_index) override {
        eeprom_set_ui8(EEVAR_ID, index);
    }
};

class MI_NOZZLE_DIAMETER : public MI_SWITCH_NOZZLE_DIAMETER_t<6> {
    static constexpr const char *const label = N_("Nozzle Diameter");
    static constexpr const std::array diameters { 0.25f, 0.3f, 0.4f, 0.5f, 0.6f, 0.8f };
    static constexpr const size_t DEFAULT_DIAMETER_INDEX = 2;

    int tool_idx; ///< Configure this tool [indexed from 0]

    /**
     * @brief Get index to diameters array from stored eeprom value.
     * @param tool_idx this tool [indexed from 0] (has to be parameter since internal variable is not yet inited when using this function)
     * @return index to diameters array
     */
    size_t get_eeprom(int tool_idx) const;

public:
    /**
     * @brief Construct menu item to configure nozzle diameter.
     * @param tool_idx this tool [indexed from 0]
     * @param with_toolchanger whether to hide this item if toolchanger is enabled
     */
    MI_NOZZLE_DIAMETER(int tool_idx = 0, is_hidden_t with_toolchanger = is_hidden_t::yes);

protected:
    void OnChange(size_t old_index) override;
};

class MI_HARDWARE_G_CODE_CHECKS : public WI_LABEL_t {
    static constexpr const char *const label = N_("G-Code Checks");

public:
    MI_HARDWARE_G_CODE_CHECKS();

protected:
    virtual void click(IWindowMenu &windowMenu) override;
};

class MI_NOZZLE_DIAMETER_CHECK : public MI_HARDWARE_CHECK_t<EEVAR_HWCHECK_NOZZLE> {
    static constexpr const char *const label = N_("Nozzle Diameter");

public:
    MI_NOZZLE_DIAMETER_CHECK()
        : MI_HARDWARE_CHECK_t(_(label)) {}
};

class MI_PRINTER_MODEL_CHECK : public MI_HARDWARE_CHECK_t<EEVAR_HWCHECK_MODEL> {
    static constexpr const char *const label = N_("Printer Model");

public:
    MI_PRINTER_MODEL_CHECK()
        : MI_HARDWARE_CHECK_t(_(label)) {}
};

class MI_FIRMWARE_CHECK : public MI_HARDWARE_CHECK_t<EEVAR_HWCHECK_FIRMW> {
    static constexpr const char *const label = N_("Firmware Version");

public:
    MI_FIRMWARE_CHECK()
        : MI_HARDWARE_CHECK_t(_(label)) {}
};

class MI_GCODE_LEVEL_CHECK : public MI_HARDWARE_CHECK_t<EEVAR_HWCHECK_GCODE> {
    static constexpr const char *const label = N_("G-Code Level");

public:
    MI_GCODE_LEVEL_CHECK()
        : MI_HARDWARE_CHECK_t(_(label)) {}
};

class MI_MK3_COMPATIBILITY_CHECK : public MI_HARDWARE_CHECK_t<EEVAR_HWCHECK_COMPATIBILITY> {
    static constexpr const char *const label = N_("MK3 Compatibility");

public:
    MI_MK3_COMPATIBILITY_CHECK()
        : MI_HARDWARE_CHECK_t(_(label)) {}
};
