#pragma once

#include "segmented_json.h"

#include <transfers/monitor.hpp>

namespace nhttp::handler {
class StatusState {
public:
    std::optional<transfers::TransferId> transfer_id { std::nullopt };
    StatusState(std::optional<transfers::TransferId> id)
        : transfer_id(id) {}
};

class StatusRenderer final : public json::JsonRenderer<StatusState> {
public:
    StatusRenderer(std::optional<transfers::TransferId> id)
        : JsonRenderer(StatusState(id)) {}
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, StatusState &state) const override;
};
} // namespace nhttp::handler
