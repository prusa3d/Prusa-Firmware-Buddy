#pragma once

#include "nhttp/handler.h"

namespace nhttp::handler {

class NetworkingBenchmarkSelector final : public Selector {
public:
    virtual std::optional<handler::ConnectionState> accept(const handler::RequestParser &parser) const override;
};

extern const NetworkingBenchmarkSelector networking_benchmark_selector;

} // namespace nhttp::handler
