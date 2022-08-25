#pragma once

#include "printer.hpp"
#include "planner.hpp"

#include <segmented_json.h>

#include <sys/stat.h>

namespace con {

struct RenderState {
    const Printer &printer;
    const Action &action;
    bool has_stat = false;
    struct stat st;
    std::optional<Printer::NetInfo> lan;
    std::optional<Printer::NetInfo> wifi;

    RenderState(const Printer &printer, const Action &action);
};

class Renderer : public json::JsonRenderer<RenderState> {
protected:
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, RenderState &state) const override;

public:
    Renderer(RenderState &&state)
        : JsonRenderer(std::move(state)) {}
};

}
