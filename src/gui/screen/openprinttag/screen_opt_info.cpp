#include "screen_opt_info.hpp"
#include "screen_opt_info_private.hpp"

#include <utils/enum_array.hpp>

#include <window_menu_callback_item.hpp>
#include <window_msgbox.hpp>
#include <ScreenHandler.hpp>
#include <feature/openprinttag/requests_read_multi.hpp>
#include <feature/openprinttag/data_utils.hpp>
#include <feature/openprinttag/filament_usage_tracker/filament_usage_tracker.hpp>
#include <string_builder.hpp>
#include <screen/openprinttag/opt_request_wizard.hpp>
#include <screen/openprinttag/screen_opt_filament_detail.hpp>
#include <gui/screen/filament/screen_filament_detail.hpp>
#include <img_resources.hpp>
#include <feature/openprinttag/utils.hpp>
#include <bsod/bsod.h>

namespace buddy::openprinttag {

namespace {

    struct ToolTagStatusData {
        const char *short_text;
        const char *long_text;
        Color color;
    };

    static constexpr EnumArray<ToolTagStatus, ToolTagStatusData, ToolTagStatus::_cnt> status_texts {
        {
            ToolTagStatus::ok,
            ToolTagStatusData {
                .short_text = N_("OK"),
                .long_text = N_("The OpenPrintTag is assigned and present."),
                .color = COLOR_GREEN,
            },
        },
        {
            ToolTagStatus::no_filament,
            ToolTagStatusData {
                .short_text = N_("-"),
                .long_text = nullptr,
                .color = COLOR_LIGHT_GRAY,
            },
        },
        {
            ToolTagStatus::not_assigned,
            ToolTagStatusData {
                .short_text = N_("NOT ASSIGNED"),
                .long_text = N_("The OpenPrintTag is not detected at the tool/slot."),
                .color = COLOR_LIGHT_GRAY,
            },
        },
        {
            ToolTagStatus::not_assigned_but_present,
            ToolTagStatusData {
                .short_text = N_("NOT ASSIGNED"),
                .long_text = N_("The tool/slot does not have an OpenPrintTag assigned, but there is a tag detected. OpenPrintTag must be linked during filament load."),
                .color = COLOR_ORANGE,
            },
        },
        {
            ToolTagStatus::different_tag_present,
            ToolTagStatusData {
                .short_text = N_("WRONG TAG"),
                .long_text = N_("Currently detected OpenPrintTag is different to the assigned one present during filament load."),
                .color = COLOR_RED,
            },
        },
        {
            ToolTagStatus::tag_missing,
            ToolTagStatusData {
                .short_text = N_("TAG MISSING"),
                .long_text = N_("The OpenPrintTag is not detected at the tool/slot."),
                .color = COLOR_ORANGE,
            },
        },
        {
            ToolTagStatus::tag_problem,
            ToolTagStatusData {
                .short_text = N_("TAG PROBLEM"),
                .long_text = N_("The tag is corrupt, locked, missing length/weight data or otherwise unsuitable."),
                .color = COLOR_RED,
            },
        },
    };

} // namespace

MI_OPT_TAG_STATUS::MI_OPT_TAG_STATUS(ScreenOPTInfo &screen)
    : IWindowMenuItem(_("OpenPrintTag Status"))
    , screen_(screen) {
    extension_width = 128;
}

void MI_OPT_TAG_STATUS::Loop() {
    const auto new_status = tool_tag_status(screen_.tool_);
    if (tag_status_ != new_status) {
        tag_status_ = new_status;
        InValidateExtension();
    }
}

void MI_OPT_TAG_STATUS::printExtension(Rect16 extension_rect, [[maybe_unused]] Color color_text, Color color_back, ropfn raster_op) const {
    const auto &status = status_texts[tag_status_];

    const auto arrow_left = extension_rect.Right() - img::arrow_right_10x16.w;

    render_text_align(Rect16::fromLTRB(extension_rect.Left(), extension_rect.Top(), arrow_left - 8, extension_rect.Bottom()), _(status.short_text), Font::small, color_back, status.color, {}, Align_t::RightCenter());

    // Render extension arrow - indicate that clicking shows a dialog
    render_icon_align(Rect16::fromLTRB(arrow_left, extension_rect.Top(), extension_rect.Right(), extension_rect.Bottom()), &img::arrow_right_10x16, color_back, icon_flags(Align_t::Center(), raster_op));
}

void MI_OPT_TAG_STATUS::click(IWindowMenu &) {
    const char *const msg = status_texts[tool_tag_status(screen_.tool_)].long_text;
    if (msg == nullptr) {
        return;
    }

    const auto assigned_tag = ToolTag::for_tool_assigned(screen_.tool_);
    PhaseResponses responses = { Response::Ok, assigned_tag.has_value() ? Response::Disable : Response::_none };

    const auto status_response = MsgBoxError(_(msg), responses);
    switch (status_response) {

    case Response::Ok:
        break;

    case Response::Disable: {
        StringViewUtf8Parameters<4> params;
        const auto prompt = _("Unbind the OpenPrintTag from tool %i?").formatted(params, screen_.tool_.display_index());
        const auto unbind_response = MsgBoxQuestion(prompt, Responses_YesNo);

        if (unbind_response == Response::Yes) {
            config_store().adhoc_filament_assigned_openprinttag.set_to_default(screen_.tool_.to_raw());

            // Rescan, unbdinding the OPT can change the displayed info
            screen_.scan_pending_ = true;
        }
        break;
    }

    default:
        bsod_unreachable();
    }
}

WindowMenuOPTInfo::WindowMenuOPTInfo(window_t *parent, Rect16 rect)
    : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) {
    // setup_items is called from ScreenOTPInfo
}

int WindowMenuOPTInfo::item_count() const {
    return index_mapping_.total_item_count();
}

void WindowMenuOPTInfo::setup_item(ItemVariant &variant, int index) {
    const auto item = index_mapping_.from_index(index);
    switch (item.item) {

    case Item::return_:
        variant.emplace<MI_RETURN>();
        break;

    case Item::data_section:
        data_items_[item.pos_in_section](variant);
        break;

    case Item::opt_tag_status: {
        variant.emplace<MI_OPT_TAG_STATUS>(*screen_);
        break;
    }

    case Item::print_parameters:
        const auto callback = [this] {
            using Mode = ScreenOPTInfo::Mode;

            ScreenFactory::Creator creator = nullptr;

            switch (screen_->mode_) {

            case Mode::ephemeral:
                creator = screen_openprinttag_filament_detail_creator(*screen_->tag_);
                break;

            case Mode::loaded:
                creator = ScreenFactory::ScreenWithArg<ScreenFilamentDetail>(FilamentType::for_tool(screen_->tool_));
                break;
            }

            Screens::Access()->Open(creator);
        };
        variant.emplace<WindowMenuCallbackItem>(_("Printing Parameters"), callback, nullptr, expands_t::yes);
        break;
    }
}

ScreenOPTInfo::ScreenOPTInfo(CtorArgs args)
    : ScreenMenuBase(nullptr, _(args.mode == Mode::loaded ? N_("LOADED FILAMENT") : N_("OPENPRINTTAG INFO")), EFooter::Off)
    , mode_(args.mode)
    , tool_(args.tool) {

    menu.menu.screen_ = this;
    menu.menu.setup_items();

    // First scan needs to be delayed (cannot be in the constructor)
    scan_pending_ = true;
}

void ScreenOPTInfo::screenEvent([[maybe_unused]] window_t *sender, GUI_event_t event, [[maybe_unused]] void *param) {
    switch (event) {

    case GUI_event_t::LOOP:
        if (scan_pending_) {
            scan_pending_ = false;
            scan();
        }
        break;

    default:
        break;
    }

    ScreenMenuBase::screenEvent(sender, event, param);
}

bool ScreenOPTInfo::scan() {
    using MultiRequest = MultiReadFieldRequest<
        MainField::material_name,
        MainField::brand_name,
        AmountsInfo::Requirements {},
        AbbreviationInfo::Requirements {}>;

    const auto ephemeral_tag = ToolTag::for_tool_ephemeral(tool_);

    switch (mode_) {

    case Mode::ephemeral:
        tag_ = ephemeral_tag;
        if (!tag_) {
            StringViewUtf8Parameters<4> fmt;
            MsgBoxError(_("No OpenPrintTag detected for slot %i").formatted(fmt, tool_.display_index()), Responses_Ok);
            close_screen();
            return false;
        }
        break;

    case Mode::loaded:
        // Can be null - in that case, some info will be hidden
        tag_ = ToolTag::for_tool_assigned(tool_);
        break;
    }

    // value_or can provide any value whatsoever, we will not be issuing the request anyway
    MultiRequest req { ephemeral_tag.value_or(ToolTag { tool_, 1 }) };

    if (ephemeral_tag.has_value() && ephemeral_tag == tag_) {
        if (!multirequest_with_troubleshooting(req)) {
            close_screen();
            return false;
        }
    } else {
        // Mark the request as failed to prevent assert(finished) in .result()
        req.fail();
    }

    AmountsInfo amounts { req };
    AbbreviationInfo type { req };

    // In the loaded mode, data in memory have precedence over what's in the tag
    if (mode_ == Mode::loaded) {
        const auto params = FilamentType::for_tool(tool_).parameters();

        // "Fake" abbreviation from filament parameters
        type.abbreviation = std::string_view {
            type.abbreviation_buffer.begin(),
            std::string_view { params.name }.copy(type.abbreviation_buffer.data(), type.abbreviation_buffer.size())
        };
    }

    auto &menu = this->menu.menu;

    menu.data_items_.clear();

    if (auto val = req.result<MainField::material_name>()) {
        add_string_item(N_("Material Name"), *val, material_name_buffer_);
    }

    if (auto val = req.result<MainField::brand_name>()) {
        add_string_item(N_("Brand"), *val, brand_name_buffer_);
    }

    if (!type.abbreviation.empty()) {
        const std::string_view abbr = type.abbreviation;
        const auto default_abbr = req.result<MainField::material_type>().transform([](auto item) { return ::openprinttag::enum_item_name(item); });

        StringBuilder sb(abbreviation_buffer_);
        sb.append_printf("%.*s", abbr.size(), abbr.data());
        if (default_abbr.has_value() && default_abbr != type.abbreviation) {
            sb.append_printf(" (%.*s)", default_abbr->size(), default_abbr->data());
        }

        add_string_item(N_("Abbreviation"), sb.str(), abbreviation_buffer_);
    }

    if (amounts.full_weight_g.has_value() && amounts.remaining_weight_g.has_value() && amounts.full_weight_g != amounts.remaining_weight_g) {
        add_fmt_item<N_("Remaining Weight"), "%.0f/%.0f g"_tstr, float, float>(*amounts.remaining_weight_g, *amounts.full_weight_g);
    } else if (amounts.full_weight_g.has_value()) {
        add_fmt_item<N_("Full Weight"), "%.0f g"_tstr, float>(*amounts.full_weight_g);
    }

    if (amounts.full_length_mm.has_value() && amounts.remaining_length_mm.has_value() && std::abs(*amounts.full_length_mm - *amounts.remaining_length_mm) > 5) {
        add_fmt_item<N_("Remaining Length"), "%.0f/%.0f m"_tstr, float, float>(*amounts.remaining_length_mm / 1000, *amounts.full_length_mm / 1000);
    } else if (amounts.full_length_mm.has_value()) {
        add_fmt_item<N_("Full Length"), "%.0f m"_tstr, float>(*amounts.full_length_mm / 1000);
    }

    // !!! When adding new add_fmt_item calls, make sure that there's enough capacity in data_items_ to accomodate them all

    menu.index_mapping_.set_section_size<Item::data_section>(menu.data_items_.size());
    menu.setup_items();
    return true;
}

void ScreenOPTInfo::add_string_item(const char *label, std::string_view val, std::span<char> buffer) {
    // Copy the text to the provided buffer - the val likely comes from ReadFieldRequest, which is on the stack and will get destroyed
    const std::string_view cpy { buffer.data(), val.copy(buffer.data(), buffer.size()) };
    menu.menu.data_items_.push_back([label, cpy](ItemVariant &iv) {
        iv.emplace<MenuItemInfo>(_(label), cpy);
    });
}

template <auto label, auto fmt, typename... Args>
void ScreenOPTInfo::add_fmt_item(std::type_identity_t<Args>... args) {
    menu.menu.data_items_.push_back([args...](ItemVariant &iv) {
        MenuItemInfo::Buffer buf;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdouble-promotion"
        snprintf(buf.data(), buf.size(), fmt, args...);
#pragma GCC diagnostic pop
        iv.emplace<MenuItemInfo>(_(label), buf.data());
    });
}

ScreenFactory::Creator screen_opt_info_ephemeral_creator(VirtualToolIndex for_tool) {
    return ScreenFactory::ScreenWithArg<ScreenOPTInfo>(ScreenOPTInfo::CtorArgs { .tool = for_tool, .mode = ScreenOPTInfo::Mode::ephemeral });
}

ScreenFactory::Creator screen_opt_info_loaded_creator(VirtualToolIndex for_tool) {
    return ScreenFactory::ScreenWithArg<ScreenOPTInfo>(ScreenOPTInfo::CtorArgs { .tool = for_tool, .mode = ScreenOPTInfo::Mode::loaded });
}

} // namespace buddy::openprinttag
