#pragma once

#include "../nhttp/handler.h"

namespace nhttp::link_content {

class Previews final : public handler::Selector {
public:
    virtual Accepted accept(const handler::RequestParser &parser, handler::Step &out) const override;
};

extern const Previews previews;

} // namespace nhttp::link_content
