#include "download.hpp"
#include "files.hpp"

#include <freertos/binary_semaphore.hpp>
#include <common/http/resp_parser.h>
#include <common/tcpip_callback_nofail.hpp>
#include <common/pbuf_deleter.hpp>
#include <common/random.h>
#ifndef UNITTESTS
    // Avoid deep transitive dependency hell in unit tests...
    #include <nhttp/server.h>
#endif
#include <nhttp/splice.h>
#include <http_lifetime.h>
#include <timing.h>

#include <atomic>
#include <cinttypes>
#include <lwip/tcpip.h>
#include <lwip/altcp.h>
#include <lwip/altcp_tcp.h>
#include <lwip/dns.h>

using automata::ExecutionControl;
using http::ContentEncryptionMode;
using http::Error;
using http::Status;
using http::parser::ResponseParser;
using std::atomic;
using std::get;
using std::get_if;
using std::holds_alternative;
using std::make_tuple;
using std::make_unique;
using std::monostate;
using std::nullopt;
using std::optional;
using std::shared_ptr;
using std::string_view;
using std::tuple;
using std::unique_ptr;
using std::variant;

extern "C" {

// Inject for tests, which are compiled on systems without it in the header.
size_t strlcpy(char *, const char *, size_t);
size_t strlcat(char *, const char *, size_t);
}

namespace {

constexpr size_t MAX_REQ_SIZE = 512;
// We split the request into segments, request each one separately. This
// prevents the TCP buffers on sender side (or any proxy / middleware thing in
// the way that is not possible to control directly) from „overfilling“. That
// would queue up a lot of data and we couldn't process other commands fast
// enough.
//
// By splitting this into smaller requests (256 kB currently), it should
// "drain" each time and allow the commands to come. We should be able to
// process this in order of seconds, but won't introduce too much overhead
// (hopefully).
constexpr uint32_t INLINE_SEGMENT_SIZE = 512 * 512;

} // namespace

namespace transfers {

class Download::Async {
public:
    static constexpr uint32_t REQUEST_TIMEOUT = 10000; // 10 seconds for the whole request (headers, not body)
    enum class Phase {
        NotStarted,
        Dns,
        Connecting,
        Headers,
        Body,
        AbortRequested,
        Done,
    };

    /* === State variables (yes, we are a "state machine") === */
    Phase phase = Phase::NotStarted;
    bool delete_requested = false;
    freertos::BinarySemaphore delete_allowed;
    atomic<DownloadStep> last_status = DownloadStep::Continue;
    uint32_t request_started = 0; // Time when we started, to allow timing out

    struct Request {
        // TODO: The buffers are ugly. At least the path could be created in-place from iv.
        // Also, 36 = CONNECT_URL_BUF_LEN, bring it in
        char hostname[36];
        uint16_t port;
        char path[41];
        uint32_t start_range;
        optional<uint32_t> end_range;
        ip_addr_t ip;
    };

    struct Splice final : public nhttp::splice::Transfer {
        Async *owner;
        size_t transfer_rest = 0;

        Splice(Async *owner, size_t transfer_rest)
            : owner(owner)
            , transfer_rest(transfer_rest) {}

        virtual PartialFile *file() const override {
            return owner->destination.get();
        }

        virtual tuple<size_t, size_t> write(const uint8_t *in, size_t in_size, uint8_t *out, size_t out_size) override {
            return owner->decryptor->decrypt(in, in_size, out, out_size);
        }

        virtual optional<tuple<Status, const char *>> done() override {
            bool success = (transfer_rest == 0);
            if (success) {
                success = owner->destination->sync();
            } // else - we potentially have an incomplete segment and we don't want to store that one.
            owner->done(success ? DownloadStep::Finished : DownloadStep::FailedNetwork);
            // Not generating response, we are the client here.
            return nullopt;
        }

        virtual bool progress(size_t len) override {
            transfer_rest -= len;
            return owner->phase == Phase::Body;
        }
    };

    variant<monostate, Request, ResponseParser, Splice> phase_payload;
    altcp_pcb *conn = nullptr;
    PartialFile::Ptr destination;
    // TODO: Make it in-place inside phase_payload instead of passing in?
    std::unique_ptr<Decryptor> decryptor;

    Async(const char *hostname, uint16_t port, const char *path, PartialFile::Ptr destination, std::unique_ptr<Decryptor> decryptor, uint32_t start_range, optional<uint32_t> end_range)
        : phase_payload(Request { {}, port, {}, start_range, end_range, {} })
        , destination(move(destination))
        , decryptor(move(decryptor)) {
        auto &request = get<Request>(phase_payload);
        strlcpy(request.hostname, hostname, sizeof request.hostname);
        strlcpy(request.path, path, sizeof request.path);
    }
    Async(const Async &other) = delete;
    Async(Async &&oter) = delete;
    Async &operator=(const Async &other) = delete;
    Async &operator=(Async &&other) = delete;
    ~Async() = default;

    void done(DownloadStep how) {
        if (phase != Phase::Done) {
            if (conn != nullptr) {
                // This happens during error states. When we successfuly
                // transfer everything, the conn is already passed into the
                // server to deal with, so we have nothing to close here.
                altcp_abort(conn);
            }
            last_status = how;
            phase = Phase::Done;
        }
        if (delete_requested) {
            delete_allowed.release();
        }
    }

    bool timed_out() {
        uint32_t now = ticks_ms();
        uint32_t elapsed = now - request_started;
        if (elapsed > REQUEST_TIMEOUT) {
            done(DownloadStep::FailedNetwork);
            return true;
        } else {
            return false;
        }
    }

    err_t connected() {
        if (phase != Phase::Connecting) {
            done(DownloadStep::Aborted);
            return ERR_ABRT;
        }
        if (timed_out()) {
            return ERR_ABRT;
        }
        assert(holds_alternative<Request>(phase_payload));
        auto &request = get<Request>(phase_payload);
        assert(conn != nullptr);

        char range[6 /* bytes = */ + 2 * 10 /* 2^32 in text */ + 1 /* - */ + 1 /* \0 */];
        if (request.end_range.has_value()) {
            snprintf(range, sizeof range, "bytes=%" PRIu32 "-%" PRIu32, request.start_range, *request.end_range);
        } else {
            snprintf(range, sizeof range, "bytes=%" PRIu32 "-", request.start_range);
        }
        char req[MAX_REQ_SIZE + 1];
        size_t len = snprintf(req, sizeof req, "GET %s HTTP/1.1\r\nHost: %s\r\nContent-Encryption-Mode: AES-CTR\r\nRange: %s\r\n\r\n", request.path, request.hostname, range);
        if (len >= sizeof req) {
            done(DownloadStep::FailedRemote);
            return ERR_ABRT;
        }

        phase = Phase::Headers;
        phase_payload = ResponseParser();

        err_t res = altcp_write(conn, req, len, TCP_WRITE_FLAG_COPY);
        if (res != ERR_OK) {
            done(DownloadStep::FailedNetwork);
            return ERR_ABRT;
        }

        altcp_output(conn);
        return ERR_OK;
    }

    static err_t connected_wrap(void *arg, altcp_pcb *, err_t) {
        return static_cast<Async *>(arg)->connected();
    }

    err_t received_resp(unique_ptr<pbuf, PbufDeleter> data, size_t position) {
        assert(phase == Phase::Headers);
        assert(holds_alternative<ResponseParser>(phase_payload));
        auto &resp = get<ResponseParser>(phase_payload);

        auto status = static_cast<Status>(resp.status_code);

        if (status != Status::Ok && status != Status::PartialContent) {
            done(DownloadStep::FailedNetwork);
            return ERR_ABRT;
        }

        if (destination->tell() != 0 && status != Status::PartialContent) {
            done(DownloadStep::FailedRemote);
            return ERR_ABRT;
        }

        if (!resp.content_length.has_value()) {
            done(DownloadStep::FailedRemote);
            return ERR_ABRT;
        }

        if (resp.content_encryption_mode != ContentEncryptionMode::AES_CTR) {
            // Other modes are no longer supported
            done(DownloadStep::FailedRemote);
            return ERR_ABRT;
        }

        // Note: Both lengths are before decryption.
        size_t len = resp.content_length.value();
        phase_payload.emplace<Splice>(this, len);
        phase = Phase::Body;
#ifdef UNITTESTS
        assert(0); // Unimplemented here, see the note about dependency hell
#else
        tcp_pcb *c = conn;
        conn = nullptr;
        httpd_instance()->inject_transfer(c, data.release(), position, &get<Splice>(phase_payload), len);
#endif

        return ERR_OK;
    }

    err_t recv(pbuf *data_raw) {
        if (data_raw == nullptr) {
            // Close by the other end (unexpected one, we still don't have all
            // the headers).
            done(DownloadStep::FailedNetwork);
            return ERR_ABRT;
        }
        unique_ptr<pbuf, PbufDeleter> data(data_raw);
        if (phase != Phase::Headers) {
            done(DownloadStep::Aborted);
            return ERR_ABRT;
        }
        if (timed_out()) {
            return ERR_ABRT;
        }

        assert(holds_alternative<ResponseParser>(phase_payload));
        auto &parser = get<ResponseParser>(phase_payload);

        size_t position = 0;
        pbuf *current = data.get();

        while (current != nullptr) {
            string_view view(static_cast<const char *>(current->payload), current->len);
            auto [result, consumed] = parser.consume(view);
            position += consumed;
            altcp_recved(conn, consumed);

            if (!parser.done && result == ExecutionControl::NoTransition) {
                done(DownloadStep::FailedNetwork);
                return ERR_ABRT;
            }

            if (parser.done) {
                // Will free the passed pbuf (now or later when processed)
                return received_resp(move(data), position);
            }

            current = current->next;
        }

        return ERR_OK;
    }

    static err_t recv_wrap(void *arg, altcp_pcb *, pbuf *data, err_t) {
        return static_cast<Async *>(arg)->recv(data);
    }

    static err_t timeout_check_wrap(void *arg, altcp_pcb *) {
        if (static_cast<Async *>(arg)->timed_out()) {
            return ERR_ABRT;
        } else {
            return ERR_OK;
        }
    }

    void err() {
        // The connection is already closed for us, so remove it from here so done doesn't get rid of it.
        conn = nullptr;
        done(DownloadStep::FailedNetwork);
    }

    static void err_wrap(void *arg, err_t) {
        static_cast<Async *>(arg)->err();
    }

    void create_connection() {
        phase = Phase::Connecting;
        assert(holds_alternative<Request>(phase_payload));
        auto &request = get<Request>(phase_payload);
#ifndef UNITTESTS
        // Hack to make it compile during tests. Not actually executed.
        conn = altcp_new_ip_type(altcp_tcp_alloc, IP_GET_TYPE(request.ip));
#endif

        if (conn == nullptr) {
            done(DownloadStep::FailedNetwork);
            return;
        }

        altcp_arg(conn, this);
        altcp_err(conn, err_wrap);
        altcp_poll(conn, timeout_check_wrap, 1);
        altcp_recv(conn, recv_wrap);
        if (altcp_connect(conn, &request.ip, request.port, connected_wrap) != ERR_OK) {
            done(DownloadStep::FailedNetwork);
        }
    }

    void dns_found(const ip_addr_t *ip) {
        if (phase != Phase::Dns) {
            done(DownloadStep::Aborted);
            return;
        }
        if (timed_out()) {
            return;
        }
        if (ip != nullptr) {
            assert(holds_alternative<Request>(phase_payload));
            get<Request>(phase_payload).ip = *ip;
            create_connection();
        } else {
            done(DownloadStep::FailedNetwork);
        }
    }

    static void dns_found_wrap(const char *, const ip_addr_t *ip, void *arg) {
        static_cast<Async *>(arg)->dns_found(ip);
    }

    // Start it inside the tcpip thread.
    void start() {
        request_started = ticks_ms();
        assert(holds_alternative<Request>(phase_payload));
        auto &request = get<Request>(phase_payload);
        phase = Phase::Dns;
        err_t dns_result = dns_gethostbyname(request.hostname, &request.ip, dns_found_wrap, this);

        switch (dns_result) {
        case ERR_OK:
            create_connection();
            break;
        case ERR_INPROGRESS:
            // Callback will be called.
            break;
        default:
            // Should not happen, but handle it anyway.
            done(DownloadStep::FailedNetwork);
            break;
        }
    }
    static void start_wrapped(void *arg) {
        static_cast<Async *>(arg)->start();
    }
    DownloadStep status() {
        return last_status;
    }

private:
    friend class AsyncDeleter;
    void request_delete() {
        // Allow setting the semaphore (don't set it before this one gets called and pulled from the queue).
        delete_requested = true;
        switch (phase) {
        case Phase::Body:
            // TODO Is it the same?
        case Phase::Dns:
            // In these cases, we have no way to interrupt the operation. We just have to let it finish.
            phase = Phase::AbortRequested;
            break;
        case Phase::AbortRequested:
            // Already requested abort once.
            // (We can't really request abort more than once)
            assert(0);
            break;
        case Phase::Done:
            // Something already gave up previously. We are allowed to just delete and be done with it.
        case Phase::NotStarted:
        case Phase::Connecting:
        case Phase::Headers:
            // In these phases, we can just pack our things and leave right now.
            done(DownloadStep::Aborted);
            break;
        }
    }
    static void request_delete_wrap(void *param) {
        static_cast<Async *>(param)->request_delete();
    }
};

void Download::AsyncDeleter::operator()(Async *a) {
    if (a != nullptr) {
        // Unfortunately, we need the Async to cooperate and finish all its
        // work before it can be deleted, so it's not left somewhere as a
        // callback or something like that.
        tcpip_callback_nofail(Async::request_delete_wrap, a);
        a->delete_allowed.acquire();
        delete a;
    }
}

Download::Download(const Request &request, PartialFile::Ptr destination, uint32_t start_range, optional<uint32_t> end_range) {
    if (const auto *encrypted = get_if<Request::Encrypted>(&request.data); encrypted) {
        assert(encrypted->encryption);
        size_t file_size = encrypted->encryption->orig_size;
        auto decryptor = make_unique<Decryptor>(encrypted->encryption->key, encrypted->encryption->nonce, start_range, file_size - start_range);
        assert(destination);

        destination->seek(start_range);
        AsyncPtr async(new Async(encrypted->host, encrypted->port, encrypted->url_path, move(destination), move(decryptor), start_range, end_range));
        Async *async_raw = async.get();
        engine = std::move(async);
        tcpip_callback_nofail(Async::start_wrapped, async_raw);
    } else {
        const auto &in = get<Request::Inline>(request.data);
        destination->seek(start_range);
        InlinePtr in_ptr(new Inline {
            in.team_id,
            // This is just a safety feature - making sure we don't mix chunks
            // of different file in ourselves (which _shouldn't_ be possible in
            // the protocol anyway). The chance of accidentally hitting the
            // same ID being 1:2^32 is good enough.
            rand_u(),
            start_range,
            end_range.value_or(in.orig_size - 1 /* End is inclusive */),
            0,
            destination,
        });
        strlcpy(in_ptr->hash, in.hash, sizeof in_ptr->hash);
        engine = std::move(in_ptr);
        // We do _nothing_ in here, in this case, the Download is kind of "passive"
    }
}

DownloadStep Download::step() {
    if (auto *async = get_if<AsyncPtr>(&engine); async != nullptr) {
        return (*async)->status();
    } else {
        const auto &in = get<InlinePtr>(engine);
        if (in->status != DownloadStep::Continue) {
            return in->status;
        } else if (in->start > in->end /* End is inclusive */) {
            return DownloadStep::Finished;
        } else {
            return DownloadStep::Continue;
        }
    }
}

uint32_t Download::file_size() const {
    return get_partial_file()->final_size();
}

PartialFile::Ptr Download::get_partial_file() const {
    if (auto *async = get_if<AsyncPtr>(&engine); async != nullptr) {
        return (*async)->destination;
    } else {
        return get<InlinePtr>(engine)->destination;
    }
}

optional<Download::InlineRequest> Download::inline_request() {
    // We are in the inline mode and we didn't started yet (didn't ask for our first segment)
    //
    // - OR -
    //
    // We completed the previous segment and there is no other segment.
    if (auto *in_p = get_if<InlinePtr>(&engine); in_p != nullptr && (*in_p)->status == DownloadStep::Continue && (((*in_p)->start > (*in_p)->segment_end && (*in_p)->segment_end != (*in_p)->end) || !(*in_p)->started)) {
        auto &in = *in_p;
        uint32_t end = std::min(in->start + INLINE_SEGMENT_SIZE - 1 /* end is inclusive */, in->end);
        in->segment_end = end;
        InlineRequest request = {
            in->file_id,
            in->start,
            end,
        };
        // We send the extended info only on the first request in the given download.
        if (!in->started) {
            request.details = InlineRequestDetails {
                in->team_id,
                in->hash,
            };
            in->started = true;
        }
        return request;
    } else {
        return nullopt;
    }
}

bool Download::inline_chunk(const InlineChunk &chunk) {
    if (auto *in = get_if<InlinePtr>(&engine); in != nullptr) {
        if ((*in)->status != DownloadStep::Continue
            || (*in)->file_id != chunk.file_id /* Different transfer */
            || chunk.size == 0 /* Error indicated by server */
            || (*in)->start + chunk.size > (*in)->end + 1 /* end is inclusive */) {
            (*in)->status = DownloadStep::FailedRemote;
            return false;
        }
        if (!(*in)->destination->write(chunk.data, chunk.size)) {
            (*in)->status = DownloadStep::FailedStorage;
            return false;
        }
        (*in)->start += chunk.size;
        if ((*in)->start > (*in)->end) {
            (*in)->destination->sync();
        }
        return true;
    } else {
        return false;
    }
}

} // namespace transfers
