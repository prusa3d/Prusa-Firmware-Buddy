#pragma once

#include "../nhttp/handler.h"

namespace nhttp::link_content {

class PrusaLinkApiV1 final : public handler::Selector {
public:
    virtual Accepted accept(const handler::RequestParser &parser, handler::Step &out) const override;
};

extern const PrusaLinkApiV1 prusa_link_api_v1;

} // namespace nhttp::link_content
