#include "download.hpp"
#include "files.hpp"

#include <common/bsod.h>
#include <common/http/get_request.hpp>
#include <common/filename_type.hpp>
#include <gui/gui_media_events.hpp>
#include <log.h>
#include <timing.h>

#include <sys/stat.h>
#include <unistd.h>
#include <cinttypes>

LOG_COMPONENT_REF(transfers);

using http::ContentEncryptionMode;
using http::Error;
using http::GetRequest;
using http::HeaderOut;
using http::HttpClient;
using http::ResponseBody;
using http::SocketConnectionFactory;
using http::Status;
using std::get;
using std::get_if;
using std::make_unique;
using std::nullopt;
using std::optional;
using std::shared_ptr;
using std::tuple;
using std::unique_ptr;

namespace {
// TODO: Tune?
const constexpr size_t BUF_SIZE = 256;
// Time out after 5 seconds of downloading
// (downloading of body doesn't block the main activity, so we can be generous)
const constexpr uint32_t DOWNLOAD_INACTIVITY_LIMIT = 5000;
} // namespace

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
size_t strlcat(char *, const char *, size_t);
}

namespace {

optional<tuple<unique_ptr<SocketConnectionFactory>, http::Response>> send_request(const char *host, uint16_t port, const char *url_path, const HeaderOut *extra_hdrs) {
    // 3 seconds timeout.
    //
    // Will be used only for the blocking operations, timeouts on the body are
    // handled separately in our code with combination of poll / tracking of
    // time.
    unique_ptr<SocketConnectionFactory> factory(make_unique<http::SocketConnectionFactory>(host, port, 3000));
    HttpClient client(*factory.get());
    GetRequest request(url_path, extra_hdrs);

    auto resp_any = client.send(request);

    if (auto *resp = get_if<http::Response>(&resp_any); resp != nullptr) {
        return make_tuple(std::move(factory), std::move(*resp));
    } else {
        return nullopt;
    }
}

} // namespace

namespace transfers {

Download::Download(ConnFactory &&factory, ResponseBody &&response, PartialFile::Ptr partial_file, shared_ptr<EncryptionInfo> encryption, std::unique_ptr<Decryptor> decryptor)
    : conn_factory(std::move(factory))
    , response(std::move(response))
    , partial_file(partial_file)
    , last_activity(ticks_ms())
    , encryption_info(std::move(encryption))
    , decryptor(std::move(decryptor)) {
}

Download::BeginResult Download::begin(const Request &request, DestinationPath destination, uint32_t start_range, optional<uint32_t> end_range) {
    // prepare headers
    char range[6 /* bytes = */ + 2 * 10 /* 2^32 in text */ + 1 /* - */ + 1 /* \0 */];
    if (end_range.has_value()) {
        snprintf(range, sizeof range, "bytes=%" PRIu32 "-%" PRIu32, start_range, *end_range);
    } else {
        snprintf(range, sizeof range, "bytes=%" PRIu32 "-", start_range);
    }
    std::unique_ptr<HeaderOut[]> extra_hdrs;
    {
        bool include_range_header = start_range != 0 || end_range.has_value();
        bool include_encryption_header = request.encryption != nullptr;
        size_t builtin_cnt = (include_range_header ? 1 : 0) + (include_encryption_header ? 1 : 0);
        size_t extra_requested_cnt = request.extra_headers(0, nullptr);
        extra_hdrs = make_unique<HeaderOut[]>(builtin_cnt + extra_requested_cnt + 1); // 1 for null terminator
        size_t fill_idx = 0;

        // fill built-in headers
        if (include_range_header) {
            extra_hdrs[fill_idx++] = HeaderOut { "Range", range, nullopt };
        }
        if (include_encryption_header) {
            extra_hdrs[fill_idx++] = HeaderOut { "Content-Encryption-Mode", "AES-CTR", nullopt };
        }

        // include extra headers
        size_t extra_used_cnt = request.extra_headers(extra_requested_cnt, &extra_hdrs[fill_idx]);
        (void)extra_used_cnt;
        assert(extra_used_cnt == extra_requested_cnt);
        fill_idx += extra_used_cnt;

        // null-terminate
        extra_hdrs[fill_idx++] = HeaderOut { nullptr, nullptr, nullopt };

        assert(fill_idx == builtin_cnt + extra_requested_cnt + 1);
    }

    if (auto resp_any = send_request(request.host, request.port, request.url_path, extra_hdrs.get()); resp_any.has_value()) {
        auto [factory, resp] = std::move(*resp_any);

        if (start_range != 0 && resp.status != Status::PartialContent) {
            // Note: We *do* allow an 200-OK in case we set the end boundary (not checking end_range.has_value() here), for two reasons:
            // * It's possible to specify the whole file when start_range == 0 (by setting it to the length or bigger of the file), in which case it is IMO OK for the server to provide the full content.
            // * Extra data at the end is much easier for us to handle (by ignoring it) than extra data at the start.
            return RefusedRequest {};
        }

        if (resp.status != Status::Ok && resp.status != Status::PartialContent) {
            return RefusedRequest {};
        }

        // This is a bit wrong in case we don't have encryption and we request
        // a range. But at that point it won't be used much (we won't be
        // creating the file and we won't be setting up encryption).
        //
        // We do check the file is large enough to incorporate the data.
        uint32_t file_size = request.encryption != nullptr ? request.encryption->orig_size : resp.content_length() + start_range;

        // did we get the partial file from the caller already?
        PartialFile::Ptr partial_file = std::visit([&](auto &&arg) -> PartialFile::Ptr {
            using T = std::decay_t<decltype(arg)>;
            if constexpr (std::is_same_v<T, PartialFile::Ptr>) {
                return arg;
            } else {
                return nullptr;
            }
        },
            destination);

        // create the partial file here
        if (partial_file.get() == nullptr) {
            // We are allowed to create the file only at the very start and at
            // that point we request the whole file. Interaction with the
            // partial content would be a bit complex.
            assert(start_range == 0 && !end_range.has_value());
            auto destination_path = std::get<const char *>(destination);
            auto preallocated = PartialFile::create(destination_path, file_size);
            if (auto *err = get_if<const char *>(&preallocated); err != nullptr) {
                return Storage { *err };
            } else {
                partial_file = get<PartialFile::Ptr>(preallocated);
            }
        }

        // if we got the file from a caller, we need to check that it's the same size
        if (partial_file.get() != nullptr && partial_file->final_size() < file_size) {
            return Storage { "File size mismatch" };
        }

        // set where to write within the file
        partial_file->seek(start_range);

        // TODO: At this point, we no longer need a lot of stuff on the stack, eg:
        // * The URL (parent stack)
        // * The extra headers (parent stack)
        // * The request itself
        //
        // On the other hand, we don't need the decryptor until here. If we
        // ever discover the stack is too deep here, we shall split this
        // function in half here, call the first half from the parent, then
        // create the decryptor and then proceed on processing the body.

        auto [initial_chunk, initial_chunk_size, body] = resp.into_body();

        std::unique_ptr<Decryptor> decryptor;

        if (request.encryption) {
            switch (resp.content_encryption_mode.value_or(ContentEncryptionMode::AES_CBC)) {
            case ContentEncryptionMode::AES_CBC:
                decryptor = make_unique<Decryptor>(request.encryption->key, Decryptor::CBC(request.encryption->iv), file_size - start_range);
                break;
            case ContentEncryptionMode::AES_CTR:
                decryptor = make_unique<Decryptor>(request.encryption->key, Decryptor::CTR(request.encryption->nonce, start_range), file_size - start_range);
                break;
            default:
                assert(false && "Unreachable");
                return RefusedRequest {};
            }
        }

        assert(partial_file.get() != nullptr);
        auto download = Download(std::move(factory), std::move(body), partial_file, request.encryption, std::move(decryptor));

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

bool Download::process(uint8_t *data, size_t size) {
    if (decryptor.get() != nullptr) {
        size = decryptor->decrypt(data, size);
    }

    if (!partial_file->write(data, size)) {
        return false;
    }

    return true;
}

DownloadStep Download::step(uint32_t max_duration_ms) {
    uint8_t buffer[BUF_SIZE];
    static_assert(BUF_SIZE % Decryptor::BlockSize == 0, "Must be multiple of cipher block size to never overflow on decryption");

    const auto result = response.read_body(buffer, sizeof buffer, max_duration_ms);

    if (const size_t *amt = get_if<size_t>(&result); amt != nullptr) {
        if (*amt == 0) {
            log_info(transfers, "Download finished");
            partial_file->sync();
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

uint32_t Download::file_size() const {
    return partial_file->final_size();
}

bool Download::allows_random_access() const {
    if (decryptor == nullptr) {
        return true;
    } else {
        return std::visit([&](auto &d) -> bool { return d.allows_random_access(); }, decryptor->get_mode());
    }
}

} // namespace transfers
