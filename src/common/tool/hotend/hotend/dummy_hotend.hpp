/// @file
#pragma once

#include <tool/hotend/hotend.hpp>

/// Represents a hotend that does nothing at all
/// Used for NoTool hotends
class DummyHotend final : public Hotend {

public:
    explicit DummyHotend();

    void filament_compatibility_report(FilamentCompatibilityReport &, const FilamentCompatibilityReportGenerateArgs &) const override {}

    void set_nozzle_target_temp([[maybe_unused]] TargetTemperature set) override {}

#if HAS_TEMP_HEATBREAK_CONTROL
    virtual void set_heatbreak_target_temp([[maybe_unused]] TargetTemperature set) override {};
#endif

protected:
    virtual void manage() override {}
};
