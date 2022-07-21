#pragma once

#include "core_interface.hpp"
#include "planner.hpp"

#include <segmented_json.h>

namespace con {

struct RenderState {
    // TODO: These structs feel a bit awkward. We are shuffling stuff from marlin_vars for no apparent reason?
    const printer_info_t &info;
    device_params_t params;
    const Action &action;
};

class Renderer : public json::JsonRenderer<RenderState> {
protected:
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, RenderState &state) const override;

public:
    Renderer(RenderState &&state)
        : JsonRenderer(std::move(state)) {}
};

}
