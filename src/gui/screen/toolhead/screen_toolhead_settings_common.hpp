#pragma once

#include <type_traits>

#include <option/has_toolchanger.h>

#include <utils/algorithm_extensions.hpp>

#include <gui/menu_item/menu_item_toggle_switch.hpp>
#include <screen_menu.hpp>
#include <WindowMenuSpin.hpp>
#include <MItem_tools.hpp>
#include <tool_index.hpp>

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

namespace screen_toolhead_settings {

using Toolhead = std::variant<PhysicalToolIndex, AllTools>;

static constexpr Toolhead default_toolhead = PhysicalToolIndex::from_raw(0);
static constexpr Toolhead all_toolheads = AllTools {};

template <typename T>
concept CMI_TOOLHEAD_SPECIFIC = requires(T &a, Toolhead t) {
    { a.set_toolhead(t) };
};

template <typename Container>
void menu_set_toolhead(Container &container, Toolhead toolhead) {
    stdext::visit_tuple(container.menu_items, [toolhead]<typename T>(T &item) {
        if constexpr (CMI_TOOLHEAD_SPECIFIC<T>) {
            item.set_toolhead(toolhead);
        }
    });
}

/// Shows a message box confirming that the change will be done on all tools (if toolhead == all_tools).
/// \returns if the user agrees with the change
bool msgbox_confirm_change(Toolhead toolhead, bool &confirmed_before);

/// Curiously recurring parent for all menu items that are per toolhead
template <typename Parent>
class MI_TOOLHEAD_SPECIFIC_BASE : public Parent {

public:
    template <typename... Args>
    inline MI_TOOLHEAD_SPECIFIC_BASE(Toolhead toolhead, Args &&...args)
        : Parent(std::forward<Args>(args)...)
        , toolhead_(toolhead) {}

    inline Toolhead toolhead() const {
        return toolhead_;
    }

    void set_toolhead(Toolhead set) {
        if (toolhead_ == set) {
            return;
        }
        toolhead_ = set;
        update();
    }

    virtual void update() = 0;

private:
    Toolhead toolhead_;
};

/// Curiously recurring parent for all menu items that are per toolhead
template <typename Value_, typename Parent>
class MI_TOOLHEAD_SPECIFIC : public MI_TOOLHEAD_SPECIFIC_BASE<Parent> {

public:
    // Inherit parent constructors
    using MI_TOOLHEAD_SPECIFIC_BASE<Parent>::MI_TOOLHEAD_SPECIFIC_BASE;

    using Value = Value_;

protected:
    /// \returns \p Child::read_value_impl(toolhead())
    /// If \p toolhead is \p all_toolheads, returns the value only if all toolheads have the same value. Otherwise returns \p nullopt.
    std::optional<Value> read_value() {
#if HAS_TOOLCHANGER()
        if (this->toolhead() == all_toolheads) {
            std::optional<Value> result;

            for (auto tool : PhysicalToolIndex::all()) {
                if (tool.is_enabled()) {
                    const auto val = read_value_impl(tool);
                    if (result.has_value() && *result != val) {
                        return std::nullopt;
                    }
                    result = val;
                }
            }

            return result;
        } else
#endif
        {
            return read_value_impl(std::get<PhysicalToolIndex>(this->toolhead()));
        }
    }

    /// Stores the value in the config store. If \p toolhead is \p all_toolheads, sets value for all toolheads
    /// Expects \p Child to have \p store_value_impl(PhysicalToolIndex)
    void store_value(Value value) {
#if HAS_TOOLCHANGER()
        if (this->toolhead() == all_toolheads) {
            for (auto i : PhysicalToolIndex::all()) {
                store_value_impl(i, value);
            }
        } else
#endif
        {
            store_value_impl(std::get<PhysicalToolIndex>(this->toolhead()), value);
        }
    }

    virtual Value read_value_impl(PhysicalToolIndex toolhead) = 0;
    virtual void store_value_impl(PhysicalToolIndex index, Value value) = 0;

protected:
    /// Stores whether the user has previously confirmed msgbox_confirm_change for this item, to prevent repeating the confirmation on subsequent changes
    bool user_already_confirmed_changes_ = false;
};

class MI_TOOLHEAD_SPECIFIC_SPIN : public MI_TOOLHEAD_SPECIFIC<float, WiSpin> {
public:
    // Inherit parent constructor
    using MI_TOOLHEAD_SPECIFIC<float, WiSpin>::MI_TOOLHEAD_SPECIFIC;

    void update() final;

    void OnClick() final;
};

class MI_TOOLHEAD_SPECIFIC_TOGGLE : public MI_TOOLHEAD_SPECIFIC<bool, MenuItemToggleSwitch> {
public:
    // Inherit parent constructor
    using MI_TOOLHEAD_SPECIFIC<bool, MenuItemToggleSwitch>::MI_TOOLHEAD_SPECIFIC;

    void update() final;

    void toggled(Tristate) final;
};

} // namespace screen_toolhead_settings
