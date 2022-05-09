/*
 * ScreenShot.hpp
 * \brief Screen shot feature, saving a bitmap of display to USB
 *
 *  Created on: July 10, 2020
 *      Author: Migi <michal.rudolf23@gmail.com>
 */

#pragma once
#include <cstdint>

/**
 * Takes screenshot and saves it to USB flash disk.
 *
 * @retval true - all operations were completed successfuly
 *
 * @retval false - any of file's opening/writing/closing returned unexpected error
 */
bool TakeAScreenshot();

struct Pixel {

    uint8_t red;
    uint8_t green;
    uint8_t blue;

    Pixel(const uint8_t data[3]);
    void SwapBlueAndRed();
};
