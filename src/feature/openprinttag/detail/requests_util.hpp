/// @file
#pragma once

#include <feature/openprinttag/detail/requests_base.hpp>
#include <utils/byte_utils.hpp>

namespace buddy::openprinttag {

class EnableRadioRequest final : public Request {

public:
    explicit EnableRadioRequest(anfc::Device device, bool enable)
        : Request { std::nullopt }
        , device_ { device }
        , enable_ { enable } {}

    SerializeResult serialize(ManagerNoLockBadge, RequestID, anfc::modbus::Request &) final;
    void complete(Bytes) final;

    bool enable() const { return enable_; }

    /// Must not be called while the request is in flight; reissue the request afterwards
    void set_enable(bool enable) { enable_ = enable; }

protected:
    const anfc::Device device_;
    bool enable_ : 1;
};

} // namespace buddy::openprinttag
