#pragma once

#include <segmented_json.h>
#include <transfers/monitor.hpp>
#include <http/types.h>

namespace nhttp::handler {

class TransferState {
public:
    transfers::TransferId transfer_id;
    TransferState(transfers::TransferId id)
        : transfer_id(id) {}
};

class TransferRenderer final : public json::JsonRenderer<TransferState> {
private:
    http::APIVersion api;

    json::JsonResult renderStateOctoprint(size_t resume_point, json::JsonOutput &output, TransferState &state) const;
    json::JsonResult renderStateV1(size_t resume_point, json::JsonOutput &output, TransferState &state) const;

public:
    TransferRenderer(transfers::TransferId id, http::APIVersion api)
        : JsonRenderer(TransferState(id))
        , api(api) {}
    virtual json::JsonResult renderState(size_t resume_point, json::JsonOutput &output, TransferState &state) const override;
};
} // namespace nhttp::handler
