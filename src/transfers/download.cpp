#include "download.hpp"
#include "files.hpp"

#include <common/http/get_request.hpp>

#include <sys/stat.h>

using http::GetRequest;
using http::HttpClient;
using http::Response;
using http::ResponseBody;
using http::Status;
using std::get;
using std::get_if;
using std::make_unique;
using std::move;

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
size_t strlcat(char *, const char *, size_t);
}

namespace transfers {

Download::Download(ConnFactory &&factory, ResponseBody &&response, Monitor::Slot &&slot, unique_file_ptr &&dest_file, size_t transfer_idx)
    : conn_factory(move(factory))
    , response(move(response))
    , slot(move(slot))
    , dest_file(move(dest_file))
    , transfer_idx(transfer_idx) {}

Download::DownloadResult Download::start_connect_download(const char *host, uint16_t port, const char *url_path, const char *destination) {
    // Early check for free transfer slot. This is not perfect, there's a race
    // and we can _lose_ the slot before we start the download. But we can
    // allocate it only once we know the size and for that we need to do the
    // HTTP request. And we don't want to do that if we already know we
    // wouldn't have the slot anyway.
    if (Monitor::instance.id().has_value()) {
        return NoTransferSlot {};
    }

    // Unlike the "Link" way from arbitrary client, we assume the file name
    // doesn't contain any invalid characters and such, that the server checks
    // it for us. That's non-critical assumption - in case the server doesn't
    // do the check properly, we would just fail _after_ transferring the data,
    // we just don't bother doing it at the start.
    //
    // We still want to check early if the file already exists (improper sync
    // of the file tree is quite possible).
    struct stat st = {};
    if (stat(destination, &st) == 0) {
        return AlreadyExists {};
    }

    // TODO: The timeout is a dummy for now, will be tuned once we start
    // reading the body.
    ConnFactory factory(make_unique<http::SocketConnectionFactory>(host, port, 42));
    HttpClient client(*factory.get());
    GetRequest request {
        url_path,
    };

    auto resp_any = client.send(request);

    if (auto *resp = get_if<Response>(&resp_any); resp != nullptr) {
        if (resp->status != Status::Ok) {
            return RefusedRequest {};
        }

        auto slot = Monitor::instance.allocate(Monitor::Type::Connect, destination, resp->content_length());
        if (!slot.has_value()) {
            return NoTransferSlot {};
        }

        const size_t transfer_idx = next_transfer_idx();
        const auto fname = transfer_name(transfer_idx);
        auto preallocated = file_preallocate(fname.begin(), resp->content_length());
        if (auto *err = get_if<const char *>(&preallocated); err != nullptr) {
            return Storage { *err };
        }

        auto file = move(get<unique_file_ptr>(preallocated));
        auto [initial_chunk, initial_chunk_size, body] = resp->into_body();

        if (initial_chunk_size > 0) {
            // We make our life easier by saying that this is one "item", so it
            // can't be split up.
            auto written = fwrite(initial_chunk, initial_chunk_size, 1, file.get());
            if (written != 1) {
                return Storage { "Can't write to the file" };
            }
        }

        return Download(move(factory), move(body), move(*slot), move(file), transfer_idx);
    } else {
        return RefusedRequest {};
    }
}

}
