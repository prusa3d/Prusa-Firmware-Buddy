#pragma once

#include <http/resp_parser.h>

#include <optional>

namespace connect_client {

class ExtractCommanId : public http::ExtraHeader {
public:
    std::optional<uint32_t> command_id = std::nullopt;
    virtual void character(char c, http::HeaderName name) override;
};

} // namespace connect_client
