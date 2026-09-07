/// @file
#pragma once

#include <tool/physical_tool.hpp>
#include <tool/hotend/hotend/dummy_hotend.hpp>

class DummyPhysicalTool final : public PhysicalTool {

public:
    explicit DummyPhysicalTool();

public:
    void filament_compatibility_report(FilamentCompatibilityReport &, const FilamentCompatibilityReportGenerateArgs &) const override {}

private:
    DummyHotend hotend_;
};
