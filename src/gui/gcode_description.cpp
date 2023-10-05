#include "gcode_description.hpp"
#include <cstdarg>
#include <span>

size_t description_line_t::title_width(string_view_utf8 *title_str) {
    return title_str->computeNumUtf8CharsAndRewind() * resource_font(IDR_FNT_SMALL)->w;
}

size_t description_line_t::value_width(string_view_utf8 *title_str) {
#ifdef USE_ST7789
    return SCREEN_WIDTH - PADDING * 2 - title_width(title_str) - 1;
#endif // USE_ST7899
#ifdef USE_ILI9488
    return GuiDefaults::PreviewThumbnailRect.Width() - title_width(title_str);
#endif // USE_ILI9488
    assert(false);
    return 0;
}
description_line_t::description_line_t(window_frame_t *frame)
    : title(frame, Rect16(0, 0, 0, 0), is_multiline::no)
    , value(frame, Rect16(0, 0, 0, 0), is_multiline::no) {
}

void description_line_t::update(bool has_preview_thumbnail, size_t row, string_view_utf8 title_str, std::function<void(std::span<char> buffer)> make_value) {
#ifdef USE_ST7789
    title.SetRect(Rect16(PADDING, calculate_y(has_preview_thumbnail, row), title_width(&title_str), LINE_HEIGHT));
    value.SetRect(Rect16(SCREEN_WIDTH - PADDING - value_width(&title_str), calculate_y(has_preview_thumbnail, row), value_width(&title_str), LINE_HEIGHT));
#endif
#ifdef USE_ILI9488
    title.SetRect(Rect16(PADDING.left, calculate_y(has_preview_thumbnail, row), title_width(&title_str), LINE_HEIGHT));
    value.SetRect(Rect16(PADDING.left + GuiDefaults::PreviewThumbnailRect.Width() - value_width(&title_str), calculate_y(has_preview_thumbnail, row), value_width(&title_str), LINE_HEIGHT));
#endif

    title.SetText(title_str);
    title.SetAlignment(Align_t::LeftBottom());
    title.SetPadding({ 0, 0, 0, 0 });
    title.set_font(resource_font(IDR_FNT_SMALL));
    title.SetTextColor(COLOR_GRAY);

    make_value({ value_buffer, sizeof(value_buffer) });
    value.SetText(string_view_utf8::MakeRAM((uint8_t *)value_buffer));
    value.SetAlignment(Align_t::RightBottom());
    value.SetPadding({ 0, 0, 0, 0 });
    value.set_font(resource_font(IDR_FNT_SMALL));
}

static std::span<char> delimited_items_per_extruder(std::span<char> buffer, char delimiter, std::function<int(int extruder, std::span<char> buffer)> echo_item) {
    for (int e = 0; e < std::min(EXTRUDERS, 5) && buffer.size() > 1 /* always needs space for trailing \0 */; e++) {
        if (e != 0) {
            int printed = snprintf(buffer.data(), buffer.size(), "%c", delimiter);
            buffer = buffer.subspan(printed); // we print max 1 character, so we don't need to limit by buffer size here
        }

        int printed = echo_item(e, buffer);
        buffer = buffer.subspan(std::min<int>(printed, buffer.size()));
    }
    return buffer;
}

/// Fills the buffer with a used material info (meant for single tool prints)
/// Example:     PLA/230 g/15 m
static void print_single_extruder_material_info(std::span<char> buffer, GCodeInfo &gcode) {
    std::optional<int> used_extruder;
    for (int e = 0; e < EXTRUDERS; e++) {
        if (gcode.get_extruder_info(e).used()) {
            used_extruder = e;
            break;
        }
    }
    if (used_extruder.has_value()) {
        const auto &extruder_info = gcode.get_extruder_info(used_extruder.value());
        if (!extruder_info.filament_name.has_value() && !extruder_info.filament_used_g.has_value() && !extruder_info.filament_used_mm.has_value()) {
            snprintf(buffer.data(), buffer.size(), "unknown");
        } else {
            snprintf(buffer.data(), buffer.size(), "%s/%1.0f g/%0.2f m",
                extruder_info.filament_name.has_value() ? extruder_info.filament_name->begin() : "?",
                extruder_info.filament_used_g.has_value() ? static_cast<double>(extruder_info.filament_used_g.value()) : 0.0,
                extruder_info.filament_used_mm.has_value() ? static_cast<double>(extruder_info.filament_used_mm.value() / 1000.0f) : 0.0);
        }
    } else {
        snprintf(buffer.data(), buffer.size(), "unknown");
    }
}

/// Fills the buffer with used material types
/// Example:     PLA
/// Example XL:  PLA,PETG,-,-,-
static void print_material_types(std::span<char> buffer, GCodeInfo &gcode) {
    delimited_items_per_extruder(buffer, ',', [&](int extruder, std::span<char> item_buffer) {
        const auto &extruder_info = gcode.get_extruder_info(extruder);
        const auto &filament_name = extruder_info.filament_name;
        if (extruder_info.used() && filament_name.has_value()) {
            return snprintf(item_buffer.data(), item_buffer.size(), "%s", filament_name.value().begin());
        } else if (extruder_info.used()) {
            return snprintf(item_buffer.data(), item_buffer.size(), "%s", "?");
        } else {
            return snprintf(item_buffer.data(), item_buffer.size(), "%s", "-");
        }
    });
}

/// Fills the buffer with used material in meters
/// Example:    1.5/3.4/-/-/- m
static void print_used_material_m(std::span<char> buffer, GCodeInfo &gcode) {
    buffer = delimited_items_per_extruder(buffer, '/', [&](int extruder, std::span<char> item_buffer) {
        const auto &extruder_info = gcode.get_extruder_info(extruder);
        if (extruder_info.used() && extruder_info.filament_used_mm.has_value()) {
            return snprintf(item_buffer.data(), item_buffer.size(), "%0.1f", static_cast<double>(extruder_info.filament_used_mm.value() / 1000.0f));
        } else {
            return snprintf(item_buffer.data(), item_buffer.size(), "-");
        }
    });
    snprintf(buffer.data(), buffer.size(), " m");
}

/// Fills the buffer with used material in grams
/// Example:    15,34,-,-,- g
static void print_used_material_g(std::span<char> buffer, GCodeInfo &gcode) {
    buffer = delimited_items_per_extruder(buffer, ',', [&](int extruder, std::span<char> item_buffer) {
        const auto &extruder_info = gcode.get_extruder_info(extruder);
        if (extruder_info.used() && extruder_info.filament_used_g.has_value()) {
            return snprintf(item_buffer.data(), item_buffer.size(), "%1.0f", static_cast<double>(extruder_info.filament_used_g.value()));
        } else {
            return snprintf(item_buffer.data(), item_buffer.size(), "-");
        }
    });
    snprintf(buffer.data(), buffer.size(), " g");
}

GCodeInfoWithDescription::GCodeInfoWithDescription(window_frame_t *frame)
    : description_lines {
        description_line_t(frame),
        description_line_t(frame),
        description_line_t(frame),
        description_line_t(frame),
    } {
}

void GCodeInfoWithDescription::update(GCodeInfo &gcode) {
    // First line - Print Time
    description_lines[0].update(gcode.has_preview_thumbnail(), 0, _("Print Time"), [&](std::span<char> value_buffer) {
        if (gcode.get_printing_time()[0]) {
            snprintf(value_buffer.data(), value_buffer.size(), "%s", gcode.get_printing_time().data());
        } else {
            snprintf(value_buffer.data(), value_buffer.size(), "unknown");
        }
    });
    // Second line - material
    description_lines[1].update(gcode.has_preview_thumbnail(), 1, _("Material"), [&](std::span<char> value_buffer) {
        if (gcode.has_preview_thumbnail()) {
            // thumbnail, we have to squeze all the material info on a single line
            if (gcode.UsedExtrudersCount() == 0) {
                snprintf(value_buffer.data(), value_buffer.size(), "?");
            } else if (gcode.UsedExtrudersCount() == 1) {
                print_single_extruder_material_info(value_buffer, gcode);
            } else {
                print_material_types(value_buffer, gcode);
            }
        } else {
            // without thumbnail, we are going to use the third and second line for more info about the used material
            print_material_types(value_buffer, gcode);
        }
    });
    // Third line - used filament in meters
    description_lines[2].update(gcode.has_preview_thumbnail(), 2, _("Used Amount"), [&](std::span<char> value_buffer) {
        print_used_material_m(value_buffer, gcode);
    });
    // Fourth line - used filament in grams
    description_lines[3].update(gcode.has_preview_thumbnail(), 3, string_view_utf8::MakeNULLSTR(), [&](std::span<char> value_buffer) {
        print_used_material_g(value_buffer, gcode);
    });

    bool has_filament_used_info = false;

    for (int e = 0; e < std::min(5, EXTRUDERS); e++) {
        has_filament_used_info |= gcode.get_extruder_info(e).filament_used_mm.has_value();
        has_filament_used_info |= gcode.get_extruder_info(e).filament_used_g.has_value();
    }

    if (gcode.has_preview_thumbnail() || !has_filament_used_info) {
        for (int i = 2; i <= 3; i++) {
            description_lines[i].value.Hide();
            description_lines[i].title.Hide();
            description_lines[i].value.Validate(); // == do not redraw
            description_lines[i].title.Validate(); // == do not redraw
        }
    }
}
