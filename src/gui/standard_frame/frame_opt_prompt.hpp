/// @file
#pragma once

#include <window_icon.hpp>
#include <window_text.hpp>
#include <window_frame.hpp>
#include <radio_button.hpp>
#include <utils/storage/strong_index_array.hpp>
#include <utils/timing/timer.hpp>
#include <inplace_function.hpp>
#include <feature/openprinttag/tool_tag.hpp>
#include <feature/openprinttag/data_utils.hpp>

namespace buddy::openprinttag {

/// Frame that loads basic properties from the provided OpenPrintTag
/// With a radio button on the bottom
class FrameOPTPrompt : public window_frame_t {

public:
    // We need screenEvent and windowEvent
    static constexpr bool needs_to_inherit_from_window_t = true;

    enum class Detail : uint8_t {
        material_name,
        brand_name,
        abbreviation,
        weight,
        _cnt,
    };

    /// Called when a radio button is pressed
    using RadioCallback = RadioButton::ClickCallback;

    /// Called when OpenPrintTag loading fails
    using ErrorCallback = stdext::inplace_function<void()>;

public:
    FrameOPTPrompt(window_frame_t *parent, const string_view_utf8 &title);

    void setup_radio(PhaseResponses responses, const RadioCallback &callback);
    void setup_tag(std::optional<ToolTag> tag, const ErrorCallback &error_callback);

protected:
    void scan();

    void screenEvent(window_t *sender, GUI_event_t event, void *param) override;

private:
    window_text_t title_;
    BasicWindow title_line_;
    window_icon_t spool_image_;
    window_text_t status_text_;
    RadioButton radio_;

    StrongIndexArray<window_text_t, std::to_underlying(Detail::_cnt), Detail, std::to_underlying<Detail>> detail_titles_;
    StrongIndexArray<window_text_t, std::to_underlying(Detail::_cnt), Detail, std::to_underlying<Detail>> detail_values_;

private:
    FieldTraits<MainField::material_name>::Buffer material_name_buffer_;
    FieldTraits<MainField::brand_name>::Buffer brand_name_buffer_;
    FieldTraits<MainField::material_abbreviation>::Buffer abbreviation_buffer_;
    AmountsInfo::WeightStrBuffer weight_buffer_;

private:
    ErrorCallback error_callback_;
    std::optional<ToolTag> tag_;
    bool scan_pending_ = true;
};

}; // namespace buddy::openprinttag
