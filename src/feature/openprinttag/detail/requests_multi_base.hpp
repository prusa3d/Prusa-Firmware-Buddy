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

    inline std::span<const Request *const> requests() const {
        return requests_span_;
    }

    struct Report {
        /// Any error isolated only to a specific field happened
        bool any_field_specific_error : 1 = false;

        /// Any error that could go away if we retry the operation
        bool any_recoverable_error : 1 = false;

        /// Any serious error that cannot be fixed by retrying
        bool any_unrecoverable_error : 1 = false;

        /// any_recoverable_error | any_unrecoverable_error
        bool any_significant_error : 1 = false;

        /// Whether a corruption of aux region has been detected
        /// Also always sets any_unrecoverable_error
        bool aux_region_corrupt : 1 = false;
    };

    Report report() const;

protected:
    MultiRequestBase() = default;

protected:
    /// To be set by the child
    std::span<Request *> requests_span_;
};

} // namespace buddy::openprinttag
