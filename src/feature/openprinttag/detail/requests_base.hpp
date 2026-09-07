/// @file
#pragma once

#include "defines.hpp"
#include <anfc/modbus.hpp>
#include <anfc/types.hpp>
#include <compact_pointer.hpp>
#include <feature/openprinttag/tool_tag.hpp>
#include <openprinttag/opt_reader.hpp>
#include <utils/byte_utils.hpp>
#include <tool_index.hpp>
#include <utils/uncopyable.hpp>
#include <utils/compact_optional.hpp>
#include <utils/badge.hpp>
#include <bsod/bsod.h>

namespace buddy::openprinttag {

using ManagerNoLockBadge = Badge<class Manager, struct NoLockTag>;

using OptionalRegion = CompactOptional<Region, Region::_cnt>;

/// Represents a type-safe request ID.
class RequestID {
    uint16_t value;

public:
    constexpr RequestID() = default;

    constexpr explicit RequestID(uint16_t value)
        : value { value } {}

    constexpr uint16_t to_underlying() const { return value; }

    constexpr bool operator==(const RequestID &) const = default;
};

/// Base class for any OpenPrintTag reader request.
/// - The system is asynchronous:
///   - The request is enqueued at the moment of creation (issue() needs to be called from the childmost constructor).
///   - Then, it gets @p finished() at some undetermined point, either successfully or with an @p error()
/// - Requests can be created from any thread, the system is thread-safe.
/// - Requests are enqueued/processed in the same order they were created in.
/// - If a request returns an error, other enqueued requests are still executed.
/// - The request needs to be issued by calling @p issue()
class Request : public Uncopyable {

public:
    using Error = ::openprinttag::OPTReader::Error;
    static constexpr auto no_associated_region = std::nullopt;

public:
    /// Issues the request.
    /// If the request is already issued, the issuement is cancelled and it gets reissued again
    void issue();

    /// Mark the request as failed (Error::other)
    /// Alternative to calling issue
    void fail();

    /// Returns either the device the request is supposed to be send to, or nullopt if the serialization failed for some reason
    using SerializeResult = std::optional<anfc::Device>;

    /// Serializes the request into the modbus request buffer
    /// On failure, can set the request state to finished, the caller should consider that
    /// This function is called while the Manager is locked under mutex - you'll need ManagerNoLockBadge to access the nolock variants
    /// @returns nullopt if the serialization failed for some reason.
    virtual SerializeResult serialize(ManagerNoLockBadge, RequestID, anfc::modbus::Request &) = 0;

    /// Called when the request completes with event data
    virtual void complete(Bytes event_data) = 0;

    /// @returns whether the request is still running or not
    bool finished() const {
        return finished_;
    }

    /// @returns whether the request has @p finished with an error
    /// The error can be obtained by @p error()
    bool has_error() const {
        debug_assert(finished());
        return error_ != Error::_cnt;
    }

    /// @returns error if @p has_error (otherwise UB)
    Error error() const {
        debug_assert(finished() && has_error());
        return error_;
    }

    /// @returns Region associated with the request, if there is any
    /// This is to help diagnose/recover from the region_corrupt error.
    inline OptionalRegion region() const {
        return region_ != Region::_cnt ? OptionalRegion { region_ } : std::nullopt;
    }

protected:
    /// This is an kinda-interface base class, cannot be constructed on its own
    Request(OptionalRegion region)
        : region_(region.value_or(Region::_cnt)) {}

    ~Request();

    void set_finished(std::expected<std::monostate, Error> result);

private:
    friend class Manager;

    /// Next request in the linked list of requests
    CompactRAMPointer<Request> next_request;

    /// See @p error
    /// _cnt represents nullopt, stored like this to reduce sizeof
    Error error_ : 4 = Error::_cnt;
    static_assert(std::to_underlying(Error::_cnt) < (1 << 4));

    /// See @p region()
    /// _cnt represents nullopt, stored like this to reduce sizeof
    Region region_ : 2 = Region::_cnt;
    static_assert(std::to_underlying(Region::_cnt) < (1 << 2));

    bool finished_ : 1 = false;
};

/// A Request specialization that is tied to a specific tag
/// Covers basically all the requests apart for those device-specific
class TagRequest : public Request {

public:
    /// @returns the ToolTag associated with this request (for tag_id lookup)
    const ToolTag &tool_tag() const { return tool_tag_; };

    /// Final override, subclasses should implement the serialize overload with TagID parameter
    SerializeResult serialize(ManagerNoLockBadge, RequestID, anfc::modbus::Request &) final;

    /// Serializes the request into the modbus request buffer
    virtual void serialize(RequestID, TagID, anfc::modbus::Request &) = 0;

protected:
    /// This is an kinda-interface base class, cannot be constructed on its own
    TagRequest(OptionalRegion region, const ToolTag &tool_tag)
        : Request { region }
        , tool_tag_(tool_tag) {}

private:
    ToolTag tool_tag_;
};

} // namespace buddy::openprinttag
