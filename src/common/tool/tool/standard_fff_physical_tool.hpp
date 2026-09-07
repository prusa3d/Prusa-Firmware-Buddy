/// @file
#pragma once

#include <tool/physical_tool.hpp>

class StandardFFFPhysicalToolBase : public PhysicalTool {

public:
#if BOARD_IS_MASTER_BOARD()
    void filament_compatibility_report(FilamentCompatibilityReport &report, const FilamentCompatibilityReportGenerateArgs &args) const override;
#endif

protected:
    explicit StandardFFFPhysicalToolBase(PhysicalToolIndex tool_index, Hotend &hotend);

protected:
    const PhysicalToolIndex tool_index_;
};

template <typename Hotend>
class StandardFFFPhysicalTool final : public StandardFFFPhysicalToolBase {

public:
    explicit StandardFFFPhysicalTool(PhysicalToolIndex tool_index, const Hotend::Config *hotend_config)
        : StandardFFFPhysicalToolBase(tool_index, hotend_)
        , hotend_(tool_index, hotend_config) {}

    Hotend &hotend() { return hotend_; }

private:
    Hotend hotend_;
};
