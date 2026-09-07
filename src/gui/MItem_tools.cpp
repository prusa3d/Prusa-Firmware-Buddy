#include "MItem_tools.hpp"
#include "img_resources.hpp"
#include "marlin_client.hpp"
#include "marlin_server.hpp"
#include "gui.hpp"
#include "time_helper.hpp"
#include "window_dlg_wait.hpp"
#include "window_file_list.hpp"
#include "sound.hpp"
#include "wui_api.h"
#include "printers.h"
#include "i18n.h"
#include "ScreenHandler.hpp"
#include "bsod.h"
#include <buddy/filename_defs.hpp>
#include <feature/filament_sensor/filament_sensors_handler.hpp>
#include "liveadjust_z.hpp"
#include <feature/filament_sensor/filament_sensor.hpp>
#include <buddy/main.h>
#include "Pin.hpp"
#include "hwio_pindef.h"
#include "config.h"
#include "WindowMenuSpin.hpp"
#include "time_tools.hpp"
#include "footer_eeprom.hpp"
#include <version/version.hpp>
#include <bootloader/bootloader.hpp>
#include "config_features.h"
#include <config_store/store_instance.hpp>
#include "connect/marlin_printer.hpp"
#include <crash_dump/dump.hpp>
#include <feature/prusa/e-stall_detector.h>
#include <option/bootloader.h>
#include <option/filament_sensor.h>
#include <option/has_indx.h>
#include <option/has_power_panic.h>
#include <option/has_side_leds.h>
#include <option/has_coldpull.h>
#include <option/has_auto_retract.h>
#include <option/has_wastebin_fill_tracking.h>
#include <gcode/inject_queue_actions.hpp>
#if HAS_WASTEBIN_FILL_TRACKING()
    #include <feature/wastebin_watcher/wastebin_watcher.hpp>
#endif
#include <raii/auto_restore.hpp>
#include <time.h>
#include <footer_items_heaters.hpp>
#include <footer_line.hpp>
#include <freertos/critical_section.hpp>
#include <utils/string_builder.hpp>
#include <netdev.h>
#include <wui.h>
#include <power_panic.hpp>
#include <logging/log_dest_file.hpp>
#include <numeric_input_config_common.hpp>
#include <option/has_mmu2.h>
#include <gui/menu_item/menu_item_utils.hpp>

#include <type_traits>

#include <option/has_toolchanger.h>
#if HAS_TOOLCHANGER()
    #include "../../../lib/Marlin/Marlin/src/module/prusa/toolchanger.h"
    #include <gui/screen/screen_tool_pick_park.hpp>
#endif

#if HAS_INDX()
    #include <puppies/INDX.hpp>
#endif

#if HAS_LEDS()
    #include <leds/status_leds_handler.hpp>
#endif

#if HAS_SIDE_LEDS()
    #include <leds/side_strip_handler.hpp>
#endif

#if BUDDY_ENABLE_CONNECT()
    #include <connect/marlin_printer.hpp>
#endif

#include <option/has_e2ee_support.h>
#if HAS_E2EE_SUPPORT()
    #include <e2ee/e2ee.hpp>
#endif // HAS_E2EE_SUPPORT()

namespace {

constexpr const char *printer_busy_text = N_("Printer is busy. Please try repeating the action later.");

} // namespace

bool gui_check_space_in_gcode_queue_with_msg() {
    if (marlin_vars().gqueue <= MEDIA_FETCH_GCODE_QUEUE_FILL_TARGET) {
        return true;
    }

    MsgBoxWarning(_(printer_busy_text), Responses_Ok);
    return false;
}

bool gui_try_gcode_with_msg(const char *gcode) {
    switch (marlin_client::gcode_try(gcode)) {

    case marlin_client::GcodeTryResult::Submitted:
        return true;

    case marlin_client::GcodeTryResult::QueueFull:
        MsgBoxWarning(_(printer_busy_text), Responses_Ok);
        return false;

    case marlin_client::GcodeTryResult::GcodeTooLong:
        bsod("Gcode too long");
    }

    return false;
}

/**********************************************************************************************/
// MI_FILAMENT_SENSOR
MI_FILAMENT_SENSOR::MI_FILAMENT_SENSOR()
    : WI_ICON_SWITCH_OFF_ON_t(0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
    update();
}

void MI_FILAMENT_SENSOR::update() {
    set_value(config_store().fsensor_enabled.get());
}

void MI_FILAMENT_SENSOR::OnChange(size_t old_index) {
    // Enabling/disabling FS can generate gcodes (I'm looking at you, MMU!).
    // Fail the action if there's no space in the queue.
    if (!gui_check_space_in_gcode_queue_with_msg()) {
        set_value(old_index > 0);
        return;
    }

    auto &fss = FSensors_instance();
    fss.set_enabled_global(value());

    if (value() && !fss.gui_wait_for_init_with_msg()) {
        FSensors_instance().set_enabled_global(false);
        set_value(old_index > 0);
    }

    // Signal to the parent to check for changed
    Screens::Access()->Get()->WindowEvent(nullptr, GUI_event_t::CHILD_CLICK, nullptr);
}

#if !HAS_INDX()
/*****************************************************************************/
// MI_STUCK_FILAMENT_DETECTION
/*****************************************************************************/
bool MI_STUCK_FILAMENT_DETECTION::init_index() const {
    return config_store().stuck_filament_detection.get();
}

void MI_STUCK_FILAMENT_DETECTION::OnChange(size_t old_index) {
    if (!gui_try_gcode_with_msg(value() ? "M591 S1 P" : "M591 S0 P")) {
        set_value(old_index > 0);
    }
}
#endif

/*****************************************************************************/
// MI_STEALTH_MODE
/*****************************************************************************/
MI_STEALTH_MODE::MI_STEALTH_MODE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().stealth_mode.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_STEALTH_MODE::OnChange(size_t old_index) {
    if (!gui_try_gcode_with_msg(value() ? "M9150" : "M9140")) {
        set_value(old_index > 0);
    }
}

/*****************************************************************************/
// MI_LIVE_ADJUST_Z
MI_LIVE_ADJUST_Z::MI_LIVE_ADJUST_Z()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes,
#if PRINTER_IS_PRUSA_MINI() || PRINTER_IS_PRUSA_MK3_5()
        is_hidden_t::no
#else
        is_hidden_t::dev
#endif
    ) {
}

void MI_LIVE_ADJUST_Z::click(IWindowMenu & /*window_menu*/) {
    open_live_adjust_z_screen();
}

/*****************************************************************************/
// MI_AUTO_HOME
MI_AUTO_HOME::MI_AUTO_HOME()
    : IWindowMenuItem(_("Auto Home"), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_AUTO_HOME::click(IWindowMenu & /*window_menu*/) {
    // Only issue if there are no gcodes in the queue yet
    if (marlin_vars().gqueue != 0) {
        MsgBoxWarning(_(printer_busy_text), Responses_Ok);
        return;
    }

    // Note: This check is _in theory_ a bit racy - we could switch between
    // printing / not printing between the check and the execution. However,
    // this is highly unlikely and also somewhat harmless:
    // * In one direction, we do precise homing even when imprecise would suffice.
    // * In another direction, we add an imprecise homing _to the start_ of the
    //   print, which is before the print itself does its own homing.
    if (marlin_client::is_printing()) {
        marlin_client::gcode("G28 P");
    } else {
        // Outside of a print, we are fine homing imprecisely.
        marlin_client::gcode("G28 P I");
    }
}

#if HAS_WASTEBIN_FILL_TRACKING()
MI_NOZZLE_CLEANER_EMPTY_WASTEBIN::MI_NOZZLE_CLEANER_EMPTY_WASTEBIN()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_NOZZLE_CLEANER_EMPTY_WASTEBIN::click(IWindowMenu & /*window_menu*/) {
    marlin_client::inject(GCodeLiteral("M1986"));
}

void MI_NOZZLE_CLEANER_EMPTY_WASTEBIN::Loop() {
    loop_gcode_inject_menu_item(*this,
        {
            .update_enabled = true,
            .update_icon = true,

            // Disabled only during the start gcodes (homing / MBL / tool-offset), where parking would
            // interfere - i.e. while printing before the first layer. Allowed when idle and once printing.
            .enabled = !marlin_client::is_printing() || marlin_vars().max_printed_z > 0,
        });
}

MI_NOZZLE_CLEANER_AUTOPAUSE::MI_NOZZLE_CLEANER_AUTOPAUSE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().nozzle_cleaner_autopause_on_full.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_NOZZLE_CLEANER_AUTOPAUSE::OnChange(size_t old_index) {
    config_store().nozzle_cleaner_autopause_on_full.set(!old_index);
}

static constexpr const char *nozzle_cleaner_capacity_items[] = {
    N_("Normal"),
    N_("Extended"),
};

// The nozzle cleaner Y manual-calibration fallback auto-detects the bin variant and sets the capacity;
// this switch is the manual override.
MI_NOZZLE_CLEANER_CAPACITY::MI_NOZZLE_CLEANER_CAPACITY()
    : MenuItemSwitch(_(label), nozzle_cleaner_capacity_items, static_cast<size_t>(config_store().nozzle_cleaner_extended_capacity.get())) {}

void MI_NOZZLE_CLEANER_CAPACITY::OnChange(size_t /*old_index*/) {
    config_store().nozzle_cleaner_extended_capacity.set(get_index() == 1);
}

MI_NOZZLE_CLEANER_FILL::MI_NOZZLE_CLEANER_FILL()
    : MenuItemAutoUpdatingLabel(
        _(label),
        [](const std::span<char> &buffer) {
            snprintf(buffer.data(), buffer.size(), "%u / %u",
                static_cast<unsigned>(WastebinWatcher::instance().fill_level()),
                static_cast<unsigned>(WastebinWatcher::instance().capacity()));
        },
        [](auto) { return WastebinWatcher::instance().fill_level(); }) {}

#endif

#if HAS_INDX()
// Range matches the calibration tolerance (offset_tolerance_mm)
static constexpr NumericInputConfig nozzle_cleaner_offset_spin_config = {
    .min_value = -3,
    .max_value = 3,
    .step = 0.05f,
    .max_decimal_places = 2,
    .unit = Unit::millimeter,
};

MI_NOZZLE_CLEANER_X_OFFSET::MI_NOZZLE_CLEANER_X_OFFSET()
    : WiSpin(config_store().nozzle_cleaner_x_origin_offset.get(), nozzle_cleaner_offset_spin_config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_NOZZLE_CLEANER_X_OFFSET::OnClick() {
    config_store().nozzle_cleaner_x_origin_offset.set(value());
}

MI_NOZZLE_CLEANER_Y_OFFSET::MI_NOZZLE_CLEANER_Y_OFFSET()
    : WiSpin(config_store().nozzle_cleaner_y_origin_offset.get(), nozzle_cleaner_offset_spin_config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_NOZZLE_CLEANER_Y_OFFSET::OnClick() {
    config_store().nozzle_cleaner_y_origin_offset.set(value());
}
#endif

/*****************************************************************************/
// MI_MESH_BED
MI_MESH_BED::MI_MESH_BED()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_MESH_BED::click(IWindowMenu & /*window_menu*/) {
    // Only issue if there are no gcodes in the queue yet
    if (marlin_vars().gqueue != 0) {
        MsgBoxWarning(_(printer_busy_text), Responses_Ok);
        return;
    }

    marlin_client::gcode("G28 O");
    marlin_client::gcode("G29");
}

/*****************************************************************************/
// MI_DISABLE_MOTORS
MI_DISABLE_MOTORS::MI_DISABLE_MOTORS()
    : IWindowMenuItem {
        _("Disable Motors"),
    } {}

void MI_DISABLE_MOTORS::click(IWindowMenu & /*window_menu*/) {
#if (PRINTER_IS_PRUSA_MK4() || PRINTER_IS_PRUSA_XL() || PRINTER_IS_PRUSA_MK3_5())
    marlin_client::gcode("M18 X Y E");
#else
    marlin_client::gcode("M18");
#endif
}

/*****************************************************************************/
// MI_SAVE_DUMP
MI_SAVE_DUMP::MI_SAVE_DUMP()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SAVE_DUMP::click(IWindowMenu & /*window_menu*/) {
    if (!crash_dump::dump_is_valid()) {
        MsgBoxInfo(_("No crash dump to save."), Responses_Ok);
        return;
    }

    bool save_result = false;
    window_dlg_wait_t::wait_until(_("A crash dump is being saved."), [&] {
        save_result = crash_dump::save_dump_to_usb("/usb/dump.bin");
        return true;
    });

    if (save_result) {
        MsgBoxInfo(_("A crash dump report (file dump.bin) has been saved to the USB drive."), Responses_Ok);
    } else {
        MsgBoxError(_("Error saving crash dump report to the USB drive. Please reinsert the USB drive and try again."), Responses_Ok);
    }
}

/*****************************************************************************/
// MI_TIMEOUT
MI_TIMEOUT::MI_TIMEOUT()
    : WI_ICON_SWITCH_OFF_ON_t(Screens::Access()->GetMenuTimeout() ? 1 : 0, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_TIMEOUT::OnChange(size_t old_index) {
    if (!old_index) {
        Screens::Access()->EnableMenuTimeout();
    } else {
        Screens::Access()->DisableMenuTimeout();
    }
    config_store().menu_timeout.set(static_cast<uint8_t>(Screens::Access()->GetMenuTimeout()));
}

/*****************************************************************************/
// MI_SOUND_MODE
static constexpr EnumArray<SoundMode, const char *, SoundMode::_count> sound_mode_values {
    { SoundMode::once, N_("Once") },
    { SoundMode::loud, N_("Loud") },
    { SoundMode::silent, N_("Silent") },
    { SoundMode::assist, N_("Assist") },
};

size_t MI_SOUND_MODE::init_index() const {
    SoundMode sound_mode = sound::get_mode();
    return (size_t)(sound_mode > SoundMode::_last ? SoundMode::_default_sound : sound_mode);
}
MI_SOUND_MODE::MI_SOUND_MODE()
    : MenuItemSwitch(_("Sound Mode"), sound_mode_values, init_index()) {
}

void MI_SOUND_MODE::OnChange(size_t /*old_index*/) {
    sound::set_mode(static_cast<SoundMode>(get_index()));
}

/*****************************************************************************/
// MI_SOUND_VOLUME
static constexpr NumericInputConfig sound_volume_spin_config = {
    .max_value = PRINTER_IS_PRUSA_MINI() ? 11 : 3,
    .special_value = 0,
};

MI_SOUND_VOLUME::MI_SOUND_VOLUME()
    : WiSpin(static_cast<uint8_t>(sound::get_volume()), sound_volume_spin_config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_SOUND_VOLUME::OnClick() {
    sound::set_volume(static_cast<int>(GetVal()));
}

/*****************************************************************************/
// MI_SORT_FILES

static constexpr const char *sort_files_items[] = {
    N_("Time"),
    N_("Name"),
};

MI_SORT_FILES::MI_SORT_FILES()
    : MenuItemSwitch(_("Sort Files"), sort_files_items, config_store().file_sort.get()) {}

void MI_SORT_FILES::OnChange(size_t old_index) {
    if (old_index == WF_SORT_BY_TIME) { // default option - was sorted by time of change, set by name
        GuiFileSort::Set(WF_SORT_BY_NAME);
    } else if (old_index == WF_SORT_BY_NAME) { // was sorted by name, set by time
        GuiFileSort::Set(WF_SORT_BY_TIME);
    }
}

/*****************************************************************************/
// MI_TIMEZONE
static constexpr NumericInputConfig timezone_spin_config = {
    .min_value = -12,
    .max_value = 14,
    .unit = Unit::hour,
};

MI_TIMEZONE::MI_TIMEZONE()
    : WiSpin(config_store().timezone.get(), timezone_spin_config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_TIMEZONE::OnClick() {
    int8_t timezone = static_cast<int8_t>(GetVal());
    config_store().timezone.set(timezone);
}

/*****************************************************************************/
// MI_TIMEZONE_MIN
static constexpr EnumArray<time_tools::TimezoneOffsetMinutes, const char *, time_tools::TimezoneOffsetMinutes::_cnt> timezone_offset_values {
    { time_tools::TimezoneOffsetMinutes::no_offset, "00 min" },
    { time_tools::TimezoneOffsetMinutes::min30, "30 min" },
    { time_tools::TimezoneOffsetMinutes::min45, "45 min" },
};

MI_TIMEZONE_MIN::MI_TIMEZONE_MIN()
    : MenuItemSwitch(_("Time Zone Minute Offset"), timezone_offset_values, std::to_underlying(config_store().timezone_minutes.get())) //
{
    set_translate_items(false);
}

void MI_TIMEZONE_MIN::OnChange([[maybe_unused]] size_t old_index) {
    config_store().timezone_minutes.set(static_cast<time_tools::TimezoneOffsetMinutes>(get_index()));
}

/*****************************************************************************/
// MI_TIMEZONE_SUMMER
MI_TIMEZONE_SUMMER::MI_TIMEZONE_SUMMER()
    : WI_ICON_SWITCH_OFF_ON_t(static_cast<uint8_t>(config_store().timezone_summer.get()), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_TIMEZONE_SUMMER::OnChange([[maybe_unused]] size_t old_index) {
    config_store().timezone_summer.set(static_cast<time_tools::TimezoneOffsetSummerTime>(value()));
}

/*****************************************************************************/
// MI_TIME_FORMAT
static constexpr EnumArray<time_tools::TimeFormat, const char *, time_tools::TimeFormat::_cnt> time_format_values {
    { time_tools::TimeFormat::_12h, "12h" },
    { time_tools::TimeFormat::_24h, "24h" },
};

MI_TIME_FORMAT::MI_TIME_FORMAT()
    : MenuItemSwitch(_("Time Format"), time_format_values, std::to_underlying(config_store().time_format.get())) //
{
    set_translate_items(false);
}

void MI_TIME_FORMAT::OnChange([[maybe_unused]] size_t old_index) {
    config_store().time_format.set(static_cast<time_tools::TimeFormat>(get_index()));
}

/*****************************************************************************/
// MI_TIME_NOW
MI_TIME_NOW::MI_TIME_NOW()
    : WiInfo(_("Time")) //
{
    ChangeInformation(time_tools::get_time());
}

/*****************************************************************************/
// MI_FAN_CHECK
MI_FAN_CHECK::MI_FAN_CHECK()
    : WI_ICON_SWITCH_OFF_ON_t(bool(marlin_vars().fan_check_enabled), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_FAN_CHECK::OnChange(size_t old_index) {
    marlin_client::set_fan_check(!old_index);
    config_store().fan_check_enabled.set(static_cast<bool>(marlin_vars().fan_check_enabled));
}

/*****************************************************************************/
// MI_FS_AUTOLOAD
static is_hidden_t get_autoload_hide_state() {
#if HAS_MMU2()
    // Do not show autoload option with MMU rework enabled - BFW-4290
    if (config_store().is_mmu_rework.get()) {
        return is_hidden_t::yes;
    }
#endif
    return is_hidden_t::no;
}

static is_enabled_t get_autoload_enable_state() {
    // Autoloading option doesn't make sense with filament sensors disabled
    if (config_store().fsensor_enabled.get()) {
        return is_enabled_t::yes;
    } else {
        return is_enabled_t::no;
    }
}

MI_FS_AUTOLOAD::MI_FS_AUTOLOAD()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().fs_autoload_enabled.get(), _(label), nullptr, get_autoload_enable_state(), get_autoload_hide_state()) {}

void MI_FS_AUTOLOAD::OnChange(size_t) {
    config_store().fs_autoload_enabled.set(value());
}

/*****************************************************************************/
// MI_PRINT_PROGRESS_TIME
MI_PRINT_PROGRESS_TIME::MI_PRINT_PROGRESS_TIME()
    : WiSpin(config_store().print_progress_time.get(),
        config, _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_PRINT_PROGRESS_TIME::OnClick() {
    config_store().print_progress_time.set(static_cast<uint16_t>(GetVal()));
}

/*****************************************************************************/
// MI_INFO_BED_TEMP
MI_INFO_BED_TEMP::MI_INFO_BED_TEMP()
    : MenuItemAutoUpdatingLabel(_("Bed Temperature"), standard_print_format::temp_c,
        [](auto) { return marlin_vars().temp_bed.get(); } //
    ) {}

/*****************************************************************************/
// MI_INFO_FILAMENT_SENSOR
MI_INFO_FILAMENT_SENSOR::MI_INFO_FILAMENT_SENSOR(const string_view_utf8 &label, const GetterFunction &getter_function)
    : MenuItemAutoUpdatingLabel(
        label, [this](auto &buf) { print_val(buf); }, getter_function) {
}

void MI_INFO_FILAMENT_SENSOR::print_val(const std::span<char> &buffer) const {
    static constexpr EnumArray<FilamentSensorState, const char *, 6> texts {
        { FilamentSensorState::NotInitialized, "ninit / %ld" },
        { FilamentSensorState::NotCalibrated, "ncal / %ld" }, // not calibrated would be too long
        { FilamentSensorState::HasFilament, " INS / %7ld" },
        { FilamentSensorState::NoFilament, "NINS / %7ld" },
        { FilamentSensorState::NotConnected, "ncon / %ld" },
        { FilamentSensorState::Disabled, "nena / %ld" },
    };

    StringBuilder sb(buffer);

    const auto val = value();
    if (!val.has_value()) {
        sb.append_string("N/A");
    } else {
        sb.append_printf(texts.get_fallback(val->state, FilamentSensorState::NotInitialized), val->value);
    }
}

MI_INFO_FILAMENT_SENSOR::Value MI_INFO_FILAMENT_SENSOR::get_value(IFSensor *fsensor) {
    if (!fsensor) {
        return std::nullopt;
    }

    return FilamentSensorStateAndValue {
        .state = fsensor->get_state(),
        .value = fsensor->GetFilteredValue(),
    };
}

#if HAS_EXTRUDER_FSENSOR()
/*****************************************************************************/
// MI_INFO_EXTRUDER_FILAMENT_SENSOR
MI_INFO_EXTRUDER_FILAMENT_SENSOR::MI_INFO_EXTRUDER_FILAMENT_SENSOR(std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool)
    : MI_INFO_FILAMENT_SENSOR(
        string_view_utf8 {},
        [](auto *item) { return static_cast<MI_INFO_EXTRUDER_FILAMENT_SENSOR *>(item)->value(); })
    , tool_(tool) {
    SetLabel(match(
        tool_,
        [&](PhysicalToolIndex t) { return t.display_name(label_params_); },
        [](CurrentlySelectedTool) {
    #if PRINTER_IS_PRUSA_XL()
            return _("Tool Filament Sensor");
    #else
            return _("Filament Sensor");
    #endif
        }));
}

std::optional<FilamentSensorStateAndValue> MI_INFO_EXTRUDER_FILAMENT_SENSOR::value() const {
    const auto tool = resolve_tool_index(tool_);
    if (!tool.has_value()) {
        return std::nullopt;
    }
    return get_value(GetExtruderFSensor(*tool));
}
#endif // HAS_EXTRUDER_FSENSOR()

/*****************************************************************************/
// MI_INFO_SIDE_FILAMENT_SENSOR
MI_INFO_SIDE_FILAMENT_SENSOR::MI_INFO_SIDE_FILAMENT_SENSOR(std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool)
    : MI_INFO_FILAMENT_SENSOR(
        string_view_utf8 {},
        [](auto *item) { return static_cast<MI_INFO_SIDE_FILAMENT_SENSOR *>(item)->value(); })
    , tool_(tool) {
    SetLabel(match(
        tool_,
        [&](PhysicalToolIndex t) { return t.display_name(label_params_); },
        [](CurrentlySelectedTool) { return _("Side Filament Sensor"); }));
}

std::optional<FilamentSensorStateAndValue> MI_INFO_SIDE_FILAMENT_SENSOR::value() const {
    const auto tool = resolve_tool_index(tool_);
    if (!tool.has_value()) {
        return std::nullopt;
    }
    return get_value(GetSideFSensor(*tool));
}

#if BOARD_IS_XBUDDY()
MI_INFO_BED_VOLTAGE::MI_INFO_BED_VOLTAGE()
    : MenuItemAutoUpdatingLabel(_("Bed Voltage"), "%.1f V",
        [](auto) { return sensor_data().bed_voltage.load(); } //
    ) {}

    #if !HAS_INDX() // INDX head circumvents this
MI_INFO_HEATER_VOLTAGE::MI_INFO_HEATER_VOLTAGE()
    : MenuItemAutoUpdatingLabel(_("Heater Voltage"), "%.1f V",
        [](auto) { return sensor_data().heater_voltage.load(); } //
    ) {}

MI_INFO_HEATER_CURRENT::MI_INFO_HEATER_CURRENT()
    : MenuItemAutoUpdatingLabel(_("Heater Current"), "%.1f A",
        [](auto) { return sensor_data().heater_current.load(); } //
    ) {}
    #endif

MI_INFO_INPUT_CURRENT::MI_INFO_INPUT_CURRENT()
    : MenuItemAutoUpdatingLabel(_("Input Current"), "%.1f A",
        [](auto) { return sensor_data().input_current.load(); } //
    ) {}

MI_INFO_MMU_CURRENT::MI_INFO_MMU_CURRENT()
    : MenuItemAutoUpdatingLabel(_("MMU Current"), "%.1f A",
        [](auto) { return sensor_data().mmuCurrent.load(); } //
    ) {}
#endif

#if BOARD_IS_XLBUDDY()
MI_INFO_5V_VOLTAGE::MI_INFO_5V_VOLTAGE()
    : MenuItemAutoUpdatingLabel(_("5V Voltage"), "%.1f V",
        [](auto) { return sensor_data().sandwich5VVoltage.load(); } //
    ) {}

MI_INFO_SANDWICH_5V_CURRENT::MI_INFO_SANDWICH_5V_CURRENT()
    : MenuItemAutoUpdatingLabel(_("Sandwich 5V Current"), "%.2f A",
        [](auto) { return sensor_data().sandwich5VCurrent.load(); } //
    ) {}

MI_INFO_BUDDY_5V_CURRENT::MI_INFO_BUDDY_5V_CURRENT()
    : MenuItemAutoUpdatingLabel(_("XL Buddy 5V Current"), "%.2f A",
        [](auto) { return sensor_data().buddy5VCurrent.load(); } //
    ) {}
#endif

MI_INFO_BOARD_TEMP::MI_INFO_BOARD_TEMP()
    : MenuItemAutoUpdatingLabel(_("Board Temperature"), standard_print_format::temp_c,
        [](auto) { return sensor_data().boardTemp.load(); } //
    ) {
}

#if HAS_DOOR_SENSOR()
MI_INFO_DOOR_SENSOR::MI_INFO_DOOR_SENSOR()
    : MenuItemAutoUpdatingLabel(
        _("Door Sensor"),
        [this](const std::span<char> &buffer) { print_val(buffer); },
        [](auto) { return sensor_data().door_sensor_detailed_state.load(); } //
    ) {
}

void MI_INFO_DOOR_SENSOR::print_val(const std::span<char> &buffer) const {
    static constexpr EnumArray<buddy::DoorSensor::State, const char *, 3> texts {
        { buddy::DoorSensor::State::sensor_detached, N_("detached") },
        { buddy::DoorSensor::State::door_open, N_("open") },
        { buddy::DoorSensor::State::door_closed, N_("closed") },
    };
    const auto detailed_state = value();
    StringBuilder sb(buffer);
    sb.append_string_view(_(texts[detailed_state.state]));
    sb.append_printf(" / 0x%04x", detailed_state.raw_data);
}
#endif

MI_INFO_MCU_TEMP::MI_INFO_MCU_TEMP()
    : MenuItemAutoUpdatingLabel(_("MCU Temperature"), standard_print_format::temp_c,
        [](auto) { return sensor_data().MCUTemp.load(); } //
    ) {}

#if HAS_INDX()
MI_INFO_INDX_PICKUP_FAIL::MI_INFO_INDX_PICKUP_FAIL()
    : MenuItemAutoUpdatingLabel(
        _("Pickup Fails"), "%u",
        [](auto) { return prusa_toolchanger.get_pickup_fail_count(); }) {}
MI_INFO_INDX_PARK_FAIL::MI_INFO_INDX_PARK_FAIL()
    : MenuItemAutoUpdatingLabel(
        _("Park Fails"), "%u",
        [](auto) { return prusa_toolchanger.get_park_fail_count(); }) {}
#endif

MI_FOOTER_RESET::MI_FOOTER_RESET()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_FOOTER_RESET::click([[maybe_unused]] IWindowMenu &window_menu) {
    // simple reset of footer eeprom would be better
    // but footer does not have reload method
    FooterItemHeater::ResetDrawMode();
    FooterLine::SetCenterN(footer::default_center_n_and_fewer);

    for (size_t i = 0; i < FOOTER_ITEMS_PER_LINE__; ++i) {
        config_store().set_footer_setting(i, footer::default_items[i]);
    }
    // send event for all footers
    Screens::Access()->ScreenEvent(nullptr, GUI_event_t::REINIT_FOOTER, footer::encode_item_for_event(footer::Item::none));

    // close this menu, because it is no longer valid and needs to be redrawn
    Screens::Access()->Close();
}

static constexpr const char *heatup_bed_values[] = {
    N_("Nozzle"),
    N_("All"),
};

MI_FILAMENT_CHANGE_PREHEAT_ALL::MI_FILAMENT_CHANGE_PREHEAT_ALL()
    : MenuItemSwitch(_("For Filament Change, Preheat"), heatup_bed_values, config_store().filament_change_preheat_all.get()) {
}
void MI_FILAMENT_CHANGE_PREHEAT_ALL::OnChange(size_t old_index) {
    config_store().filament_change_preheat_all.set(!old_index);
}

MI_SET_READY::MI_SET_READY()
    : IWindowMenuItem(_(label), &img::set_ready_16x16, connect_client::MarlinPrinter::is_printer_ready() ? is_enabled_t::no : is_enabled_t::yes, is_hidden_t::no) {
}

void MI_SET_READY::click([[maybe_unused]] IWindowMenu &window_menu) {
    if (connect_client::MarlinPrinter::set_printer_ready(true)) {
        set_enabled(false);
    }
}

#if HAS_COLDPULL()
MI_COLD_PULL::MI_COLD_PULL()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_COLD_PULL::click([[maybe_unused]] IWindowMenu &window_menu) {
    marlin_client::gcode("M1702");
}
#endif

MI_GCODE_VERIFY::MI_GCODE_VERIFY()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().verify_gcode.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_GCODE_VERIFY::OnChange([[maybe_unused]] size_t old_index) {
    bool newState = !config_store().verify_gcode.get();
    config_store().verify_gcode.set(newState);
}

/*****************************************************************************/
// MI_DEVHASH_IN_QR
MI_DEVHASH_IN_QR::MI_DEVHASH_IN_QR()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().devhash_in_qr.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}
void MI_DEVHASH_IN_QR::OnChange(size_t old_index) {
    config_store().devhash_in_qr.set(!old_index);
}

/**********************************************************************************************/
// MI_LOAD_SETTINGS

MI_LOAD_SETTINGS::MI_LOAD_SETTINGS()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_LOAD_SETTINGS::click(IWindowMenu & /*window_menu*/) {
    auto build_message = [](StringBuilder &msg_builder, const string_view_utf8 &name, bool ok) {
        msg_builder.append_string_view(name);
        msg_builder.append_string(": ");
        msg_builder.append_string_view(ok ? _("Ok") : _("Failed"));
        msg_builder.append_char('\n');
    };
    std::array<char, 150> msg;
    StringBuilder msg_builder(msg);
    msg_builder.append_string_view(_("\nLoading settings finished.\n\n"));

    const bool network_settings_loaded = netdev_load_ini_to_eeprom();
    if (network_settings_loaded) {
        notify_reconfigure();
    }
    build_message(msg_builder, _("Network"), network_settings_loaded);

#if BUDDY_ENABLE_CONNECT()
    build_message(msg_builder, _("Connect"), connect_client::MarlinPrinter::load_cfg_from_ini());
#endif

    MsgBoxInfo(string_view_utf8::MakeRAM(msg.data()), Responses_Ok);
}

#if HAS_LEDS()
/**********************************************************************************************/
// MI_LEDS_ENABLE
MI_LEDS_ENABLE::MI_LEDS_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(leds::StatusLedsHandler::instance().get_active(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}
void MI_LEDS_ENABLE::OnChange([[maybe_unused]] size_t old_index) {
    if (old_index) {
        leds::StatusLedsHandler::instance().set_active(false);
    } else {
        leds::StatusLedsHandler::instance().set_active(true);
    }
}
#endif

#if HAS_SIDE_LEDS()
/**********************************************************************************************/
// MI_SIDE_LEDS_MAX_BRIGTHNESS
MI_SIDE_LEDS_MAX_BRIGTHNESS::MI_SIDE_LEDS_MAX_BRIGTHNESS()
    : WiSpin(
        static_cast<float>(leds::SideStripHandler::instance().get_max_brightness()) * 100 / 255,
        numeric_input_config::percent_with_off,
        _(label)) {
}

void MI_SIDE_LEDS_MAX_BRIGTHNESS::OnClick() {
    leds::SideStripHandler::instance().set_max_brightness(static_cast<uint8_t>(value()) * 255 / 100);
}
#endif

#if HAS_SIDE_LEDS()
/**********************************************************************************************/
// MI_SIDE_LEDS_DIMMED_BRIGTHNESS

MI_SIDE_LEDS_DIMMED_BRIGTHNESS::MI_SIDE_LEDS_DIMMED_BRIGTHNESS()
    : WiSpin(
        static_cast<float>(leds::SideStripHandler::instance().get_dimmed_brightness()) * 100 / 255,
        numeric_input_config::percent_with_off,
        _(label)) {
}

void MI_SIDE_LEDS_DIMMED_BRIGTHNESS::OnClick() {
    leds::SideStripHandler::instance().set_dimmed_brightness(static_cast<uint8_t>(value()) * 255 / 100);
}

void MI_SIDE_LEDS_DIMMED_BRIGTHNESS::Loop() {
    set_enabled(leds::SideStripHandler::instance().get_dimming_enabled() != leds::DimmingEnabled::never);
}
#endif

#if HAS_SIDE_LEDS()
/**********************************************************************************************/
// MI_SIDE_LEDS_DIMMING_ENABLE
static constexpr EnumArray<leds::DimmingEnabled, const char *, leds::DimmingEnabled::_cnt> dimming_enabled_values {
    { leds::DimmingEnabled::never, N_("Never") },
    { leds::DimmingEnabled::always, N_("Always") },
    { leds::DimmingEnabled::not_printing, N_("On Idle") },
};

MI_SIDE_LEDS_DIMMING_ENABLE::MI_SIDE_LEDS_DIMMING_ENABLE()
    : MenuItemSwitch(_(label), dimming_enabled_values, std::to_underlying(leds::SideStripHandler::instance().get_dimming_enabled())) {
}
void MI_SIDE_LEDS_DIMMING_ENABLE::OnChange([[maybe_unused]] size_t old_index) {
    leds::SideStripHandler::instance().set_dimming_enabled(static_cast<leds::DimmingEnabled>(get_index()));
}
#endif

#if HAS_TOOLCHANGER()
/**********************************************************************************************/
// MI_TOOL_LEDS_ENABLE
MI_TOOL_LEDS_ENABLE::MI_TOOL_LEDS_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().tool_leds_enabled.get(), _(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes) {
}
void MI_TOOL_LEDS_ENABLE::OnChange(size_t old_index) {
    #if HAS_DWARF()
    for (auto tool : PhysicalToolIndex::all()) {
        prusa_toolchanger.getTool(tool).set_cheese_led(!old_index ? 0xff : 0x00, 0x00);
    }
    #endif
    // On INDX the head LEDs follow this via indx_head_leds::update() on its next tick.
    config_store().tool_leds_enabled.set(!old_index);
}
#endif

/*****************************************************************************/
#if HAS_POWER_PANIC()
MI_TRIGGER_POWER_PANIC::MI_TRIGGER_POWER_PANIC()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, is_hidden_t::dev, expands_t::no) {
}

void MI_TRIGGER_POWER_PANIC::click([[maybe_unused]] IWindowMenu &windowMenu) {
    buddy::hw::acFault.triggerIT();
}
#endif

#if HAS_TOOLCHANGER()
/*****************************************************************************/
MI_PICK_PARK_TOOL::MI_PICK_PARK_TOOL()
    : IWindowMenuItem(_(label), nullptr, is_enabled_t::yes, prusa_toolchanger.is_toolchanger_enabled() ? is_hidden_t::no : is_hidden_t::yes, expands_t::yes) {
}

void MI_PICK_PARK_TOOL::click(IWindowMenu & /*window_menu*/) {
    #if HAS_INDX()
    const bool no_tool_picked = !PhysicalToolIndex::currently_selected_opt().has_value();
    const bool no_dock_calibrated = config_store().indx_dock_calibrated_mask.get().none();
    // Dont allow pickup of tool when not calibrated (we allow park to enable parking of uncalibrated tools (default position works fine))
    if (no_dock_calibrated && no_tool_picked) {
        switch (MsgBoxWarning(_("Please calibrate the docks first."), { Response::Calibrate, Response::Back })) {
        case Response::Calibrate:
            marlin_client::gcode("M1982");
            return;
        case Response::Back:
            return;
        default:
            bsod_unreachable();
        }
    }
    #endif
    Screens::Access()->Open(screen_tool_pick_park_creator());
}
#endif

/*****************************************************************************/
#if HAS_ILI9488_DISPLAY()
static constexpr const char *display_baudrate_items[] {
    N_("High"), N_("Low")
};

MI_DISPLAY_BAUDRATE::MI_DISPLAY_BAUDRATE()
    : MenuItemSwitch(_("Display Refresh Speed"), display_baudrate_items, config_store().reduce_display_baudrate.get()) {
}

void MI_DISPLAY_BAUDRATE::OnChange(size_t) {
    config_store().reduce_display_baudrate.set(get_index());
}
#endif

/*****************************************************************************/
MI_LOG_TO_TXT::MI_LOG_TO_TXT()
    : WI_ICON_SWITCH_OFF_ON_t(logging::file_log_is_enabled(), _("Save Logs To File")) {}

void MI_LOG_TO_TXT::OnChange(size_t) {
    if (!value()) {
        logging::file_log_disable();
        MsgBoxInfo(_("Logging has been turned off. You can now safely remove the USB drive."), Responses_Ok);
        return;
    }

    static constexpr const char *location = "/usb/";
    static constexpr const char *filename = "log.txt";

    ArrayStringBuilder<filename_defs::path_buffer_size> filepath;
    filepath.append_string(location);
    filepath.append_string(filename);

    StringViewUtf8Parameters<filename_defs::filename_buffer_size> fmt_buf;

    if (!logging::file_log_enable(filepath.str())) {
        MsgBoxError(_("Failed to open file '%s' for writing.").formatted(fmt_buf, filename), Responses_Ok);
        set_value(false);
        return;
    }

    log_info(Marlin, "Printer: %s", PrinterModelInfo::current().id_str);
    log_info(Marlin, "Version: %s", version::project_version_full);

    MsgBoxInfo(_("The printer will now save all logs to file until restart.\n\nLog file: %s").formatted(fmt_buf, filename), Responses_Ok);
    MsgBoxWarning(_("Turn the logging off before disconnecting the USB drive, or you risk damaging the filesystem!"), Responses_Ok);
}

#if HAS_AUTO_RETRACT()
MI_PRE_NOZZLE_CLEANING_RETRACT::MI_PRE_NOZZLE_CLEANING_RETRACT()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().pre_nozzle_cleaning_retraction_enable.get(), _("Nozzle Cleaning Retraction")) {}

void MI_PRE_NOZZLE_CLEANING_RETRACT::OnChange(size_t) {
    config_store().pre_nozzle_cleaning_retraction_enable.set(value());
}
#endif
