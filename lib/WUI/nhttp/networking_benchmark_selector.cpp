#include "networking_benchmark_selector.h"

#include "networking_benchmark.h"

namespace nhttp::handler {

const NetworkingBenchmarkSelector networking_benchmark_selector;

std::optional<handler::ConnectionState> NetworkingBenchmarkSelector::accept(const handler::RequestParser &parser) const {
    if (parser.uri() == "/networking-benchmark") {
        switch (parser.method) {
        case http::Method::Put:
            if (parser.content_length.has_value()) {
                return NetworkingBenchmark(NetworkingBenchmark::Mode::Put, *parser.content_length);
            }
            return StatusPage(http::Status::LengthRequired, parser);
        case http::Method::Get:
            return NetworkingBenchmark(NetworkingBenchmark::Mode::Get, 1024 * 1024);
        default:
            return StatusPage(http::Status::MethodNotAllowed, parser);
        }
    }
    return std::nullopt;
}

} // namespace nhttp::handler
