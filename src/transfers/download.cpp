#include "download.hpp"
#include "files.hpp"
#include "changed_path.hpp"

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
using http::SocketConnectionFactory;
using http::Status;
using std::get;
using std::get_if;
using std::make_unique;
using std::move;
using std::nullopt;
using std::optional;
using std::tuple;
using std::unique_ptr;

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

namespace {

bool write_chunk(const uint8_t *data, size_t size, FILE *f) {
    // It seems fwrite is not capable of handling a 0-sized element :-O, causes division by 0.
    if (size == 0) {
        return true;
    }

    // We make our life easier by saying that this is one "item", so it
    // can't be split up.
    auto written = fwrite(data, size, 1, f);
    return written == 1;
}

optional<tuple<unique_ptr<SocketConnectionFactory>, Response>> send_request(const char *host, uint16_t port, const char *url_path, const HeaderOut *extra_hdrs) {
    // 3 seconds timeout.
    //
    // Will be used only for the blocking operations, timeouts on the body are
    // handled separately in our code with combination of poll / tracking of
    // time.
    unique_ptr<SocketConnectionFactory> factory(make_unique<http::SocketConnectionFactory>(host, port, 3000));
    HttpClient client(*factory.get());
    GetRequest request(url_path, extra_hdrs);

    auto resp_any = client.send(request);

    if (auto *resp = get_if<Response>(&resp_any); resp != nullptr) {
        return make_tuple(move(factory), move(*resp));
    } else {
        return nullopt;
    }
}

}

namespace transfers {

Download::Download(ConnFactory &&factory, ResponseBody &&response, Monitor::Slot &&slot, unique_file_ptr &&dest_file, size_t transfer_idx, unique_ptr<Decryptor> &&decryptor)
    : conn_factory(move(factory))
    , response(move(response))
    , slot(move(slot))
    , dest_file(move(dest_file))
    , transfer_idx(transfer_idx)
    , last_activity(ticks_ms())
    , decryptor(move(decryptor)) {}

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

Download::DownloadResult Download::start_connect_download(const char *host, uint16_t port, const char *url_path, const char *destination, const HeaderOut *extra_hdrs, unique_ptr<Decryptor> &&decryptor) {
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

    if (auto resp_any = send_request(host, port, url_path, extra_hdrs); resp_any.has_value()) {
        auto [factory, resp] = move(*resp_any);

        if (resp.status != Status::Ok) {
            return RefusedRequest {};
        }

        auto slot = Monitor::instance.allocate(Monitor::Type::Connect, destination, resp.content_length());
        if (!slot.has_value()) {
            return NoTransferSlot {};
        }

        const size_t transfer_idx = next_transfer_idx();
        const auto fname = transfer_name(transfer_idx);
        auto preallocated = file_preallocate(fname.begin(), resp.content_length());
        if (auto *err = get_if<const char *>(&preallocated); err != nullptr) {
            return Storage { *err };
        }

        // TODO: At this point, we no longer need a lot of stuff on the stack, eg:
        // * The URL (parent stack)
        // * The extra headers (parent stack)
        // * The request itself
        //
        // On the other hand, we don't need the decryptor until here. If we
        // ever discover the stack is too deep here, we shall split this
        // function in half here, call the first half from the parent, then
        // create the decryptor and then proceed on processing the body.

        auto file = move(get<unique_file_ptr>(preallocated));
        auto [initial_chunk, initial_chunk_size, body] = resp.into_body();

        auto download = Download(move(factory), move(body), move(*slot), move(file), transfer_idx, move(decryptor));

        if (initial_chunk_size > 0) {
            if (!download.process(initial_chunk, initial_chunk_size)) {
                return Storage { "Can't write to the file" };
            }
        }

        return download;
    } else {
        return RefusedRequest {};
    }
}

Download::RecoverResult Download::recover_encrypted_connect_download(const char *host, uint16_t port, const char *url_path, const http::HeaderOut *extra_hdrs, const Decryptor::Block &reset_iv, uint32_t reset_size) {
    // Close anything related to the previous attempt first, before allocating anything new.
    conn_factory.reset();

    RecoverResult result = Continued {};

    if (auto resp_any = send_request(host, port, url_path, extra_hdrs); resp_any.has_value()) {
        auto [factory, resp] = move(*resp_any);

        switch (resp.status) {
        case Status::Ok:
            rewind(dest_file.get());
            slot.reset_progress();
            decryptor->reset(reset_iv, reset_size);
            result = FromStart {};
            [[fallthrough]];
        case Status::PartialContent: {
            auto [initial_chunk, initial_chunk_size, body] = resp.into_body();

            if (initial_chunk_size > 0) {
                if (!process(initial_chunk, initial_chunk_size)) {
                    return Storage { "Can't write to the file" };
                }
            }
            response = move(body);
            conn_factory = move(factory);
            return result;
        }
        default:
            return RefusedRequest {};
        }
    } else {
        return RefusedRequest {};
    }
}

bool Download::process(uint8_t *data, size_t size) {
    size_t orig_size = size;
    if (decryptor.get() != nullptr) {
        size = decryptor->decrypt(data, size);
    }
    if (!write_chunk(data, size, dest_file.get())) {
        return false;
    }
    slot.progress(orig_size);
    return true;
}

DownloadStep Download::step(uint32_t max_duration_ms) {
    uint8_t buffer[BUF_SIZE];
    static_assert(BUF_SIZE % Decryptor::BlockSize == 0, "Must be multiple of cipher block size to never overflow on decryption");

    if (slot.is_stopped()) {
        slot.done(Monitor::Outcome::Stopped);
        return DownloadStep::Finished;
    }

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
                    return DownloadStep::FailedOther;
                } else {
                    ChangedPath::instance.changed_path(status->destination, ChangedPath::Type::File, ChangedPath::Incident::Created);
                }
            }
            // This must be outside of the block with status, otherwise we would deadlock.
            slot.done(Monitor::Outcome::Finished);
            return DownloadStep::Finished;
        } else {
            if (!process(buffer, *amt)) {
                return DownloadStep::FailedOther;
            }

            last_activity = ticks_ms();
            return DownloadStep::Continue;
        }
    } else {
        auto err = get<Error>(result);
        if (err == Error::Timeout) {
            auto now = ticks_ms();
            if (now - last_activity >= DOWNLOAD_INACTIVITY_LIMIT) {
                return DownloadStep::FailedNetwork;
            } else {
                return DownloadStep::Continue;
            }
        } else {
            return DownloadStep::FailedNetwork;
        }
    }
}

uint32_t Download::position() const {
    auto status = Monitor::instance.status();
    // We hold the slot, so we must be able to get the status.
    assert(status.has_value());
    return status->transferred;
}

}
