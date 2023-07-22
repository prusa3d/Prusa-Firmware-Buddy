#pragma once

#include <transfers/async_io.hpp>
#include <transfers/monitor.hpp>
#include <types.h>

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
    private:
        uint8_t released_counter = 0;

    protected:
        std::optional<transfers::Monitor::Slot> monitor_slot;

    public:
        Server *server;
        // Can be reset in Write to false if writing fails.
        //
        // Successive writes shall be skipped, the connection aborted.
        std::atomic<Result> result = Result::Ok;
        // Called from whatever thread!
        virtual std::variant<FILE *, transfers::PartialFile *> file() const = 0;

        // Perform an in-place transformation (eg. decryption). May be a NOP.
        //
        // size_in: How much relevant data there is in the buffer.
        // size_out: How much data can be output (size of the buffer). At least size_in.
        //
        // Returns:
        // * How much data was consumed (<= size_in).
        // * How much data was produced (<= size_out).
        // * How large the buffer needs to be to process the rest (might be 0).
        //
        // If the last one is not 0, it shall be called again with the rest and
        // buffer of the right size. The data need to start at the beginning of
        // that buffer (eg. caller handles offset in the buffer).
        //
        // Called inside the IO thread for now.
        virtual std::tuple<size_t, size_t, size_t> transform(uint8_t *buffer, size_t size_in, size_t size_out) = 0;

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

        void release();
        const char *filepath();
        void set_monitor_slot(transfers::Monitor::Slot &&slot);
    };

    // Does nothing in the IO thread (but passes through there for ensuring
    // ordering after all writes).
    //
    // Calls done on the transfer in the tcpip thread.
    class Done final : public async_io::Request {
    private:
        Transfer *transfer = nullptr;

    public:
        virtual bool io_task() override;
        virtual void callback() override;
        virtual void final_callback() override;
        void init(Transfer *transfer);
        static Done instance;
    };

    class Write final : public async_io::Request {
    private:
        // This one is used both for manipulation / performing stuff in either task
        // and also for checking if it is free.
        //
        // It is ever written only in the IO thread (assigned on allocation, reset
        // in callback).
        Transfer *transfer = nullptr;
        // The data to write (the whole thing).
        pbuf *data = nullptr;
        // Skip this first number of bytes (already processed before start of the
        // file data), until the very end.
        size_t offset = 0;
        // Synchronize with the number in async_io.hpp
        static constexpr size_t REQ_CNT = 4;
        static std::array<Write, REQ_CNT> instances;

        // Note: we write this in multiple shots and Ack after each one.
        pbuf *current = nullptr;
        std::atomic<uint16_t> to_ack;

        struct WriteError {};
        struct WriteComplete {};
        struct NeedMore {
            size_t used;
            size_t buff_needed;
        };

        using ProcessResult = std::variant<WriteError, WriteComplete, NeedMore>;

        ProcessResult process(uint8_t *buffer, size_t size_in, size_t size_out);

    public:
        virtual bool io_task() override;
        virtual void callback() override;
        virtual void final_callback() override;
        void init(Transfer *transfer, pbuf *data, uint16_t offset = 0);
        static Write *find_empty();
    };

} // namespace splice
} // namespace nhttp
