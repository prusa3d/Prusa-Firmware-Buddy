#include "download.hpp"

namespace transfers {

DownloadResult start_connect_download(const char *host, uint16_t port, const char *url_path, SharedPath destination) {
    return RefusedRequest {};
}

}
