#pragma once

#pragma pack(push, 1)

struct Animation_model {
    uint8_t type;
    uint8_t R;
    uint8_t G;
    uint8_t B;
    uint16_t period;
    uint8_t color_count; // number of colors extra, not counting base color inside the model
    uint8_t next_index;
};

#pragma pack(pop)
