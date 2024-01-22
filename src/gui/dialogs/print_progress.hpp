/**
 * @file print_progress.hpp
 * @author Radek Vana
 * @brief print progress - to be used while printing
 * @date 2021-03-04
 */
#pragma once

#include "DialogTimed.hpp"
#include "window_text.hpp"
#include "window_progress.hpp"
#include "window_icon.hpp"
#include "window_print_progress.hpp"
#include "window_thumbnail.hpp"
#include "gcode_info.hpp"
#include "print_time_module.hpp"

class PrintProgress : public AddSuperWindow<DialogTimed> {
    enum class ProgressMode_t {
        PRINTING_INIT,
        PRINTING,
        STOPPED_INIT,
        FINISHED_INIT,
        END_PREVIEW,
    };

    window_text_t estime_label;
    window_text_t estime_value;
    window_text_t info_text;
    WindowPrintProgress progress_bar;
    WindowNumbPrintProgress progress_num;
    WindowProgressThumbnail thumbnail;
    window_icon_t more_icon; // icon suggesting that there's more to be seen if user 'clicks'

    GCodeInfo &gcode_info;
    PrintTime print_time;
    PT_t time_end_format;
    ProgressMode_t mode;

    void UpdateTexts();
    void updateEsTime();
    uint16_t getTime();

    void show_col_text_fields();
    void hide_col_text_fields();

protected:
    virtual void updateLoop(visibility_changed_t visibility_changed) override;

public:
    PrintProgress(window_t *parent);

    /**
     * @brief Initialize GCodeInfo.
     * This must be called after GCodeInfo has valid file path,
     *   after GCodeInfo::Init() has been inited with gui_media_SFN_path and that has been updated.
     */
    void init_gcode_info();

    void Pause();
    void Resume();
    void FinishedMode();
    void StoppedMode();
    void PrintingMode();
};
