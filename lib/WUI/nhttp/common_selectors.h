/**
 * \file
 *
 * \brief Selectors for general HTTP handling.
 *
 * Likely each setup/server pipeline needs these.
 */
#pragma once

#include "handler.h"

namespace nhttp::handler::selectors {

/**
 * \brief Selector to validate the request.
 *
 * This performs basic validation of parsed data. The parser only signals
 * things like parse errors, but doesn't act on them. This selector shall be
 * put first and does just these kinds of checks ‒ turning them into the right
 * kind of error pages.
 */
class ValidateRequest final : public Selector {
    virtual Accepted accept(const RequestParser &request, Step &out) const override;
};

const extern ValidateRequest validate_request;

/**
 * \brief A fallback selector to return 404.
 *
 * This is expected to go last. It'll return a 404 Not Found page for all the
 * requests that reach it.
 */
class UnknownRequest final : public Selector {
    virtual Accepted accept(const RequestParser &request, Step &out) const override;
};

const extern UnknownRequest unknown_request;

} // namespace nhttp::handler::selectors
