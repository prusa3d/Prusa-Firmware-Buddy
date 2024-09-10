#pragma once

#include "decrypt.hpp"
#include "monitor.hpp"
#include "partial_file.hpp"
#include <common/http/httpc.hpp>
#include <common/http/socket_connection_factory.hpp>
#include <common/unique_file_ptr.hpp>
#include <inplace_function.hpp>

#include <variant>
#include <memory>

namespace transfers {

// Some error states
struct AlreadyExists {};
struct Storage {
    const char *msg;
};
// Continuing the download from where it failed.
struct Continued {};
// Retrying the download from the start.
struct FromStart {};

enum class DownloadStep {
    Continue,
    Finished,
    // Network issues (timeouts, lost connections, ...)
    FailedNetwork,
    // Failed to write to USB
    FailedStorage,
    // Server not acting the way we expect.
    FailedRemote,
    // Aborted by calling the deleter
    Aborted,
};

//
// This class corresponds to a single GET request on the server and stores the result on disk.
//
// The request is represented by a single Download::Request object.
// The class does not handle retries or anything like that.
//
// TODO:
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
public:
    struct EncryptionInfo {
        /// The key used for AES decryption.
        Decryptor::Block key;

        /// We get "something" from the server, which is later used as a nonce
        /// (for AES-CTR) or IV (for AES-CBC).
        union {
            Decryptor::Block nonce;
            Decryptor::Block iv;
            Decryptor::Block nonce_or_iv;
        };

        /// The size of the original file.
        uint32_t orig_size;

        EncryptionInfo(Decryptor::Block key, Decryptor::Block nonce_or_iv, uint32_t orig_size)
            : key(key)
            , nonce_or_iv(nonce_or_iv)
            , orig_size(orig_size) {}

        EncryptionInfo(const EncryptionInfo &) = default;
    };

    /// Getter for extra headers to be sent with the request.
    /// The function should return the number of extra headers it wishes to send
    /// and fill the headers array with up to _headers_size_ headers.
    /// It is expected that the size of the headers array might not be sufficient,
    /// in such case the function should return the number of headers it wishes to
    /// and it will be called again with a sufficiently large array.
    using ExtraHeaders = stdext::inplace_function<size_t(size_t headers_size, http::HeaderOut *headers)>;

    struct Request {
        struct Encrypted {
            const char *host;
            uint16_t port;
            const char *url_path;
            std::shared_ptr<EncryptionInfo> encryption;
        };

        struct Inline {
            const char *hash;
            uint64_t team_id;
            uint32_t orig_size;
        };

        std::variant<Encrypted, Inline> data;

        Request(const char *host, uint16_t port, const char *url_path, std::unique_ptr<EncryptionInfo> &&encryption)
            : data(Encrypted {
                host,
                port,
                url_path,
                std::move(encryption) }) {}

        Request(const char *hash, uint64_t team_id, uint32_t orig_size)
            : data(Inline {
                hash,
                team_id,
                orig_size,
            }) {}

        uint32_t orig_size() const {
            if (const auto *encrypted = get_if<Encrypted>(&data); encrypted != nullptr) {
                return encrypted->encryption->orig_size;
            } else {
                return get<Inline>(data).orig_size;
            }
        }
    };

private:
    class Async;
    class AsyncDeleter {
    public:
        void operator()(Async *);
    };
    struct Inline {
        uint64_t team_id;
        uint32_t file_id;
        uint32_t start;
        // One past end
        uint32_t end;
        uint32_t segment_end = 0;
        PartialFile::Ptr destination;
        DownloadStep status = DownloadStep::Continue;
        bool started = false;
        static constexpr size_t HASH_BUFF = 29;
        char hash[HASH_BUFF] = {};
    };
    using AsyncPtr = std::unique_ptr<Async, AsyncDeleter>;
    // Using pointer here to make both versions same sized. Once we get rid of
    // the old download way, we should be able to just put it inside us
    // directly.
    using InlinePtr = std::unique_ptr<Inline>;
    using Engine = std::variant<AsyncPtr, InlinePtr>;
    Engine engine;

public:
    /// Makes an HTTP request.
    ///
    /// \param request The request to make.
    /// \param destination The destination file. If PartialFile is provided, it has to match the final file size. If a string is provided, the PartialFile will be created with the same name.
    /// \param offset The offset to start the download from.
    /// \return A Download object if the request was successful and the caller is expected to call step() in a loop to continue with the download.
    Download(const Request &request, PartialFile::Ptr destination, uint32_t start_range = 0, std::optional<uint32_t> end_range = std::nullopt);
    Download(Download &&other) = default;
    Download(const Download &other) = delete;
    Download &operator=(Download &&other) = default;
    Download &operator=(const Download &other) = delete;

    /// Continue the download.
    DownloadStep step();

    /// Returns the final size of the file being downloaded.
    uint32_t file_size() const;

    /// Returns the partial file object where the downloaded data is being stored.
    PartialFile::Ptr get_partial_file() const;

    struct InlineRequestDetails {
        uint64_t team_id;
        // The InlineRequest is just taken and rendered. It's fine to point
        // into the Download, as that can be removed only in connect's Sleep,
        // which doesn't happen during rendering of the InlineRequest. We do
        // not keep the request across reconnects.
        const char *hash;
    };

    struct InlineRequest {
        uint32_t file_id;
        uint32_t start;
        uint32_t end;
        std::optional<InlineRequestDetails> details = std::nullopt;
    };

    struct InlineChunk {
        uint32_t file_id;
        uint32_t size;
        const uint8_t *data;
    };

    // Get the request in case we are in inline mode and mark as started.
    std::optional<InlineRequest> inline_request();
    bool inline_chunk(const InlineChunk &chunk);
    // Network failed during the inline transfer - reset it in case it was already started.
    void network_failed();
};

} // namespace transfers
