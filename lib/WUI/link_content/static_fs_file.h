#pragma once

#include "../nhttp/handler.h"

namespace nhttp::link_content {

// TODO: Eventually remove in favor of files in XFlash.
class StaticFsFile final : public handler::Selector {
public:
    virtual std::optional<handler::ConnectionState> accept(const handler::RequestParser &parser) const override;
};

extern const StaticFsFile static_fs_file;

}
