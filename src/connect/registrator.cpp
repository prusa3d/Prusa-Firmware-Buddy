#include "registrator.hpp"
#include "connect/connection_cache.hpp"
#include "connect/status.hpp"
#include "printer.hpp"
#include "json_out.hpp"
#include "sleep.hpp"

#include <printers.h>
#include <http/httpc.hpp>
#include <http/types.h>
#include <segmented_json.h>
#include <segmented_json_macros.h>

#include <cstdint>
#include <cstring>

using http::ContentType;
using http::Error;
using http::ExtraHeader;
using http::HeaderName;
using http::HeaderOut;
using http::HttpClient;
using http::Method;
using http::Request;
using http::Response;
using json::ChunkRenderer;
using json::JsonOutput;
using json::JsonRenderer;
using json::JsonResult;
using std::array;
using std::get;
using std::holds_alternative;
using std::min;
using std::nullopt;
using std::optional;

namespace connect_client {

namespace {

#if (PRINTER_TYPE == PRINTER_PRUSA_MINI)
    constexpr uint8_t type = 2;
    constexpr uint8_t version = 1;
    constexpr uint8_t subversion = 0;
#else
    #error "Unknown printer type"
#endif

    constexpr const char *const REGISTER_URL = "/p/register";
    constexpr size_t MAX_RESP_SIZE = 256;
    // Split sleeps to max these bits to support abort or similar.
    constexpr Duration SLEEP_MAX_MS = 125;
    // Ask the server this often.
    constexpr Duration POLL_INTERVAL_MS = 5 * 1000;

    struct ReqData {
        Printer::PrinterInfo printer_info;
    };

    class PostRenderer final : public JsonRenderer<ReqData> {
    protected:
        virtual JsonResult renderState(size_t resume_point, JsonOutput &output, ReqData &state) const {
            // Keep the indentation of the JSON in here!
            // clang-format off
            JSON_START;
            JSON_OBJ_START;
                JSON_FIELD_STR("sn", state.printer_info.serial_number) JSON_COMMA;
                JSON_FIELD_STR("fingerprint", state.printer_info.fingerprint) JSON_COMMA;
                JSON_FIELD_INT("type", type) JSON_COMMA;
                JSON_FIELD_INT("version", version) JSON_COMMA;
                JSON_FIELD_INT("subversion", subversion) JSON_COMMA;
                JSON_FIELD_STR("firmware", state.printer_info.firmware_version);
            JSON_OBJ_END;
            JSON_END;
            // clang-format on
        }

    public:
        PostRenderer(const Printer::PrinterInfo &info)
            : JsonRenderer(ReqData { info }) {}
    };

    class PostRequest final : public JsonPostRequest {
    private:
        PostRenderer renderer_impl;

    protected:
        virtual ChunkRenderer &renderer() override {
            return renderer_impl;
        }

    public:
        virtual const char *url() const override {
            return REGISTER_URL;
        }
        PostRequest(const Printer::PrinterInfo &info)
            : renderer_impl(info) {}
    };

    class PollRequest final : public Request {
    private:
        array<HeaderOut, 2> headers;

    public:
        PollRequest(const char *code)
            : headers({ { "code", code, nullopt },
                { nullptr, nullptr, nullopt } }) {}
        virtual const char *url() const override {
            return REGISTER_URL;
        }
        virtual Method method() const override {
            return Method::Get;
        }
        virtual const HeaderOut *extra_headers() const override {
            return headers.begin();
        }
        virtual ContentType content_type() const override {
            // Note: We send no body, so this is not really used.
            return ContentType::ApplicationOctetStream;
        }
    };

    class ExtractCode : public ExtraHeader {
    public:
        Code code = {};
        size_t code_len = 0;
        virtual void character(char c, HeaderName name) {
            if (name == HeaderName::Code && code_len < CODE_SIZE) {
                code[code_len++] = c;
            }
        }
    };

    class ExtractToken : public ExtraHeader {
    public:
        array<char, Printer::Config::CONNECT_TOKEN_BUF_LEN> token = {};
        size_t token_len = 0;
        virtual void character(char c, HeaderName name) {
            if (name == HeaderName::Token && token_len < Printer::Config::CONNECT_TOKEN_LEN /* Leave at least the last \0 there */) {
                token[token_len++] = c;
            }
        }
    };
}

optional<OnlineStatus> Registrator::communicate(CachedFactory &conn_factory) {
    const auto [config, cfg_changed] = printer.config();

    if (cfg_changed) {
        conn_factory.invalidate();
        return nullopt;
    }

    conn_factory.refresh(config);

    const Timestamp n = now();

    const Duration since_last = n - last_comm;
    if (since_last <= POLL_INTERVAL_MS) {
        Duration sleep_for = min(since_last, SLEEP_MAX_MS);
        sleep_raw(sleep_for);
        return nullopt;
    }

    auto exchange = [&](auto req, auto extract, auto callback) -> optional<OnlineStatus> {
        HttpClient http(conn_factory);
        auto result = http.send(req, extract);
        last_comm = now();
        if (holds_alternative<Error>(result)) {
            return bail();
        }

        Response resp = get<Response>(result);
        // Unfortunately, Connect sends us bodies that we don't read (because
        // they are useless), which would poison the next iteration. Throw the
        // connection out. Such a waste :-(.
        conn_factory.invalidate();

        return callback(resp);
    };

    switch (status) {
    case Status::Init: {
        ExtractCode code;
        PostRequest req(printer.printer_info());
        return exchange(req, &code, [&](Response &resp) -> optional<OnlineStatus> {
            switch (resp.status) {
            case http::Status::Ok: {
                if (code.code_len == 0) {
                    // Didn't get the code.
                    bail();
                }

                // Already contains the \0
                this->code = code.code;

                status = Status::GotCode;
                return OnlineStatus::RegistrationCode;
            }
            default:
                return bail();
            }
        });
    }
    case Status::GotCode: {
        ExtractToken token;
        PollRequest req(code.begin());
        return exchange(req, &token, [&](Response &resp) -> optional<OnlineStatus> {
            switch (resp.status) {
            case http::Status::Accepted:
            case http::Status::NoContent:
                // Not yet...
                // Accepted is what the docs say, but NoContent is what would actually make sense O:-) ... so having both.
                return nullopt;
            case http::Status::Ok: {
                if (token.token_len == 0) {
                    bail();
                }
                printer.init_connect(token.token.begin());
                status = Status::Done;
                return OnlineStatus::RegistrationDone;
            }
            default:
                return bail();
            }
        });
    }
    case Status::Done:
    case Status::Error:
    default:
        return nullopt;
    }
}

OnlineStatus Registrator::bail() {
    status = Status::Error;
    return OnlineStatus::RegistrationError;
}

}
