#include "download.hpp"
#include "files.hpp"
#include "notify_filechange.hpp"

#include <common/http/get_request.hpp>
#include <timing.h>

#include <sys/stat.h>
#include <unistd.h>

using http::Error;
using http::GetRequest;
using http::HeaderOut;
using http::HttpClient;
using http::Response;
using http::ResponseBody;
using http::Status;
using std::get;
using std::get_if;
using std::make_unique;
using std::move;
using std::nullopt;

namespace {
// TODO: Tune?
const constexpr size_t BUF_SIZE = 256;
// Time out after 5 seconds of downloading
// (downloading of body doesn't block the main activity, so we can be generous)
const constexpr uint32_t DOWNLOAD_INACTIVITY_LIMIT = 5000;
}

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
size_t strlcat(char *, const char *, size_t);
}

namespace transfers {

Download::Download(ConnFactory &&factory, ResponseBody &&response, Monitor::Slot &&slot, unique_file_ptr &&dest_file, size_t transfer_idx, NotifyFilechange *notify_done)
    : conn_factory(move(factory))
    , response(move(response))
    , slot(move(slot))
    , dest_file(move(dest_file))
    , transfer_idx(transfer_idx)
    , last_activity(ticks_ms())
    , notify_done(notify_done) {}

Download::~Download() {
    if (dest_file.get() != nullptr) {
        // We are being destroyed without properly finishing the transfer. Try
        // to clean up the temp file.
        //
        // Close it first.
        dest_file.reset();
        const auto fname = transfer_name(transfer_idx);
        // No error handling - nothing we could concievably do if it fails anyway.
        remove(fname.begin());
    }
}

Download::DownloadResult Download::start_connect_download(const char *host, uint16_t port, const char *url_path, const char *destination, const char *token, const char *fingerprint, size_t fingerprint_size, NotifyFilechange *notify_done) {
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

    // 3 seconds timeout.
    //
    // Will be used only for the blocking operations, timeouts on the body are
    // handled separately in our code with combination of poll / tracking of
    // time.
    ConnFactory factory(make_unique<http::SocketConnectionFactory>(host, port, 3000));
    HttpClient client(*factory.get());
    HeaderOut hdrs[] = {
        { "Fingerprint", fingerprint, fingerprint_size },
        { "Token", token, nullopt },
        { nullptr, nullptr, nullopt },
    };
    GetRequest request(url_path, hdrs);

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
            slot->progress(initial_chunk_size);
        }

        return Download(move(factory), move(body), move(*slot), move(file), transfer_idx, notify_done);
    } else {
        return RefusedRequest {};
    }
}

DownloadStep Download::step(uint32_t max_duration_ms) {
    uint8_t buffer[BUF_SIZE];

    const auto result = response.read_body(buffer, sizeof buffer, max_duration_ms);

    if (const size_t *amt = get_if<size_t>(&result); amt != nullptr) {
        if (*amt == 0) {
            { // Scope the status, so we release it before calling .done.
                const auto status = Monitor::instance.status();
                assert(status.has_value());
                assert(status->destination != nullptr);
                // Remove extra pre-allocated space (ignore the result explicitly to avoid warnings)
                (void)ftruncate(fileno(dest_file.get()), ftell(dest_file.get()));
                // Close the file so we can rename it.
                dest_file.reset();
                const auto fname = transfer_name(transfer_idx);
                if (rename(fname.begin(), status->destination) == -1) {
                    // Failed to rename :-(.
                    // At least try cleaning the temp file up.
                    // (no error handling here, we have no backup plan there).
                    remove(fname.begin());
                    return DownloadStep::Failed;
                } else {
                    if (notify_done != nullptr) {
                        notify_done->notify_filechange(status->destination);
                    }
                }
            }
            // This must be outside of the block with status, otherwise we would deadlock.
            slot.done(Monitor::Outcome::Finished);
            return DownloadStep::Finished;
        } else {
            auto written = fwrite(buffer, *amt, 1, dest_file.get());
            if (written != 1) {
                return DownloadStep::Failed;
            }

            slot.progress(*amt);
            last_activity = ticks_ms();
            return DownloadStep::Continue;
        }
    } else {
        auto err = get<Error>(result);
        if (err == Error::Timeout) {
            auto now = ticks_ms();
            if (now - last_activity >= DOWNLOAD_INACTIVITY_LIMIT) {
                return DownloadStep::Failed;
            } else {
                return DownloadStep::Continue;
            }
        } else {
            return DownloadStep::Failed;
        }
    }
}

}
