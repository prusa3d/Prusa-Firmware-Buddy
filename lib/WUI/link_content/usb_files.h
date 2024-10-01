#pragma once

#include "../nhttp/handler.h"

namespace nhttp::link_content {

class UsbFiles final : public handler::Selector {
public:
    virtual Accepted accept(const handler::RequestParser &parser, handler::Step &out) const override;
};

extern const UsbFiles usb_files;

} // namespace nhttp::link_content
