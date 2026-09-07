/// @file
#pragma once

#include <feature/openprinttag/detail/requests_base.hpp>

namespace buddy::openprinttag {

class MultiRequestBase {

public:
    // Issues/reissues all requests
    void issue();

    /// Marks all requests as failed (Error::other)
    /// Alternative to calling issue()
    void fail();

    inline bool finished() const {
        return requests_span_.back()->finished();
    }

    /// @returns list of all requests in the multirequest (excluding the sync request)
    inline std::span<Request *> requests() {
        return requests_span_;
    }

protected:
    MultiRequestBase() = default;

protected:
    /// To be set by the child
    std::span<Request *> requests_span_;
};

} // namespace buddy::openprinttag
