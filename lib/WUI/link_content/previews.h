#pragma once

#include "../nhttp/handler.h"

namespace nhttp::link_content {

class Previews final : public handler::Selector {
public:
    virtual std::optional<handler::ConnectionState> accept(const handler::RequestParser &parser) const override;
};

extern const Previews previews;

} // namespace nhttp::link_content
