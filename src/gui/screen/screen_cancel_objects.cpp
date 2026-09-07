#include "screen_cancel_objects.hpp"

#include <marlin_vars.hpp>
#include <marlin_client.hpp>
#include <feature/cancel_object/cancel_object.hpp>
#include <screen_menu.hpp>
#include <window_menu_virtual.hpp>
#include <dynamic_index_mapping.hpp>
#include <WindowMenuSwitch.hpp>
#include <ScreenHandler.hpp>
#include <utils/string_builder.hpp>

using namespace buddy;

namespace {
static constexpr const char *cancel_switch_items[] = {
    N_("Printing"),
    N_("Canceled"),
};

using ObjectID = CancelObject::ObjectID;

class MI_CANCEL_OBJECT final : public MenuItemSwitch {

public:
    MI_CANCEL_OBJECT(ObjectID object)
        : MenuItemSwitch({}, cancel_switch_items) //
    {
        set_object(object, true);
        set_behavior(Behavior::quick_cycle);
    }

    struct CurrentObject {};
    MI_CANCEL_OBJECT(CurrentObject)
        : MenuItemSwitch({}, cancel_switch_items)
        , link_to_current_object_(true) //
    {
        set_object(-1, true);
    }

protected:
    void Loop() override {
        if (link_to_current_object_) {
            set_object(cancel_object().current_object(), false);
        }
    }

    virtual void OnChange(size_t) final {
        marlin_client::set_object_cancelled(object_, get_index());
    }

    // Colorize the extension red/green to better visualise which objects are cancelled
    Color resolved_value_text_color(Color) const override {
        return current_item() ? COLOR_RED : COLOR_GREEN;
    }

private:
    void set_object(ObjectID object, bool force) {
        if (object_ == object && !force) {
            return;
        }

        object_ = object;

        // set_enabled is especially necessary for the "current object" field, as we might currently be outside of any valid object
        set_enabled(cancel_object().is_object_cancellable(object_));
        set_current_item(cancel_object().is_object_cancelled(object_));

        // Update label
        {
            StringBuilder sb(label_buffer_);
            if (link_to_current_object_) {
                sb.append_string_view(_("Current Object"));
                if (object_ >= 0) {
                    sb.append_printf(" (%i)", object_ + 1);
                }

            } else {
                {
                    std::array<char, 8> fmt;
                    StringBuilder(fmt).append_printf("%%%id ", NumericInputConfig::num_digits(buddy::cancel_object().object_count()));
                    sb.append_printf(fmt.data(), object + 1);
                }

                bool has_name = false;
                if (object_ < ObjectID(marlin_vars_t::CANCEL_OBJECTS_NAME_COUNT)) {
                    marlin_vars().cancel_object_names[object_].execute_with([&](const auto &data) {
                        has_name = (data[0] != '\0');
                        sb.append_string(data);
                    });
                }
                if (!has_name) {
                    sb.append_string_view(_("Object"));
                }
            }

            SetLabel(string_view_utf8::MakeRAM(label_buffer_.data()));
            Invalidate(); // SetLabel does not invalidate if you always put in the same pointer
        }
    }

private:
    ObjectID object_ = 0;
    std::array<char, marlin_vars_t::CANCEL_OBJECT_NAME_LEN> label_buffer_;
    bool link_to_current_object_ = false;
};

class WindowMenuCancelObject final : public WindowMenuVirtual {
public:
    WindowMenuCancelObject(window_t *parent, Rect16 rect)
        : WindowMenuVirtual(parent, rect, CloseScreenReturnBehavior::yes) //
    {
        index_mapping.set_section_size<Item::object_i>(cancel_object().object_count());
        setup_items();
    }

public:
    int item_count() const final {
        return index_mapping.total_item_count();
    }

protected:
    void setup_item(ItemVariant &variant, int index) override {
        const auto mapping = index_mapping.from_index(index);
        switch (mapping.item) {

        case Item::return_: {
            variant.emplace<MI_RETURN>();
            break;
        }

        case Item::current_object: {
            variant.emplace<MI_CANCEL_OBJECT>(MI_CANCEL_OBJECT::CurrentObject {});
            break;
        }

        case Item::object_i: {
            variant.emplace<MI_CANCEL_OBJECT>(static_cast<ObjectID>(mapping.pos_in_section));
            break;
        }
        }
    }

private:
    enum class Item {
        return_,
        current_object,
        object_i,
    };
    static constexpr auto mapping_items = std::to_array<DynamicIndexMappingRecord<Item>>({
        { Item::return_ },
        { Item::current_object },
        { Item::object_i, DynamicIndexMappingType::dynamic_section },
    });
    DynamicIndexMapping<mapping_items> index_mapping;
};

class ScreenCancelObjects : public ScreenMenuBase<WindowMenuCancelObject> {
public:
    ScreenCancelObjects()
        : ScreenMenuBase(nullptr, _("CANCEL OBJECTS"), EFooter::On) {}
};

} // namespace

MI_CO_CANCEL_OBJECT::MI_CO_CANCEL_OBJECT()
    : IWindowMenuItem(_("Cancel Object"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
    set_enabled(buddy::cancel_object().object_count() > 0);
}

void MI_CO_CANCEL_OBJECT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->Open(ScreenFactory::Screen<ScreenCancelObjects>);
}
