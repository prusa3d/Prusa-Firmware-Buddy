/**
 * @file usbd_desc.h
 * @brief USB descriptors
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

#ifndef _USBD_DESC_H
#define _USBD_DESC_H

//Dependencies
#include "os_port.h"

//Endpoint identifiers
#define USB_EP0 0
#define USB_EP1 1
#define USB_EP2 2
#define USB_EP3 3

//Data transfer direction
#define USB_DIR_MASK 0x80
#define USB_DIR_OUT  0x00
#define USB_DIR_IN   0x80

//Endpoint maximum packet size
#define USB_EP0_MAX_PACKET_SIZE 64
#define USB_EP1_MAX_PACKET_SIZE 64
#define USB_EP2_MAX_PACKET_SIZE 64
#define USB_EP3_MAX_PACKET_SIZE 64

//bmAttributes field
#define USB_SELF_POWERED        0xC0
#define USB_BUS_POWERED         0x80
#define USB_REMOTE_WAKEUP       0xA0
#define USB_NO_REMOTE_WAKEUP    0x80

//Endpoint types
#define USB_ENDPOINT_TYPE_MASK        0x03
#define USB_ENDPOINT_TYPE_CONTROL     0x00
#define USB_ENDPOINT_TYPE_ISOCHRONOUS 0x01
#define USB_ENDPOINT_TYPE_BULK        0x02
#define USB_ENDPOINT_TYPE_INTERRUPT   0x03

//Device class
#define USB_DEVICE_CLASS_CDC    0x02
//Device subclass
#define USB_DEVICE_SUBCLASS_CDC 0x00
//Device protocol
#define USB_DEVICE_PROTOCOL_CDC 0x00

//CDC communication interface class
#define CDC_INTERFACE_CLASS_COMMUNICATION      0x02
#define CDC_INTERFACE_CLASS_RNDIS              0xEF
//CDC communication interface subclass
#define CDC_INTERFACE_SUBCLASS_ACM             0x02
#define CDC_INTERFACE_SUBCLASS_RNDIS           0x04
//CDC communication interface protocol
#define CDC_INTERFACE_PROTOCOL_RNDIS           0x01
#define CDC_INTERFACE_PROTOCOL_VENDOR_SPECIFIC 0xFF

//CDC data interface class
#define CDC_INTERFACE_CLASS_DATA    0x0A
//CDC data interface subclass
#define CDC_INTERFACE_SUBCLASS_DATA 0x00
//CDC data interface protocol
#define CDC_INTERFACE_PROTOCOL_DATA 0x00

//CDC descriptor types
#define CDC_CS_INTERFACE 0x24
#define CDC_CS_ENDPOINT  0x25

//CDC descriptor subtypes
#define CDC_HEADER_DESC_SUBTYPE          0x00
#define CDC_CALL_MANAGEMENT_DESC_SUBTYPE 0x01
#define CDC_ACM_DESC_SUBTYPE             0x02
#define CDC_UNION_DESC_SUBTYPE           0x06


/**
 * @brief Device descriptor
 **/

typedef __start_packed struct
{
   uint8_t bLength;
   uint8_t bDescriptorType;
   uint16_t bcdUsb;
   uint8_t bDeviceClass;
   uint8_t bDeviceSubClass;
   uint8_t bDeviceProtocol;
   uint8_t bMaxPacketSize0;
   uint16_t idVendor;
   uint16_t idProduct;
   uint16_t bcdDevice;
   uint8_t iManufacturer;
   uint8_t iProduct;
   uint8_t iSerialNumber;
   uint8_t bNumConfigurations;
} __end_packed UsbDeviceDescriptor;


/**
 * @brief Configuration descriptor
 **/

typedef __start_packed struct
{
   uint8_t bLength;
   uint8_t bDescriptorType;
   uint16_t wTotalLength;
   uint8_t bNumInterfaces;
   uint8_t bConfigurationValue;
   uint8_t iConfiguration;
   uint8_t bmAttributes;
   uint8_t bMaxPower;
} __end_packed UsbConfigDescriptor;


/**
 * @brief Interface descriptor
 **/

typedef __start_packed struct
{
   uint8_t bLength;
   uint8_t bDescriptorType;
   uint8_t bInterfaceNumber;
   uint8_t bAlternateSetting;
   uint8_t bNumEndpoints;
   uint8_t bInterfaceClass;
   uint8_t bInterfaceSubClass;
   uint8_t bInterfaceProtocol;
   uint8_t iInterface;
} __end_packed UsbInterfaceDescriptor;


/**
 * @brief Endpoint descriptor
 **/

typedef __start_packed struct
{
   uint8_t bLength;
   uint8_t bDescriptorType;
   uint8_t bEndpointAddress;
   uint8_t bmAttributes;
   uint16_t wMaxPacketSize;
   uint8_t bInterval;
} __end_packed UsbEndpointDescriptor;


/**
 * @brief String descriptor
 **/

typedef __start_packed struct
{
   uint8_t bLength;
   uint8_t bDescriptorType;
   uint16_t bString[];
} __end_packed UsbStringDescriptor;


/**
 * @brief CDC header functional descriptor
 **/

typedef __start_packed struct
{
   uint8_t bFunctionLength;
   uint8_t bDescriptorType;
   uint8_t bDescriptorSubtype;
   uint16_t bcdCdc;
} __end_packed CdcHeaderDescriptor;


/**
 * @brief CDC call management functional descriptor
 **/

typedef __start_packed struct
{
   uint8_t bFunctionLength;
   uint8_t bDescriptorType;
   uint8_t bDescriptorSubtype;
   uint8_t bmCapabilities;
   uint8_t bDataInterface;
} __end_packed CdcCallManagementDescriptor;


/**
 * @brief CDC abstract control management functional descriptor
 **/

typedef __start_packed struct
{
   uint8_t bFunctionLength;
   uint8_t bDescriptorType;
   uint8_t bDescriptorSubtype;
   uint8_t bmCapabilities;
} __end_packed CdcAcmDescriptor;


/**
 * @brief CDC union functional descriptor
 **/

typedef __start_packed struct
{
   uint8_t bFunctionLength;
   uint8_t bDescriptorType;
   uint8_t bDescriptorSubtype;
   uint8_t bMasterInterface;
   uint8_t bSlaveInterface0;
} __end_packed CdcUnionDescriptor;


/**
 * @brief Configuration descriptors
 **/

typedef __start_packed struct
{
   //Standard configuration descriptor
   UsbConfigDescriptor configDescriptor;
   //Communication class interface descriptor
   UsbInterfaceDescriptor communicationInterfaceDescriptor;
   //CDC header functional descriptor
   CdcHeaderDescriptor cdcHeaderDescriptor;
   //CDC call management functional descriptor
   CdcCallManagementDescriptor cdcCallManagementDescriptor;
   //CDC abstract control management functional descriptor
   CdcAcmDescriptor cdcAcmDescriptor;
   //CDC union functional descriptor
   CdcUnionDescriptor cdcUnionDescriptor;
   //Notification endpoint descriptor
   UsbEndpointDescriptor notificationEndpointDescriptor;
   //Data class interface descriptor
   UsbInterfaceDescriptor dataInterfaceDescriptor;
   //Data OUT endpoint descriptor
   UsbEndpointDescriptor dataOutEndpointDescriptor;
   //Data IN endpoint descriptor
   UsbEndpointDescriptor dataInEndpointDescriptor;
} __end_packed UsbConfigDescriptors;


//Global variables
extern USBD_DescriptorsTypeDef usbdRndisDescriptors;
extern const UsbDeviceDescriptor usbdDeviceDescriptor;
extern const UsbConfigDescriptors usbdConfigDescriptors;

//USB related functions
uint8_t *usbdGetDeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *usbdGetLangIdStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *usbdGetManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *usbdGetProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *usbdGetSerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *usbdGetConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);
uint8_t *usbdGetInterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length);

#endif
