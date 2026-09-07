/// @file
#include "screen_printer_type_changed.hpp"

#include <bsod.h>
#include <common/printer_model.hpp>
#include <config_store/store_instance.hpp>
#include <feature/factory_reset/factory_reset.hpp>
#include <guiconfig/GuiDefaults.hpp>
#include <img_resources.hpp>

namespace {

constexpr PhaseResponses responses { Response::Continue };

constexpr Font title_font = Font::big;
constexpr Font font = Font::normal;

/// Free space around the screen content, also between the icon and the title.
/// The buttons are placed by GuiDefaults and do not use this.
constexpr uint16_t margin = 32;

constexpr uint16_t icon_size = 48;
constexpr uint16_t line_height = height(font);

constexpr const img::Resource &arrow_icon = img::arrow_right_10x16;

/// Column reserved for the arrow between the model names, including the spacing around it
constexpr uint16_t arrow_spacing = 15;
constexpr uint16_t arrow_column_width = arrow_icon.w + 2 * arrow_spacing;

constexpr int16_t buttons_top = GuiDefaults::ScreenHeight - GuiDefaults::ButtonHeight - GuiDefaults::FramePadding;
constexpr uint16_t body_width = GuiDefaults::ScreenWidth - 2 * margin;
constexpr int16_t body_right = margin + body_width;

/// The description is top aligned, so its height has to be reserved up front.
/// Three lines fit the more verbose translations.
constexpr uint16_t description_height = 3 * line_height;

/// The rows are distributed evenly in the space between the margins
constexpr uint16_t rows_height = icon_size + line_height + description_height;
constexpr uint16_t row_gap = (buttons_top - 2 * margin - rows_height) / 2;

constexpr Rect16 icon_rect { static_cast<int16_t>(margin), static_cast<int16_t>(margin), icon_size, icon_size };

constexpr Rect16 title_rect {
    static_cast<int16_t>(margin + icon_size + margin),
    static_cast<int16_t>(margin),
    static_cast<uint16_t>(body_width - icon_size - margin),
    icon_size,
};

constexpr int16_t models_top = margin + icon_size + row_gap;
constexpr int16_t arrow_column_left = (GuiDefaults::ScreenWidth - arrow_column_width) / 2;
constexpr int16_t arrow_column_right = arrow_column_left + arrow_column_width;

constexpr Rect16 model_from_rect { static_cast<int16_t>(margin), models_top, static_cast<uint16_t>(arrow_column_left - margin), line_height };
constexpr Rect16 model_to_rect { arrow_column_right, models_top, static_cast<uint16_t>(body_right - arrow_column_right), line_height };

/// The arrow is smaller than the text line, center it on the model names
constexpr Rect16 arrow_rect {
    static_cast<int16_t>((GuiDefaults::ScreenWidth - arrow_icon.w) / 2),
    static_cast<int16_t>(models_top + (line_height - arrow_icon.h) / 2),
    arrow_icon.w,
    arrow_icon.h,
};

constexpr int16_t description_top = models_top + line_height + row_gap;
constexpr Rect16 description_rect { static_cast<int16_t>(margin), description_top, body_width, description_height };

} // namespace

bool ScreenPrinterTypeChanged::should_show() {
    auto &model_var = config_store().last_boot_base_printer_model;
    const auto current_base_model = PrinterModelInfo::firmware_base().model;

    if (model_var.get() == model_var.default_val) {
        // Not initialized - assume correct printer
        model_var.set(current_base_model);
        return false;
    }

    return model_var.get() != current_base_model;
}

ScreenPrinterTypeChanged::ScreenPrinterTypeChanged()
    // The factory reset is mandatory, the screen must not close on its own
    : screen_t {
        nullptr,
        win_type_t::normal,
        is_closed_on_timeout_t::no,
        is_closed_on_printing_t::no,
    }
    , icon {
        this,
        icon_rect,
        &img::warning_48x48,
    }
    , title {
        this,
        title_rect,
        _("Printer type changed"),
        is_multiline::yes,
    }
    , model_from {
        this,
        model_from_rect,
        string_view_utf8::MakeCPUFLASH(PrinterModelInfo::get(config_store().last_boot_base_printer_model.get()).display_str()),
        is_multiline::no,
    }
    , arrow {
        this,
        arrow_rect,
        &arrow_icon,
    }
    , model_to {
        this,
        model_to_rect,
        string_view_utf8::MakeCPUFLASH(PrinterModelInfo::firmware_base().display_str()),
        is_multiline::no,
    }
    , description {
        this,
        description_rect,
        _("Factory reset is required. Don't worry, your settings will be kept."),
        is_multiline::yes,
    }
    , radio {
        this,
        GuiDefaults::GetButtonRect(GetRect()),
        responses,
    } {
    CaptureNormalWindow(radio);

    title.set_font(title_font);
    title.SetAlignment(Align_t::LeftCenter());

    model_from.set_font(font);
    model_from.SetAlignment(Align_t::RightCenter());

    model_to.set_font(font);
    model_to.SetAlignment(Align_t::LeftCenter());

    description.set_font(font);
    description.SetAlignment(Align_t::CenterTop());
}

void ScreenPrinterTypeChanged::windowEvent(window_t *, GUI_event_t event, void *) {
    switch (event) {

    case GUI_event_t::CHILD_CLICK:
        FactoryReset::perform(false, FactoryReset::printer_type_change);
        bsod_unreachable();
        break;

    default:
        break;
    }
}
