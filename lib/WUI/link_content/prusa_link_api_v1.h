#pragma once

#include "../nhttp/handler.h"

namespace nhttp::link_content {

class PrusaLinkApiV1 final : public handler::Selector {
public:
    virtual std::optional<handler::ConnectionState> accept(const handler::RequestParser &parser) const override;
};

extern const PrusaLinkApiV1 prusa_link_api_v1;

} // namespace nhttp::link_content
