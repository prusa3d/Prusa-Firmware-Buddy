#pragma once

#include <transfers/monitor.hpp>
#include <http/types.h>

#include <array>
#include <optional>
#include <atomic>
#include <cstdio>
#include <variant>
#include <lwip/altcp.h>

namespace transfers {

class PartialFile;

} // namespace transfers

namespace nhttp {

class Server;

namespace splice {

    enum class Result {
        Stopped,
        CantWrite,
        ClosedByClient,
        Timeout,
        Ok
    };
    class Transfer {
    protected:
        std::optional<transfers::Monitor::Slot> monitor_slot;

    public:
        Server *server;
        // Can be reset in Write to false if writing fails.
        //
        // Successive writes shall be skipped, the connection aborted.
        Result result = Result::Ok;
        // Called from whatever thread!
        virtual transfers::PartialFile *file() const = 0;

        // Transfer some data from the in buffer to the out buffer.
        //
        // This'll be called with non-empty buffers (eg. in_size > 0 && out_size > 0).
        //
        // It may be a simple memcpy, but it also can do an arbitrary
        // transformation. It returns how much of in-buffer and out-buffer was
        // actually used. At least one of them must be non-zero and these
        // numbers can be different.
        virtual std::tuple<size_t, size_t> write(const uint8_t *in, size_t in_size, uint8_t *out, size_t out_size) = 0;

        // Is supposed to close the file.
        // Called in the tcpip thread.
        //
        // Will make sure that all previous writes were processed.
        virtual std::optional<std::tuple<http::Status, const char *>> done() = 0;
        // Report progress - more data was written (incremental, not total).
        //
        // Can return false to abort the transfer.
        virtual bool progress(size_t len) = 0;
        virtual ~Transfer() = default;

        // Attempts to write more data into the contained file from the pbuf.
        //
        // Returns how much was consumed from the pbuf and how much was written.
        //
        // May be both 0 in case of it would block (this one is non-blocking),
        // there's an error or if the transfer was requested to stop. Also
        // provides current snapshot of Result.
        std::tuple<size_t, Result> write(const uint8_t *in, size_t in_size);

        void release();
        const char *filepath();
        void set_monitor_slot(transfers::Monitor::Slot &&slot);
    };

} // namespace splice
} // namespace nhttp
