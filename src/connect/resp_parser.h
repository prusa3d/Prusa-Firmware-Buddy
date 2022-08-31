#pragma once

#include <automata/core.h>
#include <http/types.h>

namespace con::parser {

class ResponseParser final : public automata::Execution {
private:
    virtual automata::ExecutionControl event(automata::Event event) override;

public:
    uint16_t status_code = 0;
    std::optional<size_t> content_length;
    std::optional<uint32_t> command_id;
    http::ContentType content_type = http::ContentType::ApplicationOctetStream;
    // We have a connection header (value set) and its to keep-alive (true) or not.
    std::optional<bool> keep_alive;
    // http version parsed
    uint8_t version_major : 2;
    uint8_t version_minor : 4;

    bool done = false;
    ResponseParser();
};

}
