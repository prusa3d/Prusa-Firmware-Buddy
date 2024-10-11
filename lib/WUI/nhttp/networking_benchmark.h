#pragma once

#include <string_view>
#include <cstdint>
#include <optional>
#include "step.h"
#include "timing.h"

namespace nhttp::handler {

class NetworkingBenchmark {
public:
    enum class Mode {
        Get,
        Put,
    };

private:
    Mode mode;
    size_t size_rest;
    bool headers_sent;
    uint32_t ticks_ms_started;
    int cpu_usage_max = 0;
    int cpu_usage_min = 100;
    int cpu_usage_total = 0;
    int cpu_usage_samples = 0;

    void update_stats();
    size_t print_stats(uint8_t *output, size_t output_size) const;

public:
    NetworkingBenchmark(Mode mode, size_t size_rest)
        : mode { mode }
        , size_rest { size_rest }
        , headers_sent { false }
        , ticks_ms_started { ticks_ms() } {}

    bool want_read() const;
    bool want_write() const;
    void step(std::string_view input, bool terminated_by_client, uint8_t *output, size_t output_size, Step &out);
};

} // namespace nhttp::handler
