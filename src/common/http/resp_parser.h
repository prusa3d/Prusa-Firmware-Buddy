#pragma once

#include <automata/core.h>
#include "types.h"

namespace http {

enum class HeaderName {
    CommandId,
    Code,
    Token,
    WebSocketAccept,
    WebSocketUpgrade,
    ConnectionUpgrade,
    WebSocketPrusaConnect,
};

class ExtraHeader {
public:
    virtual ~ExtraHeader() = default;
    virtual void character(char c, HeaderName name) = 0;
};

namespace parser {

    class ResponseParser final : public automata::Execution {
    private:
        virtual automata::ExecutionControl event(automata::Event event) override;
        ExtraHeader *extra_hdr;
        void extra(char c, HeaderName name);

    public:
        uint16_t status_code = 0;
        std::optional<size_t> content_length;
        http::ContentType content_type = http::ContentType::ApplicationOctetStream;
        // We have a connection header (value set) and its to keep-alive (true) or not.
        std::optional<bool> keep_alive;
        // We got a Content-Encryption-Mode header
        std::optional<ContentEncryptionMode> content_encryption_mode;
        // http version parsed
        uint8_t version_major : 2;
        uint8_t version_minor : 4;

        bool done = false;
        ResponseParser(ExtraHeader *extra_hdr = nullptr);
    };

} // namespace parser

} // namespace http
