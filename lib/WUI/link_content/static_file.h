#pragma once

#include "../nhttp/handler.h"

namespace nhttp::link_content {

/**
 * Serve static files.
 *
 * For now, they are served from /usb/www. Eventually we'll switch to xflash,
 * but this is putting the code into place and also being able to replace
 * prusa-link without flashing (and testing).
 */
class StaticFile final : public handler::Selector {
public:
    virtual std::optional<handler::ConnectionState> accept(const handler::RequestParser &parser) const override;
};

extern const StaticFile static_file;

}
