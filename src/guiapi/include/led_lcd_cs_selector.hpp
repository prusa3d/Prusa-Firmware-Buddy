#pragma once

#include <cstdint>

/**
 * @brief Writer to push data to Side LCD strip
 */
class SideStripWriter {
public:
    static void write(uint8_t *pb, uint16_t size);
};

/**
 * @brief Writer to push data to GUi LCD strip
 */
class GuiLedsWriter {
public:
    static void write(uint8_t *pb, uint16_t size);
};
