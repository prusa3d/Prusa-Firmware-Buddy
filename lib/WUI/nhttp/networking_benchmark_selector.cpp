#include "networking_benchmark_selector.h"

#include "networking_benchmark.h"

namespace nhttp::handler {

const NetworkingBenchmarkSelector networking_benchmark_selector;

Selector::Accepted NetworkingBenchmarkSelector::accept(const handler::RequestParser &parser, Step &out) const {
    if (parser.uri() == "/networking-benchmark") {
        switch (parser.method) {
        case http::Method::Put:
            if (parser.content_length.has_value()) {
                out.next = NetworkingBenchmark(NetworkingBenchmark::Mode::Put, *parser.content_length);
            } else {
                out.next = StatusPage(http::Status::LengthRequired, parser);
            }
            return Accepted::Accepted;
        case http::Method::Get:
            out.next = NetworkingBenchmark(NetworkingBenchmark::Mode::Get, 1024 * 1024);
            return Accepted::Accepted;
        default:
            out.next = StatusPage(http::Status::MethodNotAllowed, parser);
            return Accepted::Accepted;
        }
    }
    return Accepted::NextSelector;
}

} // namespace nhttp::handler
