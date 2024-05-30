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
 * @retval true  - all operations were completed successfully
 * @retval false - any of file's opening/writing/closing returned unexpected error
 */
bool TakeAScreenshot();

/**
 * @brief Takes screenshot and saves it to specified location
 *
 * @param file_name - location + name + suffix == "/usb/screenshot.bmp"
 * @return true     - all operations were completed successfully
 * @return false    - any of file's opening/writing/closing returned unexpected error
 */
bool TakeAScreenshotAs(const char *file_name);
