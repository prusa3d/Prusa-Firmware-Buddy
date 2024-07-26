#pragma once

#include <type_traits>

#include <option/has_toolchanger.h>

#include <utils/algorithm_extensions.hpp>

#include <gui/widget/window_toggle_switch.hpp>
#include <screen_menu.hpp>
#include <WindowMenuSpin.hpp>
#include <MItem_tools.hpp>

#if HAS_TOOLCHANGER()
    #include <module/prusa/toolchanger.h>
#endif

namespace screen_toolhead_settings {

/// Applies settings to all toolheads
using AllToolheads = std::monostate;
using ToolheadIndex = uint8_t;
using Toolhead = std::variant<ToolheadIndex, AllToolheads>;

static constexpr ToolheadIndex toolhead_count = HOTENDS;
static constexpr Toolhead default_toolhead = ToolheadIndex(0);
static constexpr Toolhead all_toolheads = AllToolheads();

template <typename T>
concept CMI_TOOLHEAD_SPECIFIC = requires(T a) {
    { a.set_toolhead(Toolhead()) };
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
template <typename Child, typename Parent>
class MI_TOOLHEAD_SPECIFIC : public Parent {

public:
    template <typename... Args>
    inline MI_TOOLHEAD_SPECIFIC(Toolhead toolhead, Args &&...args)
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
        if constexpr (UpdatableMenuItem<Child>) {
            static_cast<Child *>(this)->update();
        }
    }

protected:
    /// \returns \p Child::read_value_impl(toolhead())
    /// If \p toolhead is \p all_toolheads, returns the value only if all toolheads have the same value. Otherwise returns \p nullopt.
    template <typename = void>
    auto read_value() {
        using Value = std::remove_cvref_t<decltype(std::declval<Child *>()->read_value_impl(0))>;
        using Result = std::optional<Value>;

#if HAS_TOOLCHANGER()
        if (toolhead_ == all_toolheads) {
            Result result;

            for (ToolheadIndex i = 0; i < toolhead_count; i++) {
                if (prusa_toolchanger.is_tool_enabled(i)) {
                    const auto val = static_cast<Child *>(this)->read_value_impl(i);
                    if (result.has_value() && *result != val) {
                        return Result(std::nullopt);
                    }
                    result = val;
                }
            }

            return result;
        } else
#endif
        {
            return Result(static_cast<Child *>(this)->read_value_impl(std::get<ToolheadIndex>(toolhead_)));
        }
    }

    /// Stores the value in the config store. If \p toolhead is \p all_toolheads, sets value for all toolheads
    /// Expects \p Child to have \p store_value_impl(ToolheadIndex)
    template <typename T>
    void store_value(const T &value) {
#if HAS_TOOLCHANGER()
        if (toolhead_ == all_toolheads) {
            for (ToolheadIndex i = 0; i < toolhead_count; i++) {
                static_cast<Child *>(this)->store_value_impl(i, value);
            }
        } else
#endif
        {
            static_cast<Child *>(this)->store_value_impl(std::get<ToolheadIndex>(toolhead_), value);
        }
    }

protected:
    /// Stores whether the user has previously confirmed msgbox_confirm_change for this item, to prevent repeating the confirmation on subsequent changes
    bool user_already_confirmed_changes_ = false;

private:
    Toolhead toolhead_;
};

template <typename Child>
class MI_TOOLHEAD_SPECIFIC_SPIN : public MI_TOOLHEAD_SPECIFIC<Child, WiSpin> {
public:
    // Inherit parent constructor
    using MI_TOOLHEAD_SPECIFIC<Child, WiSpin>::MI_TOOLHEAD_SPECIFIC;

    void update() {
        this->set_value(this->template read_value().value_or(*this->config().special_value));
    }

    void OnClick() final {
        if (this->value() == this->config().special_value) {
            return;
        }

        if (msgbox_confirm_change(this->toolhead(), this->user_already_confirmed_changes_)) {
            this->template store_value(this->value());
        } else {
            update();
        }
    }
};

template <typename Child>
class MI_TOOLHEAD_SPECIFIC_TOGGLE : public MI_TOOLHEAD_SPECIFIC<Child, WindowToggleSwitch> {
public:
    // Inherit parent constructor
    using MI_TOOLHEAD_SPECIFIC<Child, WindowToggleSwitch>::MI_TOOLHEAD_SPECIFIC;

    void update() {
        this->set_value(this->template read_value().transform(Tristate::from_bool).value_or(Tristate::other), false);
    }

    void toggled(Tristate) final {
        if (this->value() == Tristate::other) {
            return;
        }

        if (msgbox_confirm_change(this->toolhead(), this->user_already_confirmed_changes_)) {
            this->template store_value(this->value());
        } else {
            update();
        }
    }
};

} // namespace screen_toolhead_settings
