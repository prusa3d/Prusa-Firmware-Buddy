/**
 * @file usbd_desc.c
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

//Dependencies
#include "usbd_def.h"
#include "usbd_ctlreq.h"
#include "usbd_desc.h"
#include "os_port.h"
#include "cpu_endian.h"
#include "debug.h"

//Vendor identifier
#define USBD_VENDOR_ID  0x0483
//Product identifier
#define USBD_PRODUCT_ID 0x0123
//Device revision
#define USBD_DEVICE_REV 0x0100

//String descriptors
#define USBD_MANUFACTURER_STR  "STMicroelectronics"
#define USBD_PRODUCT_STR       "STM32 RNDIS Demo"
#define USBD_SERIAL_NUMBER_STR "00000000123C"
#define USBD_CONFIGURATION_STR "RNDIS Configuration"
#define USBD_INTERFACE_STR     "RNDIS Interface"

//Global variables
static uint8_t usbdStrDescriptor[USBD_MAX_STR_DESC_SIZ];


/**
 * @brief USB descriptors
 **/

USBD_DescriptorsTypeDef usbdRndisDescriptors = {
   usbdGetDeviceDescriptor,
   usbdGetLangIdStrDescriptor,
   usbdGetManufacturerStrDescriptor,
   usbdGetProductStrDescriptor,
   usbdGetSerialStrDescriptor,
   usbdGetConfigStrDescriptor,
   usbdGetInterfaceStrDescriptor,
};


/**
 * @brief USB device descriptor
 **/

const UsbDeviceDescriptor usbdDeviceDescriptor =
{
   sizeof(UsbDeviceDescriptor), //bLength
   USB_DESC_TYPE_DEVICE,        //bDescriptorType
   HTOLE16(0x0200),             //bcdUsb (2.00)
   USB_DEVICE_CLASS_CDC,        //bDeviceClass
   USB_DEVICE_SUBCLASS_CDC,     //bDeviceSubClass
   USB_DEVICE_PROTOCOL_CDC,     //bDeviceProtocol
   USB_EP0_MAX_PACKET_SIZE,     //bMaxPacketSize0
   HTOLE16(USBD_VENDOR_ID),     //idVendor
   HTOLE16(USBD_PRODUCT_ID),    //idProduct
   HTOLE16(USBD_DEVICE_REV),    //bcdDevice
   1,                           //iManufacturer
   2,                           //iProduct
   3,                           //iSerialNumber
   1                            //bNumConfigurations
};


/**
 * @brief USB configuration descriptors
 **/

const UsbConfigDescriptors usbdConfigDescriptors =
{
   //Standard configuration descriptor
   {
      sizeof(UsbConfigDescriptor),              //bLength
      USB_DESC_TYPE_CONFIGURATION,              //bDescriptorType
      HTOLE16(sizeof(usbdConfigDescriptors)),   //wTotalLength
      2,                                        //bNumInterfaces
      1,                                        //bConfigurationValue
      0,                                        //iConfiguration
      USB_SELF_POWERED | USB_NO_REMOTE_WAKEUP,  //bmAttributes
      0                                         //bMaxPower
   },
   //Communication class interface Descriptor
   {
      sizeof(UsbInterfaceDescriptor),           //bLength
      USB_DESC_TYPE_INTERFACE,                  //bDescriptorType
      0,                                        //bInterfaceNumber
      0,                                        //bAlternateSetting
      1,                                        //bNumEndpoints
#if 0
      //Linux or windows 7
      CDC_INTERFACE_CLASS_COMMUNICATION,        //bInterfaceClass
      CDC_INTERFACE_SUBCLASS_ACM,               //bInterfaceSubClass
      CDC_INTERFACE_PROTOCOL_VENDOR_SPECIFIC,   //bInterfaceProtocol
#else
      //Windows 10
      CDC_INTERFACE_CLASS_RNDIS,                //bInterfaceClass
      CDC_INTERFACE_SUBCLASS_RNDIS,             //bInterfaceSubClass
      CDC_INTERFACE_PROTOCOL_RNDIS,             //bInterfaceProtocol
#endif
      0                                         //iInterface
   },
   //CDC header functional descriptor
   {
      sizeof(CdcHeaderDescriptor),              //bFunctionLength
      CDC_CS_INTERFACE,                         //bDescriptorType
      CDC_HEADER_DESC_SUBTYPE,                  //bDescriptorSubtype
      HTOLE16(0x0110),                          //bcdCdc (1.10)
   },
   //CDC call management functional descriptor
   {
      sizeof(CdcCallManagementDescriptor),      //bFunctionLength
      CDC_CS_INTERFACE,                         //bDescriptorType
      CDC_CALL_MANAGEMENT_DESC_SUBTYPE,         //bDescriptorSubtype
      0x00,                                     //bmCapabilities
      1                                         //bDataInterface
   },
   //CDC abstract control management functional descriptor
   {
      sizeof(CdcAcmDescriptor),                 //bFunctionLength
      CDC_CS_INTERFACE,                         //bDescriptorType
      CDC_ACM_DESC_SUBTYPE,                     //bDescriptorSubtype
      0x00                                      //bmCapabilities
   },
   //CDC union functional descriptor
   {
      sizeof(CdcUnionDescriptor),               //bFunctionLength
      CDC_CS_INTERFACE,                         //bDescriptorType
      CDC_UNION_DESC_SUBTYPE,                   //bDescriptorSubtype
      0,                                        //bControlInterface (communication interface)
      1                                         //bSubordinateInterface0 (data interface)
   },
   //Notification endpoint descriptor
   {
      sizeof(UsbEndpointDescriptor),            //bLength
      USB_DESC_TYPE_ENDPOINT,                   //bDescriptorType
      USB_DIR_IN | USB_EP1,                     //bEndpointAddress
      USB_ENDPOINT_TYPE_INTERRUPT,              //bmAttributes
      HTOLE16(USB_EP1_MAX_PACKET_SIZE),         //wMaxPacketSize
      1                                         //bInterval (1ms)
   },
   //Data class interface descriptor
   {
      sizeof(UsbInterfaceDescriptor),           //bLength
      USB_DESC_TYPE_INTERFACE,                  //bDescriptorType
      1,                                        //bInterfaceNumber
      0,                                        //bAlternateSetting
      2,                                        //bNumEndpoints
      CDC_INTERFACE_CLASS_DATA,                 //bInterfaceClass
      CDC_INTERFACE_SUBCLASS_DATA,              //bInterfaceSubClass
      CDC_INTERFACE_PROTOCOL_DATA,              //bInterfaceProtocol
      0                                         //iInterface
   },
   //Data IN endpoint descriptor
   {
      sizeof(UsbEndpointDescriptor),            //bLength
      USB_DESC_TYPE_ENDPOINT,                   //bDescriptorType
      USB_DIR_IN | USB_EP2,                     //bEndpointAddress
      USB_ENDPOINT_TYPE_BULK,                   //bmAttributes
      HTOLE16(USB_EP2_MAX_PACKET_SIZE),         //wMaxPacketSize
      0                                         //bInterval
   },
   //Data OUT endpoint descriptor
   {
      sizeof(UsbEndpointDescriptor),            //bLength
      USB_DESC_TYPE_ENDPOINT,                   //bDescriptorType
      USB_DIR_OUT | USB_EP3,                    //bEndpointAddress
      USB_ENDPOINT_TYPE_BULK,                   //bmAttributes
      HTOLE16(USB_EP3_MAX_PACKET_SIZE),         //wMaxPacketSize
      0                                         //bInterval
   }
};


/**
 * @brief Retrieve device descriptor
 * @param[in] speed Current speed
 * @param[out] length Length of the device descriptor, in bytes
 * @return Pointer to the device descriptor
 **/

uint8_t *usbdGetDeviceDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
   //Length of the string descriptor
   *length = sizeof(usbdDeviceDescriptor);
   //Return a pointer to the string descriptor
   return (uint8_t *) &usbdDeviceDescriptor;
}


/**
 * @brief Retrieve the languages supported by the device
 * @param[in] speed Current speed
 * @param[out] length Length of the string descriptor, in bytes
 * @return Pointer to the string descriptor
 **/

uint8_t *usbdGetLangIdStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
   UsbStringDescriptor *desc;

   //Point to the buffer where to format the string descriptor
   desc = (UsbStringDescriptor *) usbdStrDescriptor;

   //Format string descriptor
   desc->bLength = 4;
   desc->bDescriptorType = USB_DESC_TYPE_STRING;
   desc->bString[0] = 0x0409;

   //Length of the string descriptor
   *length = desc->bLength;
   //Return a pointer to the string descriptor
   return (uint8_t *) desc;
}


/**
 * @brief Retrieve the manufacturer string descriptor
 * @param[in] speed Current speed
 * @param[out] length Length of the string descriptor, in bytes
 * @return Pointer to the string descriptor
 **/

uint8_t *usbdGetManufacturerStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
   //Convert the string to unicode
   USBD_GetString((uint8_t *) USBD_MANUFACTURER_STR, usbdStrDescriptor, length);
   //Return a pointer to the string descriptor
   return usbdStrDescriptor;
}


/**
 * @brief Retrieve the product string descriptor
 * @param[in] speed Current speed
 * @param[out] length Length of the string descriptor, in bytes
 * @return Pointer to the string descriptor
 **/

uint8_t *usbdGetProductStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
   //Convert the string to unicode
   USBD_GetString((uint8_t *) USBD_PRODUCT_STR, usbdStrDescriptor, length);
   //Return a pointer to the string descriptor
   return usbdStrDescriptor;
}


/**
 * @brief Retrieve the serial number string descriptor
 * @param[in] speed Current speed
 * @param[out] length Length of the string descriptor, in bytes
 * @return Pointer to the string descriptor
 **/

uint8_t *usbdGetSerialStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
   //Convert the string to unicode
   USBD_GetString((uint8_t *) USBD_SERIAL_NUMBER_STR, usbdStrDescriptor, length);
   //Return a pointer to the string descriptor
   return usbdStrDescriptor;
}


/**
 * @brief Retrieve the configuration string descriptor
 * @param[in] speed Current speed
 * @param[out] length Length of the string descriptor, in bytes
 * @return Pointer to the string descriptor
 **/

uint8_t *usbdGetConfigStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
   //Convert the string to unicode
   USBD_GetString((uint8_t *) USBD_CONFIGURATION_STR, usbdStrDescriptor, length);
   //Return a pointer to the string descriptor
   return usbdStrDescriptor;
}


/**
 * @brief Retrieve the interface string descriptor
 * @param[in] speed Current speed
 * @param[out] length Length of the string descriptor, in bytes
 * @return Pointer to the string descriptor
 **/

uint8_t *usbdGetInterfaceStrDescriptor(USBD_SpeedTypeDef speed, uint16_t *length)
{
   //Convert the string to unicode
   USBD_GetString((uint8_t *) USBD_INTERFACE_STR, usbdStrDescriptor, length);
   //Return a pointer to the string descriptor
   return usbdStrDescriptor;
}
