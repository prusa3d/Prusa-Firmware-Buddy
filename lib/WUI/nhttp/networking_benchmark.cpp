#include "networking_benchmark.h"

#include "cpu_utils.hpp"
#include "handler.h"
#include "headers.h"

namespace nhttp::handler {

namespace {

    Step step_error(http::Status status) {
        return { 0, 0, StatusPage { status, StatusPage::CloseHandling::ErrorClose, true } };
    }

    Step step_continue(size_t read, size_t written) {
        return { read, written, Continue {} };
    }

} // namespace

void NetworkingBenchmark::update_stats() {
    const int sample = osGetCPUUsage();
    cpu_usage_max = std::max(sample, cpu_usage_max);
    cpu_usage_min = std::min(sample, cpu_usage_min);
    cpu_usage_samples += 1;
    cpu_usage_total += sample;
}

size_t NetworkingBenchmark::print_stats(uint8_t *output, size_t output_size) const {
    const uint32_t duration_ms = ticks_ms() - ticks_ms_started;
    return snprintf(
        (char *)output, output_size, R"({"duration_ms":%u,"cpu_usage":{"max":%u,"min":%u,"avg":%u,"total":%u,"samples":%u}})",
        (unsigned)duration_ms, cpu_usage_max, cpu_usage_min, cpu_usage_total / cpu_usage_samples, cpu_usage_total, cpu_usage_samples);
}

bool NetworkingBenchmark::want_read() const {
    switch (mode) {
    case Mode::Get:
        return false;
    case Mode::Put:
        return size_rest > 0;
    }
    return false;
}

bool NetworkingBenchmark::want_write() const {
    switch (mode) {
    case Mode::Get:
        return true;
    case Mode::Put:
        return size_rest == 0;
    }
    return false;
}

Step NetworkingBenchmark::step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size) {
    update_stats();

    switch (mode) {
    case Mode::Get:
        if (!output) {
            return step_continue(0, 0);
        }
        if (!headers_sent) {
            const size_t written = write_headers(output, output_size, http::Status::Ok, http::ContentType::ApplicationOctetStream, http::ConnectionHandling::Close);
            headers_sent = true;
            return step_continue(0, written);
        }
        if (size_rest > 0) {
            const size_t written = std::min(output_size, size_rest);
            std::fill(output, output + written, ' ');
            size_rest -= written;
            return step_continue(0, written);
        } else {
            const size_t written = print_stats(output, output_size);
            return Step { 0, written, Terminating { false, Done::Close } };
        }
    case Mode::Put:
        if (terminated_by_client && size_rest > 0) {
            return step_error(http::Status::BadRequest);
        }
        if (size_rest > 0) {
            const size_t read = std::min(input.size(), size_rest);
            volatile char dummy_write;
            for (char c : input.substr(0, read)) {
                dummy_write = c;
            }
            (void)dummy_write;
            size_rest -= read;
            return step_continue(read, 0);
        }
        if (!headers_sent) {
            const size_t written = write_headers(output, output_size, http::Status::Ok, http::ContentType::ApplicationOctetStream, http::ConnectionHandling::Close);
            headers_sent = true;
            return step_continue(0, written);
        } else {
            const size_t written = print_stats(output, output_size);
            return Step { 0, written, Terminating { false, Done::Close } };
        }
    }
    return step_error(http::Status::InternalServerError);
}

} // namespace nhttp::handler
