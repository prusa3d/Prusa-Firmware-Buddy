#pragma once
#include <optional>
#include <stdint.h>
#include <atomic>
#include "../inc/MarlinConfig.h"


#if HAS_HEATED_BED

class BedPreheat {
public:
    void update();

    void wait_for_preheat();

private:
    std::atomic<bool> preheated = false;
    bool can_preheat = false;
    uint16_t last_enabled_bedlets = 0;

    std::optional<uint32_t> heating_start_time = std::nullopt;

    uint32_t required_preheat_time();

    uint32_t remaining_preheat_time();
};

extern BedPreheat bed_preheat;

#endif
