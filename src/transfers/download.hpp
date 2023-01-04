#pragma once

#include <common/shared_buffer.hpp>

#include <variant>

namespace transfers {

class Download {
};

struct NoTransferSlot {};
struct Filename {};
struct RefusedRequest {};
struct AlreadyExists {};

using DownloadResult = std::variant<Download, NoTransferSlot, Filename, AlreadyExists, RefusedRequest>;

DownloadResult start_connect_download(const char *host, uint16_t port, const char *url_path, SharedPath destination);

}
