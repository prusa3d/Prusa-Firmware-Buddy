#pragma once

#include "handler.h"

namespace nhttp::handler::selectors {

class ValidateRequest final : public Selector {
    virtual std::optional<ConnectionState> accept(const RequestParser &request) const override;
};

const extern ValidateRequest validate_request;

class UnknownRequest final : public Selector {
    virtual std::optional<ConnectionState> accept(const RequestParser &request) const override;
};

const extern UnknownRequest unknown_request;

}
