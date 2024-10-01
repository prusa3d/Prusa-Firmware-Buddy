#pragma once

#include "../nhttp/handler.h"

namespace nhttp::link_content {

class PrusaLinkApiOcto final : public handler::Selector {
public:
    virtual Accepted accept(const handler::RequestParser &parser, handler::Step &out) const override;
};

extern const PrusaLinkApiOcto prusa_link_api_octo;

} // namespace nhttp::link_content
