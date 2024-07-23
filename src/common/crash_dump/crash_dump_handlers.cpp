#include "crash_dump_handlers.hpp"
#include <w25x.h>
#include <cstring>
#include <device/board.h>
#include <logging/log.hpp>

LOG_COMPONENT_REF(CrashDump);

namespace crash_dump {
std::span<const DumpHandler *> get_present_dumps(BufferT &buffer) {
    size_t num_present { 0 };
    for (const auto &handler : dump_handlers) {
        if (handler.presence_check()) {
            buffer[num_present++] = &handler;
        }
    }

    return { buffer.begin(), num_present };
}

class BuddyDumpRequest : public http::Request {
public:
    enum class ReadState {
        ram,
        otp,
        flash,
        done,
    };

    BuddyDumpRequest(const char *url_string_)
        : dump_size(dump_get_size())
        , hdrs {
            { "Content-Length", dump_size, std::nullopt },
            { nullptr, nullptr, std::nullopt },
        }
        , url_string(url_string_) {}
    const char *url() const override { return url_string; }
    http::ContentType content_type() const override {
        return http::ContentType::ApplicationOctetStream;
    }
    http::Method method() const override {
        return http::Post;
    }
    const http::HeaderOut *extra_headers() const override {
        return hdrs;
    }
    std::variant<size_t, http::Error> write_body_chunk(char *data, size_t size) override {
        if (state == ReadState::done) {
            return static_cast<size_t>(0);
        }

        static constexpr auto calculate_to_read = [](size_t idx, size_t sz, size_t read, size_t max) {
            return std::min(max - idx, sz - read);
        };

        static constexpr auto update_variables = [](size_t &idx, size_t &read, ReadState &state, size_t to_read, ReadState new_state, size_t max) {
            read += to_read;
            idx += to_read;
            if (idx >= max) {
                state = new_state;
                idx = 0;
            }
        };

        size_t read { 0 }; // number of bytes that have been read
        if (state == ReadState::ram) {
            const size_t to_read { calculate_to_read(idx, size, read, dump_size) };

            memset(data, 0, to_read);
            if (!dump_read_data(idx, to_read, reinterpret_cast<uint8_t *>(data))) {
                log_error(CrashDump, "error reading from flash");
                return http::Error::InternalError;
            }

            update_variables(idx, read, state, to_read, ReadState::otp, dump_size);
        }
        return read;
    }

private:
    size_t dump_size;
    http::HeaderOut hdrs[2];
    const char *url_string;
    ReadState state { ReadState::ram };
    size_t idx { 0 };
};

void upload_buddy_dump_to_server() {
    std::array<char, url_buff_size> url_buff;
    std::array<char, url_buff_size> escaped_url_string;

    create_url_string(url_buff, escaped_url_string, BOARD_STRING());
    BuddyDumpRequest req(escaped_url_string.data());
    upload_dump_to_server(req);
}
} // namespace crash_dump
