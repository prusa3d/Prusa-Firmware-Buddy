#pragma once

#include "nhttp/handler.h"

namespace nhttp::handler {

class NetworkingBenchmarkSelector final : public Selector {
public:
    virtual Accepted accept(const handler::RequestParser &parser, Step &out) const override;
};

extern const NetworkingBenchmarkSelector networking_benchmark_selector;

} // namespace nhttp::handler
