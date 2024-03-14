/**
 * @file print_progress.cpp
 * @author Radek Vana
 * @date 2021-03-04
 */

#include "print_progress.hpp"
#include "GuiDefaults.hpp"
#include "menu_spin_config.hpp"
#include "fonts.hpp"
#include "gcode_thumb_decoder.h"
#include <config_store/store_instance.hpp>
#include <media.h>

namespace {
constexpr const char *finish_print_text = N_("Print finished");
constexpr const char *stop_print_text = N_("Print stopped");
constexpr size_t text_baseline_y { GuiDefaults::ProgressThumbnailRect.Height() + GuiDefaults::ProgressBarHeight + GuiDefaults::ProgressTextTopOffset };
constexpr auto more_icon_res { &img::arrow_right_10x16 };
constexpr size_t icon_size_x { more_icon_res->w };
constexpr size_t icon_size_y { more_icon_res->h };
constexpr size_t text_left_side_offset { 12 };
constexpr size_t icon_right_side_offset { 12 };
constexpr size_t icon_x_offset { icon_size_x + icon_right_side_offset };

/**
 * @brief Print progress has three columns:
 * LEFT TEXT .... MIDDLE TEXT ... PROGRESS % + 'more' icon
 * MIDDLE TEXT is not available on narrow screens
 */

constexpr size_t text_label_height { 16 };
constexpr size_t text_value_height { 22 };
constexpr size_t text_value_y_offset { 4 }; // additional offset of value row from label

constexpr size_t inter_col_offsets { 10 };
#if defined(USE_ILI9488)
constexpr size_t left_column_width { (GuiDefaults::ProgressThumbnailRect.Width() - text_left_side_offset - icon_right_side_offset - inter_col_offsets * 2) / 3 };
constexpr size_t middle_column_start_x { text_left_side_offset + left_column_width + inter_col_offsets };
constexpr size_t middle_column_width { left_column_width };

constexpr size_t info_text_width { left_column_width * 2 + inter_col_offsets };
#elif defined(USE_ST7789)
constexpr size_t left_column_width { (GuiDefaults::ProgressThumbnailRect.Width() - text_left_side_offset - icon_right_side_offset - inter_col_offsets) / 2 };

constexpr size_t info_text_width { left_column_width };
#endif

constexpr size_t progress_num_icon_offset { 10 };
constexpr size_t progress_num_width { left_column_width - icon_size_x - progress_num_icon_offset };
constexpr size_t progress_num_x_offset { icon_x_offset + progress_num_width + progress_num_icon_offset };
constexpr size_t progress_num_height { GuiDefaults::ProgressTextHeight - GuiDefaults::ProgressTextTopOffset };

#ifdef USE_ILI9488
constexpr auto progress_num_font { IDR_FNT_LARGE };
constexpr int right_side_perc_magic_number { 4 }; // magic number to align text from left_value with percent-text from progress_num font
constexpr int right_side_icon_magic_number { -right_side_perc_magic_number }; // align icon with percentile
#elif defined(USE_ST7789)
constexpr auto progress_num_font { IDR_FNT_BIG };
constexpr int right_side_perc_magic_number { 0 };
constexpr int right_side_icon_magic_number { -5 };
#endif

constexpr auto progress_num_y_baseline { GuiDefaults::ProgressThumbnailRect.Height() + GuiDefaults::ProgressBarHeight + right_side_perc_magic_number };

constexpr auto icon_y_baseline {
    static_cast<Rect16::Y_t>((GuiDefaults::ScreenHeight - progress_num_y_baseline) / 2 - icon_size_y / 2 + progress_num_y_baseline + right_side_icon_magic_number)
};

static_assert(resource_font_size(progress_num_font).w * 4 <= progress_num_width);
static_assert(resource_font_size(progress_num_font).h <= progress_num_height);

} // namespace

PrintProgress::PrintProgress(window_t *parent)
    : AddSuperWindow<DialogTimed>(getTime() > SpinCnf::print_progress.Min() ? parent : nullptr, GuiDefaults::RectScreen, 1000 * getTime())
    , estime_label(this, Rect16(text_left_side_offset, text_baseline_y, left_column_width, text_label_height), is_multiline::no)
    , estime_value(this, Rect16(text_left_side_offset, text_baseline_y + text_label_height + text_value_y_offset, left_column_width
#ifdef USE_ILI9488 // adding middle_column_width because middle column is currently unused & the left estime_value doesn't have enough space to hold the current version of all the data
                                 + middle_column_width
#endif
                             ,
                             text_value_height),
          is_multiline::no)
    // #if defined(USE_ILI9488)
    //     , middle_col_label(this, Rect16(middle_column_start_x, text_baseline_y, middle_column_width, text_label_height), is_multiline::no)
    //     , middle_col_value(this, Rect16(middle_column_start_x, text_baseline_y + text_label_height + text_value_y_offset, middle_column_width, text_value_height), is_multiline::no)
    // #endif
    , info_text(this, Rect16(text_left_side_offset, text_baseline_y + text_label_height, info_text_width, text_value_height), is_multiline::no)
    , progress_bar(this, Rect16(Left(), GuiDefaults::ProgressThumbnailRect.Height(), Width(), GuiDefaults::ProgressBarHeight))
    , progress_num(this, Rect16(Width() - progress_num_x_offset, progress_num_y_baseline, progress_num_width, progress_num_height))
    , thumbnail(this, GuiDefaults::ProgressThumbnailRect, GuiDefaults::OldSlicerProgressImgWidth)
    , more_icon(this, Rect16(Width() - icon_x_offset, icon_y_baseline, icon_size_x, icon_size_y), more_icon_res)
    , gcode_info(GCodeInfo::getInstance())
    , time_end_format(PT_t::init)
    , mode(ProgressMode_t::PRINTING_INIT) {

    info_text.set_font(resource_font(IDR_FNT_BIG));
    info_text.SetPadding({ 0, 0, 0, 0 });
    info_text.SetAlignment(Align_t::LeftCenter());
    info_text.SetText(_(finish_print_text));
    info_text.Hide();

    estime_label.set_font(resource_font(IDR_FNT_SMALL));
    estime_label.SetPadding({ 0, 0, 0, 0 });
    estime_label.SetAlignment(Align_t::LeftTop());
    estime_label.SetTextColor(COLOR_SILVER);
    updateEsTime();

    estime_value.set_font(resource_font(IDR_FNT_BIG));
    estime_value.SetPadding({ 0, 0, 0, 0 });
    estime_value.SetAlignment(Align_t::LeftTop());

    // #if defined(USE_ILI9488)
    //     middle_col_label.set_font(resource_font(IDR_FNT_SMALL));
    //     middle_col_label.SetPadding({ 0, 0, 0, 0 });
    //     middle_col_label.SetAlignment(Align_t::LeftTop());
    //     middle_col_label.SetTextColor(COLOR_SILVER);

    //     middle_col_value.set_font(resource_font(IDR_FNT_BIG));
    //     middle_col_value.SetPadding({ 0, 0, 0, 0 });
    //     middle_col_value.SetAlignment(Align_t::LeftTop());
    // #endif

    progress_num.SetAlignment(Align_t::Center());
    progress_num.set_font(resource_font(progress_num_font));
}

void PrintProgress::init_gcode_info() {
    if (!gcode_info.is_loaded()) {
        // Signal media_prefetch to get gcode info
        osSignalSet(prefetch_thread_id, PREFETCH_SIGNAL_GCODE_INFO_INIT);
    }
}

uint16_t PrintProgress::getTime() {
    int val = config_store().print_progress_time.get();
    val = std::clamp(val, SpinCnf::print_progress.Min(), SpinCnf::print_progress.Max());
    return val;
}

void PrintProgress::show_col_text_fields() {
    estime_label.Show();
    estime_value.Show();
    // #if defined(USE_ILI9488)
    //     middle_col_label.Show();
    //     middle_col_value.Show();
    // #endif
}

void PrintProgress::hide_col_text_fields() {
    estime_label.Hide();
    estime_value.Hide();
    // #if defined(USE_ILI9488)
    //     middle_col_label.Hide();
    //     middle_col_value.Hide();
    // #endif
}

void PrintProgress::UpdateTexts() {
    switch (mode) {
    case ProgressMode_t::PRINTING_INIT:
        show_col_text_fields();
        info_text.Hide();
        mode = ProgressMode_t::PRINTING;
        [[fallthrough]];
    case ProgressMode_t::PRINTING:
        updateEsTime();
        break;
    case ProgressMode_t::STOPPED_INIT:
        hide_col_text_fields();
        info_text.SetText(_(stop_print_text));
        info_text.Show();
        mode = ProgressMode_t::END_PREVIEW;
        break;
    case ProgressMode_t::FINISHED_INIT:
        hide_col_text_fields();
        info_text.SetText(_(finish_print_text));
        info_text.Show();
        thumbnail.redrawWhole(); // thumbnaill will be invalidated by hiding estime_label above, we need to force it to redraw entirely
        thumbnail.Invalidate();
        mode = ProgressMode_t::END_PREVIEW;
        break;
    default:
        break;
    }
}

void PrintProgress::updateLoop(visibility_changed_t visibility_changed) {

    // ProgressMode_t::STOPPED_INIT invokes redrawWhole(), due to black screen bug during Stopping from plink/Connect
    if (visibility_changed == visibility_changed_t::yes || mode == ProgressMode_t::STOPPED_INIT) {
        thumbnail.redrawWhole();
        thumbnail.Invalidate();
    }

    UpdateTexts();
}

void PrintProgress::updateEsTime() {
    PT_t time_format = print_time.update_loop(time_end_format, &estime_value); // cannot return init

    if (time_format != time_end_format) {
        switch (time_format) {
        case PT_t::init: // should not happen
            return;
        case PT_t::countdown:
            estime_label.SetText(_(PrintTime::EN_STR_COUNTDOWN));
            break;
        case PT_t::timestamp:
            estime_label.SetText(_(PrintTime::EN_STR_TIMESTAMP));
            break;
        }

        time_end_format = time_format;
    }

    // TODO: Handle middle column
}

void PrintProgress::Pause() {
    pauseDialog();
    thumbnail.pauseDeinit();
}

void PrintProgress::Resume() {
    thumbnail.pauseReinit();
    if (gcode_info.has_progress_thumbnail()) {
        resumeDialog();
    }
}

void PrintProgress::FinishedMode() {
    if (mode != ProgressMode_t::END_PREVIEW) { // For avoiding redundant operations
        mode = ProgressMode_t::FINISHED_INIT;
    }
}

void PrintProgress::StoppedMode() {
    if (mode != ProgressMode_t::END_PREVIEW) { // For avoiding redundant operations
        mode = ProgressMode_t::STOPPED_INIT;
    }
}

void PrintProgress::PrintingMode() {
    if (mode != ProgressMode_t::PRINTING) { // For avoiding redundant operations
        mode = ProgressMode_t::PRINTING_INIT;
    }
}
