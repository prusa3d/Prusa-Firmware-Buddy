/// @file
#include "frame_opt_prompt.hpp"

#include <array>

#include <img_resources.hpp>
#include <feature/openprinttag/data_utils.hpp>
#include <screen/openprinttag/screen_opt_info.hpp>
#include <screen/openprinttag/opt_request_wizard.hpp>
#include <gui/auto_layout.hpp>
#include <ScreenHandler.hpp>
#include <utils/enum_array.hpp>

static_assert(HAS_LARGE_DISPLAY(), "This frame has been only done for a large display");

namespace buddy::openprinttag {

using Detail = FrameOPTPrompt::Detail;

namespace {
    constexpr std::array layout {
        StackLayoutItem { .height = 32, .margin_side = 16, .margin_bottom = 4 },
        StackLayoutItem { .height = 2, .margin_side = 16, .margin_bottom = 16 },
        StackLayoutItem { .height = StackLayoutItem::stretch, .margin_side = 8 },
        StackLayoutItem { .height = 32, .margin_side = 8, .margin_bottom = 4 },
        standard_stack_layout::for_radio,
    };

    constexpr EnumArray<Detail, const char *, Detail::_cnt> detail_texts {
        { Detail::material_name, N_("Name") },
        { Detail::brand_name, N_("Brand") },
        { Detail::abbreviation, N_("Type") },
        { Detail::weight, N_("Weight") },
    };

} // namespace

FrameOPTPrompt::FrameOPTPrompt(window_frame_t *parent, const string_view_utf8 &title)
    : window_frame_t { parent, GuiDefaults::RectScreenNoHeader }
    , title_(this, Rect16 {}, title, is_multiline::no)
    , title_line_(this, Rect16 {})
    , spool_image_(this, Rect16 {}, &img::prusament_spool_white_100x100)
    , status_text_(this, Rect16 {}, is_multiline::no)
    , radio_(this, Rect16 {}) {

    title_.set_font(GuiDefaults::FontBig);
    title_.SetTextColor(COLOR_BRAND);
    title_.SetAlignment(Align_t::LeftBottom());

    title_line_.SetBackColor(COLOR_DARK_GRAY);

    status_text_.SetAlignment(Align_t::Center());
    status_text_.set_font(Font::small);
    status_text_.set_enabled(false);

    parent->CaptureNormalWindow(*this);
    CaptureNormalWindow(radio_);

    auto windows = std::to_array<window_t *>({
        &title_,
        &title_line_,
        &spool_image_,
        &status_text_,
        &radio_,
    });
    layout_vertical_stack(GetRect(), windows, layout);

    // Layout the data content
    {
        // Spool image was layouted as if it was the whole content - grab its rect and resize it accordingly
        const auto content_rect = spool_image_.GetRect();
        const auto spool_img = spool_image_.resource();
        spool_image_.SetRect(Rect16::fromLTWH(content_rect.Left() + 16, content_rect.Top(), spool_img->w, spool_img->h));

        const int16_t titles_x = spool_image_.GetRect().Right() + 24;
        const int16_t values_x = titles_x + 82;
        const int16_t row_height = 28;
        int16_t y = content_rect.Top();

        for (size_t i = 0; i < std::to_underlying(Detail::_cnt); i++) {
            const Detail detail = static_cast<Detail>(i);
            int16_t row_bottom = y + row_height;

            auto &title = detail_titles_[detail];
            auto &value = detail_values_[detail];

            if (detail == Detail::material_name) {
                // Keep rows for the material name
                row_bottom += 20;
                value.set_is_multiline(true);
            }

            title.SetText(_(detail_texts[detail]));
            title.SetTextColor(COLOR_LIGHT_GRAY);
            title.set_font(Font::small);
            title.SetRect(Rect16::fromLTRB(titles_x, y, values_x, row_bottom));

            value.SetTextColor(COLOR_WHITE);
            value.SetRect(Rect16::fromLTRB(values_x, y, content_rect.Right(), row_bottom));
            value.set_check_overflow(false);

            for (auto *wnd : { &title, &value }) {
                wnd->SetAlignment(Align_t::LeftTop());
                wnd->set_enabled(false);
                RegisterSubWin(*wnd);
            }

            y = row_bottom;
        }
    }
}

void FrameOPTPrompt::setup_radio(PhaseResponses responses, const RadioCallback &callback) {
    radio_.Change(responses);
    radio_.SetBtnIndex(0);
    radio_.set_callback(callback);
}

void FrameOPTPrompt::setup_tag(std::optional<ToolTag> tag, const ErrorCallback &error_callback) {
    tag_ = tag;
    error_callback_ = error_callback;
}

void FrameOPTPrompt::scan() {
    using MultiRequest = MultiReadFieldRequest<
        MainField::material_name,
        MainField::brand_name,
        AmountsInfo::Requirements {},
        AbbreviationInfo::Requirements {}>;

    MultiRequest req { *tag_ };

    if (!multirequest_with_troubleshooting(req)) {
        error_callback_();
        return;
    }

    static constexpr auto set_text = [](const string_view_utf8 &text, window_text_t &window) {
        window.SetText(text);
        window.auto_select_font(Font::normal);
        window.Invalidate();
    };

    static constexpr auto copy_str_and_set = [](const std::string_view &str, std::span<char> buffer, window_text_t &window) {
        const auto sz = str.copy(buffer.data(), buffer.size());
        set_text(string_view_utf8::MakeRAM(std::string_view { buffer.data(), sz }), window);
    };

    static constexpr auto string_value = [](const ReadStringRequestBase &req, std::span<char> buffer, window_text_t &window) {
        const auto r = req.result();
        if (!r.has_value()) {
            window.SetText({});
            return;
        }
        copy_str_and_set(*r, buffer, window);
    };

    string_value(req.request<MainField::material_name>(), material_name_buffer_, detail_values_[Detail::material_name]);
    string_value(req.request<MainField::brand_name>(), brand_name_buffer_, detail_values_[Detail::brand_name]);
    copy_str_and_set(AbbreviationInfo { req }.abbreviation, abbreviation_buffer_, detail_values_[Detail::abbreviation]);

    {
        StringBuilder sb { weight_buffer_ };
        AmountsInfo { req }.build_weight_str(sb);
        set_text(string_view_utf8::MakeRAM(sb.str()), detail_values_[Detail::weight]);
    }

    // TODO something with status text
}

void FrameOPTPrompt::screenEvent(window_t *sender, GUI_event_t event, void *param) {
    switch (event) {

    case GUI_event_t::LOOP:
        if (tag_.has_value()) {
            // If the tag is no longer present, close the screen
            if (ToolTag::for_tool_ephemeral(tag_->tool()) != tag_) {
                error_callback_();
                return;
            }

            if (scan_pending_) {
                scan_pending_ = false;
                scan();
            }
        }
        break;

    default:
        break;
    }

    window_frame_t::screenEvent(sender, event, param);
}

} // namespace buddy::openprinttag
