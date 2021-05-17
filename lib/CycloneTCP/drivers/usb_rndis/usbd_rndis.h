/**
 * @file usbd_rndis.h
 * @brief USB RNDIS class
 *
 * @section License
 *
 * SPDX-License-Identifier: GPL-2.0-or-later
 *
 * Copyright (C) 2010-2021 Oryx Embedded SARL. All rights reserved.
 *
 * This file is part of CycloneTCP Open.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _USB_RNDIS_H
#define _USB_RNDIS_H

//Dependencies
#include "usbd_ioreq.h"

//USB endpoints
#define RNDIS_NOTIFICATION_EP 0x81
#define RNDIS_DATA_IN_EP      0x82
#define RNDIS_DATA_OUT_EP     0x03

//Endpoint maximum packet size
#define RNDIS_NOTIFICATION_EP_MPS 64
#define RNDIS_DATA_IN_EP_MPS_FS   64
#define RNDIS_DATA_OUT_EP_MPS_FS  64
#define RNDIS_DATA_IN_EP_MPS_HS   512
#define RNDIS_DATA_OUT_EP_MPS_HS  512

//RNDIS Class specific requests
#define RNDIS_SEND_ENCAPSULATED_COMMAND 0x00
#define RNDIS_GET_ENCAPSULATED_RESPONSE 0x01

//Global variables
extern USBD_HandleTypeDef USBD_Device;
extern USBD_ClassTypeDef usbdRndisClass;
#define USBD_RNDIS_CLASS &usbdRndisClass

//RNDIS related functions
uint8_t usbdRndisInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
uint8_t usbdRndisDeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx);
uint8_t usbdRndisSetup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req);
uint8_t usbdRndisEp0RxReady(USBD_HandleTypeDef *pdev);
uint8_t usbdRndisDataIn(USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t usbdRndisDataOut(USBD_HandleTypeDef *pdev, uint8_t epnum);
uint8_t *usbdRndisGetHighSpeedConfigDesc(uint16_t *length);
uint8_t *usbdRndisGetFullSpeedConfigDesc(uint16_t *length);
uint8_t *usbdRndisGetOtherSpeedConfigDesc(uint16_t *length);
uint8_t *usbdRndisGetDeviceQualifierDesc(uint16_t *length);

#endif
