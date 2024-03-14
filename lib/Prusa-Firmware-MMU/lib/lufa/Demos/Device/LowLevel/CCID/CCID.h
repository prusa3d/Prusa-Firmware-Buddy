/*
             LUFA Library
     Copyright (C) Dean Camera, 2021.

  dean [at] fourwalledcubicle [dot] com
           www.lufa-lib.org
*/

/*
  Copyright 2021  Dean Camera (dean [at] fourwalledcubicle [dot] com)
  Copyright 2021  Filipe Rodrigues (filipepazrodrigues [at] gmail [dot] com)

  Permission to use, copy, modify, distribute, and sell this
  software and its documentation for any purpose is hereby granted
  without fee, provided that the above copyright notice appear in
  all copies and that both that the copyright notice and this
  permission notice and warranty disclaimer appear in supporting
  documentation, and that the name of the author not be used in
  advertising or publicity pertaining to distribution of the
  software without specific, written prior permission.

  The author disclaims all warranties with regard to this
  software, including all implied warranties of merchantability
  and fitness.  In no event shall the author be liable for any
  special, indirect or consequential damages or any damages
  whatsoever resulting from loss of use, data or profits, whether
  in an action of contract, negligence or other tortious action,
  arising out of or in connection with the use or performance of
  this software.
*/

/** \file
 *
 *  Header file for CCID.c.
 */

#ifndef _CCID_H_
#define _CCID_H_

	/* Includes: */
		#include <avr/io.h>
		#include <avr/wdt.h>
		#include <avr/power.h>
		#include <avr/interrupt.h>
		#include <stdlib.h>

		#include "Descriptors.h"

		#include "Lib/Iso7816.h"

		#include <LUFA/Drivers/USB/USB.h>
		#include <LUFA/Drivers/Board/LEDs.h>
		#include <LUFA/Platform/Platform.h>

	/* Macros: */
		/** LED mask for the library LED driver, to indicate that the USB interface is not ready. */
		#define LEDMASK_USB_NOTREADY       LEDS_LED1

		/** LED mask for the library LED driver, to indicate that the USB interface is enumerating. */
		#define LEDMASK_USB_ENUMERATING   (LEDS_LED2 | LEDS_LED3)

		/** LED mask for the library LED driver, to indicate that the USB interface is ready. */
		#define LEDMASK_USB_READY         (LEDS_LED2 | LEDS_LED4)

		/** LED mask for the library LED driver, to indicate that an error has occurred in the USB interface. */
		#define LEDMASK_USB_ERROR         (LEDS_LED1 | LEDS_LED3)

		/** LED mask for the library LED driver, to indicate that the USB interface is busy. */
		#define LEDMASK_USB_BUSY           LEDS_LED2

	/* Function Prototypes: */
		void	SetupHardware(void);
		void	CCID_Task(void);

		uint8_t CCID_IccPowerOn(uint8_t Slot,
		                        uint8_t* const Attr,
		                        uint8_t* const AttrLength,
		                        uint8_t* const Error);
		uint8_t CCID_IccPowerOff(uint8_t Slot,
		                         uint8_t* const Error);
		uint8_t	CCID_GetSlotStatus(uint8_t Slot,
		                           uint8_t* const Error);
		uint8_t CCID_Abort(uint8_t Slot,
		                   uint8_t Seq,
		                   uint8_t* const Error);
		bool	CCID_CheckStatusNoError(uint8_t Status);

		void	EVENT_USB_Device_Connect(void);
		void	EVENT_USB_Device_Disconnect(void);
		void	EVENT_USB_Device_ConfigurationChanged(void);
		void	EVENT_USB_Device_ControlRequest(void);

#endif
