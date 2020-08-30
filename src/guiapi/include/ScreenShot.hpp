/*
 * ScreenShot.hpp
 * \brief Screen shot feature, saving a bitmap of display to USB
 *
 *  Created on: July 10, 2020
 *      Author: Migi <michal.rudolf23@gmail.com>
 */

#pragma once

/**
 * Takes screenshot and saves it to USB flash disk.
 *
 * @retval true - all operations were completed successfuly
 *
 * @retval false - any of file's opening/writing/closing returned unexpected error
 */
bool TakeAScreenshot();
