#pragma once

#include "monitor.hpp"

#include <common/http/httpc.hpp>
#include <common/http/socket_connection_factory.hpp>
#include <common/unique_file_ptr.hpp>

#include <variant>
#include <memory>

namespace transfers {

// Some error states
struct NoTransferSlot {};
struct RefusedRequest {};
struct AlreadyExists {};
struct Storage {
    const char *msg;
};

// TODO:
//
// Any idea how to make this thing smaller? :-(
//
// We could "gut" the response and keep only the part that is necessary to read
// the body data. Would need some refactoring, but we could do that _after_
// exhausting the temporary buffer there, so we wouldn't have to keep that one
// around.
//
// If we had a FILE *->filename function, we could name the temp file as
// real_file_name.tmp and then change the suffix only, not keeping the filename
// around.
class Download {
private:
    using ConnFactory = std::unique_ptr<http::SocketConnectionFactory>;
    // The connection factory. That holds the connection.
    //
    // We need to hide it behind a pointer, because the Response holds a
    // pointer to it and we need to move the Download around.
    ConnFactory conn_factory;
    http::ResponseBody response;
    // Note: Abused to also hold the path where the file goes eventually.
    Monitor::Slot slot;
    unique_file_ptr dest_file;
    size_t transfer_idx;
    Download(ConnFactory &&factory, http::ResponseBody &&response, Monitor::Slot &&slot, unique_file_ptr &&dest_file, size_t transfer_idx);

public:
    using DownloadResult = std::variant<Download, NoTransferSlot, AlreadyExists, RefusedRequest, Storage>;
    static DownloadResult start_connect_download(const char *host, uint16_t port, const char *url_path, const char *destination);
    // TODO: A way to read the body + some timeouts (timeout for now / total timeout / error)
};

}
