/*****************************************************************************/
// print related menu items
#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"
#include <guiconfig/guiconfig.h>
#include <option/has_indx.h>
#include <WindowItemFormatableLabel.hpp>

/// Nozzle target temperature (adjustable spin)
class MI_NOZZLE_TARGET_TEMP : private NumericInputConfigHolder, public WiSpin {

public:
    MI_NOZZLE_TARGET_TEMP(std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool = CurrentlySelectedTool {});

protected:
    void OnClick() override;
    void Loop() override;

private:
    const std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool_;

    /// Resolved tool that is updated in Loop(). Cached so that a tool change mid-edit doesn't write to a wrong tool.
    std::optional<PhysicalToolIndex> resolved_tool_;

    StringViewUtf8Parameters<4> label_params_;
};

/// Nozzle current temperature (read only, auto updating)
class MI_INFO_NOZZLE_TEMP : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_NOZZLE_TEMP(std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool = CurrentlySelectedTool {});

    float value() const;

private:
    StringViewUtf8Parameters<4> label_params_;
    const std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool_;
};

#if HAS_TEMP_HEATBREAK
/// Heatbreak current temperature (read only, auto updating)
class MI_INFO_HEATBREAK_TEMP : public MenuItemAutoUpdatingLabel<float> {
public:
    MI_INFO_HEATBREAK_TEMP(std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool = CurrentlySelectedTool {});

    float value() const;

private:
    StringViewUtf8Parameters<4> label_params_;
    const std::variant<PhysicalToolIndex, CurrentlySelectedTool> tool_;
};
#endif

class MI_HEATBED : public WiSpin {
    constexpr static const char *label =
#if HAS_MINI_DISPLAY()
        N_("Heatbed");
#else
        N_("Heatbed Temperature");
#endif

public:
    MI_HEATBED();
    virtual void OnClick() override;
};

class MI_PRINTFAN : public WiSpin {
    constexpr static const char *label =
#if HAS_MINI_DISPLAY()
        N_("Print Fan");
#else
        N_("Print Fan Speed");
#endif

public:
    MI_PRINTFAN();
    virtual void OnClick() override;
};

#if HAS_INDX()
class MI_DOCKFAN : public WiSpin {
    constexpr static const char *label = N_("Dock Fan");

public:
    MI_DOCKFAN();
    virtual void OnClick() override;
};
#endif

class MI_SPEED : public WiSpin {
    constexpr static const char *label = N_("Print Speed");

public:
    MI_SPEED();
    virtual void OnClick() override;
};

/// Flow factor (adjustable spin)
class MI_FLOW_FACTOR : public WiSpin {

public:
    MI_FLOW_FACTOR(std::variant<VirtualToolIndex, CurrentlySelectedTool> tool = CurrentlySelectedTool {});

protected:
    void OnClick() override;
    void Loop() override;

private:
    const std::variant<VirtualToolIndex, CurrentlySelectedTool> tool_;

    /// Resolved tool that is updated in Loop(). Cached so that a tool change mid-edit doesn't write to a wrong tool.
    std::optional<VirtualToolIndex> resolved_tool_;

    StringViewUtf8Parameters<4> label_params_;
};
