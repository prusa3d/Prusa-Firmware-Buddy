#pragma once

#include "../nhttp/handler.h"

namespace nhttp::link_content {

class UsbFiles final : public handler::Selector {
public:
    virtual std::optional<handler::ConnectionState> accept(const handler::RequestParser &parser) const override;
};

extern const UsbFiles usb_files;

} // namespace nhttp::link_content
