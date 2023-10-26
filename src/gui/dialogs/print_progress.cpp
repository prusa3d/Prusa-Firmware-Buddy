/**
 * @file print_progress.cpp
 * @author Radek Vana
 * @date 2021-03-04
 */

#include "print_progress.hpp"
#include "GuiDefaults.hpp"
#include "menu_spin_config.hpp"
#include "fonts.hpp"
#include "gcode_file.h"
#include "gcode_thumb_decoder.h"
#include <config_store/store_instance.hpp>

constexpr static const char *finish_print_text = N_("Print finished");
constexpr static const char *stop_print_text = N_("Print stopped");
#if not PRINTER_IS_PRUSA_MK4
constexpr static const char *input_shaper_alpha_text = N_("Input Shaper (Alpha)");
#else
constexpr static const char *input_shaper_alpha_text = N_("Input Shaper");
#endif

PrintProgress::PrintProgress(window_t *parent)
    : AddSuperWindow<DialogTimed>(getTime() > SpinCnf::print_progress.Min() ? parent : nullptr, GuiDefaults::RectScreen, 1000 * getTime())
    , estime_label(this, Rect16(22, 260, 140, 16), is_multiline::no)
    , estime_value(this, Rect16(22, 280, 200, 22), is_multiline::no)
    , info_text(this, Rect16(22, 265, 300, 30), is_multiline::no)
    , info_subtitle_text(this, Rect16(22, 295, 300, 30), is_multiline::no)
    , input_shaper_text(this, Rect16(Width() - 200, Height() - 20, 195, 16), is_multiline::no)
    , progress_bar(this, Rect16(Width() - (Width() - GuiDefaults::ProgressThumbnailRect.Width()), 0, Width() - GuiDefaults::ProgressThumbnailRect.Width(), GuiDefaults::ProgressThumbnailRect.Height()))
    , progress_num(this, Rect16(Width() - 145, GuiDefaults::ProgressThumbnailRect.Height() + 5, 130, Height() - GuiDefaults::ProgressThumbnailRect.Height() - 25))
    , thumbnail(this, GuiDefaults::ProgressThumbnailRect)
    , gcode_info(GCodeInfo::getInstance())
    , time_end_format(PT_t::init)
    , mode(ProgressMode_t::PRINTING_INIT) {

    info_text.set_font(resource_font(IDR_FNT_BIG));
    info_text.SetPadding({ 0, 0, 0, 0 });
    info_text.SetAlignment(Align_t::LeftCenter());
    info_text.SetText(_(finish_print_text));
    info_text.Hide();

    info_subtitle_text.set_font(resource_font(IDR_FNT_SMALL));
    info_subtitle_text.SetPadding({ 0, 0, 0, 0 });
    info_subtitle_text.SetAlignment(Align_t::LeftCenter());
    info_subtitle_text.Hide();

    input_shaper_text.set_font(resource_font(IDR_FNT_SMALL));
    input_shaper_text.SetAlignment(Align_t::RightTop());
    input_shaper_text.SetText(_(input_shaper_alpha_text));

    estime_label.set_font(resource_font(IDR_FNT_SMALL));
    estime_label.SetPadding({ 0, 0, 0, 0 });
    estime_label.SetAlignment(Align_t::LeftTop());
    estime_label.SetTextColor(COLOR_SILVER);
    updateEsTime();

    estime_value.set_font(resource_font(IDR_FNT_BIG));
    estime_value.SetPadding({ 0, 0, 0, 0 });
    estime_value.SetAlignment(Align_t::LeftTop());

    progress_num.SetAlignment(Align_t::RightTop());
#ifdef USE_ILI9488
    progress_num.set_font(resource_font(IDR_FNT_LARGE));
#elif defined(USE_ST7789)
    progress_num.set_font(resource_font(IDR_FNT_BIG));
#endif
    gcode_info.initFile(GCodeInfo::GI_INIT_t::THUMBNAIL); // initialize has_..._thumbnail values

    if (!gcode_info.has_progress_thumbnail()) {
        // Permanently disable progress as it makes no sense to look
        // for thumbnail later - there is none.
        disableDialog();
    }
}

uint16_t PrintProgress::getTime() {
    int val = config_store().print_progress_time.get();
    val = std::clamp(val, SpinCnf::print_progress.Min(), SpinCnf::print_progress.Max());
    return val;
}

void PrintProgress::UpdateTexts() {
    switch (mode) {
    case ProgressMode_t::PRINTING_INIT:
        estime_label.Show(); // If label is already shown, this function does nothing
        estime_value.Show();
        info_text.Hide();
        mode = ProgressMode_t::PRINTING;
        [[fallthrough]];
    case ProgressMode_t::PRINTING:
        updateEsTime();
        break;
    case ProgressMode_t::STOPPED_INIT:
        estime_label.Hide();
        estime_value.Hide();
        info_text.SetText(_(stop_print_text));
        info_text.Show();
        PrintProgress::updateEsTime();
        info_subtitle_text.Show();
        mode = ProgressMode_t::END_PREVIEW;
        break;
    case ProgressMode_t::FINISHED_INIT:
        estime_label.Hide();
        estime_value.Hide();
        info_text.SetText(_(finish_print_text));
        info_text.Show();
        PrintProgress::updateEsTime();
        info_subtitle_text.Show();
        thumbnail.redrawWhole(); // thumbnaill will be invalidated by hiding estime_label above, we need to force it to redraw entirely
        thumbnail.Invalidate();
        mode = ProgressMode_t::END_PREVIEW;
        break;
    default:
        break;
    }
}

void PrintProgress::updateLoop(visibility_changed_t visibility_changed) {

    if (thumbnail.updatePercentage(progress_num.getPercentage())) {
        thumbnail.Invalidate();
    }

    // ProgressMode_t::STOPPED_INIT invokes redrawWhole(), due to black screen bug during Stopping from plink/Connect
    if (visibility_changed == visibility_changed_t::yes || mode == ProgressMode_t::STOPPED_INIT) {
        thumbnail.redrawWhole();
        thumbnail.Invalidate();
    }

    UpdateTexts();
}

void PrintProgress::updateEsTime() {
    PT_t time_format = print_time.update_loop(time_end_format, &estime_value, &info_subtitle_text); // cannot return init

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
