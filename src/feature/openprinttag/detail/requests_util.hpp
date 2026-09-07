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

protected:
    const anfc::Device device_;
    const bool enable_ : 1;
};

} // namespace buddy::openprinttag
