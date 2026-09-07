/// @file
#pragma once

#include <inplace_vector.hpp>
#include <inplace_function.hpp>

#include <screen_menu.hpp>
#include <tool_index.hpp>
#include <WindowMenuInfo.hpp>
#include <window_menu_virtual.hpp>
#include <dynamic_index_mapping.hpp>

#include <feature/openprinttag/detail/defines.hpp>
#include <feature/openprinttag/tool_tag.hpp>
#include <feature/openprinttag/utils.hpp>

namespace buddy::openprinttag {

namespace {

    class ScreenOPTInfo;

    enum class Item {
        return_,
        data_section,
        opt_tag_status,
        print_parameters,
    };

    static constexpr auto index_mapping_items = std::to_array<DynamicIndexMappingRecord<Item>>({
        Item::return_,
        { Item::data_section, DynamicIndexMappingType::dynamic_section, 0 },
        Item::opt_tag_status,
        Item::print_parameters,
    });

    class MI_OPT_TAG_STATUS final : public IWindowMenuItem {

    public:
        MI_OPT_TAG_STATUS(ScreenOPTInfo &screen);

    protected:
        void Loop() final;
        void click(IWindowMenu &) final;
        void printExtension(Rect16 extension_rect, Color color_text, Color color_back, ropfn raster_op) const final;

    private:
        ScreenOPTInfo &screen_;
        ToolTagStatus tag_status_ = ToolTagStatus::_cnt;
    };

    /// A bit longer than standard WI_Info_t - it was awkward that "PLA Prusa Galaxy Blac" was cropped
    using MenuItemInfo = WiInfo<32>;

    class WindowMenuOPTInfo final : public WindowMenuVirtual {
        friend class ScreenOPTInfo;

    public:
        WindowMenuOPTInfo(window_t *parent, Rect16 rect);

        int item_count() const final;

    protected:
        void setup_item(ItemVariant &variant, int index) final;

    protected:
        using ItemVariant = WindowMenuOPTInfo::ItemVariant;
        using ItemConstructor = stdext::inplace_function<void(ItemVariant &), 12>;
        stdext::inplace_vector<ItemConstructor, 16> data_items_;

        DynamicIndexMapping<index_mapping_items> index_mapping_;

        ScreenOPTInfo *screen_ = nullptr;
    };

    /// Screen that scans an OpenPrintTag and displays information present on the tag
    class ScreenOPTInfo final : public ScreenMenuBase<WindowMenuOPTInfo> {
        friend class WindowMenuOPTInfo;
        friend class MI_OPT_TAG_STATUS;
        using ItemVariant = WindowMenuOPTInfo::ItemVariant;

    public:
        enum class Mode : uint8_t {
            ephemeral,
            loaded,
        };

        struct CtorArgs {
            VirtualToolIndex tool;
            Mode mode;
        };

        /// @param tool what antenna/reader to use for scanning
        ScreenOPTInfo(CtorArgs args);

        /// Pops up a wait dialog and scans the tag for data.
        /// Then updates the data on the screen.
        /// Blocks till the scan finishes.
        /// Closes the screen on failure.
        /// @returns whether the scan was successful or not
        bool scan();

        virtual void InitState(screen_init_variant) override {
            // Do not restore state. The number of items dynamically changes from 0 after the first scan()
            // So trying to select menu items right after construction would only trigger asserts
        }

    protected:
        void screenEvent(window_t *sender, GUI_event_t event, void *param) override;

    private:
        void add_string_item(const char *label, std::string_view val, std::span<char> buffer);

        /// Note: The type_identity is there to enforce specyfing the types in the template list.
        /// This is so that you accidentally don't pass int to a float formatter
        template <auto label, auto fmt, typename... Args>
        void add_fmt_item(std::type_identity_t<Args>... args);

    private:
        /// If true, scan() will be called on the next loop event
        bool scan_pending_ = false;

        const Mode mode_;

        VirtualToolIndex tool_;
        std::optional<buddy::openprinttag::ToolTag> tag_;

        openprinttag::FieldTraits<MainField::material_name>::Buffer material_name_buffer_;
        openprinttag::FieldTraits<MainField::brand_name>::Buffer brand_name_buffer_;
        std::array<char, 32> abbreviation_buffer_;
    };

} // namespace

} // namespace buddy::openprinttag
