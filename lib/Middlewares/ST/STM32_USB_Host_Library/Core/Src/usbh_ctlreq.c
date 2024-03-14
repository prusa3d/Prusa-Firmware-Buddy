/**
  ******************************************************************************
  * @file    usbh_ctlreq.c
  * @author  MCD Application Team
  * @brief   This file implements the control requests for device enumeration
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2015 STMicroelectronics.
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under Ultimate Liberty license
  * SLA0044, the "License"; You may not use this file except in compliance with
  * the License. You may obtain a copy of the License at:
  *                      www.st.com/SLA0044
  *
  ******************************************************************************
  */

/* Includes ------------------------------------------------------------------*/
#include "usbh_ctlreq.h"

/** @addtogroup USBH_LIB
  * @{
  */

/** @addtogroup USBH_LIB_CORE
  * @{
  */

/** @defgroup USBH_CTLREQ
  * @brief This file implements the standard requests for device enumeration
  * @{
  */


/** @defgroup USBH_CTLREQ_Private_Defines
  * @{
  */
/**
  * @}
  */


/** @defgroup USBH_CTLREQ_Private_TypesDefinitions
  * @{
  */
/**
  * @}
  */



/** @defgroup USBH_CTLREQ_Private_Macros
  * @{
  */
/**
  * @}
  */


/** @defgroup USBH_CTLREQ_Private_Variables
  * @{
  */
/**
  * @}
  */

/** @defgroup USBH_CTLREQ_Private_FunctionPrototypes
  * @{
  */
static USBH_StatusTypeDef USBH_HandleControl(USBH_HandleTypeDef *phost);

static void USBH_ParseDevDesc(USBH_DevDescTypeDef *dev_desc,
                              uint8_t *buf, uint16_t length);

static USBH_StatusTypeDef USBH_ParseCfgDesc(USBH_HandleTypeDef *phost,
                                            uint8_t *buf, uint16_t length);

static USBH_StatusTypeDef USBH_ParseEPDesc(USBH_HandleTypeDef *phost, USBH_EpDescTypeDef  *ep_descriptor, uint8_t *buf);
static void USBH_ParseStringDesc(uint8_t *psrc, uint8_t *pdest, uint16_t length);
static void USBH_ParseInterfaceDesc(USBH_InterfaceDescTypeDef  *if_descriptor, uint8_t *buf);


/**
  * @}
  */


/** @defgroup USBH_CTLREQ_Private_Functions
  * @{
  */


/**
  * @brief  USBH_Get_DevDesc
  *         Issue Get Device Descriptor command to the device. Once the response
  *         received, it parses the device descriptor and updates the status.
  * @param  phost: Host Handle
  * @param  length: Length of the descriptor
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_Get_DevDesc(USBH_HandleTypeDef *phost, uint8_t length)
{
  USBH_StatusTypeDef status;

  status = USBH_GetDescriptor(phost,
                              USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD,
                              USB_DESC_DEVICE, phost->device.Data,
                              (uint16_t)length);

  if (status == USBH_OK)
  {
    /* Commands successfully sent and Response Received */
    USBH_ParseDevDesc(&phost->device.DevDesc, phost->device.Data,
                      (uint16_t)length);
  }

  return status;
}


/**
  * @brief  USBH_Get_CfgDesc
  *         Issues Configuration Descriptor to the device. Once the response
  *         received, it parses the configuration descriptor and updates the
  *         status.
  * @param  phost: Host Handle
  * @param  length: Length of the descriptor
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_Get_CfgDesc(USBH_HandleTypeDef *phost,
                                    uint16_t length)

{
  USBH_StatusTypeDef status;
  uint8_t *pData = phost->device.CfgDesc_Raw;

  status = USBH_GetDescriptor(phost, (USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD),
                              USB_DESC_CONFIGURATION, pData, length);

  if (status == USBH_OK)
  {
    /* Commands successfully sent and Response Received  */
    status = USBH_ParseCfgDesc(phost, pData, length);
  }

  return status;
}


/**
  * @brief  USBH_Get_StringDesc
  *         Issues string Descriptor command to the device. Once the response
  *         received, it parses the string descriptor and updates the status.
  * @param  phost: Host Handle
  * @param  string_index: String index for the descriptor
  * @param  buff: Buffer address for the descriptor
  * @param  length: Length of the descriptor
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_Get_StringDesc(USBH_HandleTypeDef *phost,
                                       uint8_t string_index, uint8_t *buff,
                                       uint16_t length)
{
  USBH_StatusTypeDef status;

  status = USBH_GetDescriptor(phost,
                              USB_REQ_RECIPIENT_DEVICE | USB_REQ_TYPE_STANDARD,
                              USB_DESC_STRING | string_index,
                              phost->device.Data, length);

  if (status == USBH_OK)
  {
    /* Commands successfully sent and Response Received  */
    USBH_ParseStringDesc(phost->device.Data, buff, length);
  }

  return status;
}


/**
  * @brief  USBH_GetDescriptor
  *         Issues Descriptor command to the device. Once the response received,
  *         it parses the descriptor and updates the status.
  * @param  phost: Host Handle
  * @param  req_type: Descriptor type
  * @param  value_idx: Value for the GetDescriptr request
  * @param  buff: Buffer to store the descriptor
  * @param  length: Length of the descriptor
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_GetDescriptor(USBH_HandleTypeDef *phost,
                                      uint8_t  req_type,
                                      uint16_t value_idx,
                                      uint8_t *buff,
                                      uint16_t length)
{
  if (phost->RequestState == CMD_SEND)
  {
    phost->Control.setup.b.bmRequestType = USB_D2H | req_type;
    phost->Control.setup.b.bRequest = USB_REQ_GET_DESCRIPTOR;
    phost->Control.setup.b.wValue.w = value_idx;

    if ((value_idx & 0xff00U) == USB_DESC_STRING)
    {
      phost->Control.setup.b.wIndex.w = 0x0409U;
    }
    else
    {
      phost->Control.setup.b.wIndex.w = 0U;
    }
    phost->Control.setup.b.wLength.w = length;
  }

  return USBH_CtlReq(phost, buff, length);
}


/**
  * @brief  USBH_SetAddress
  *         This command sets the address to the connected device
  * @param  phost: Host Handle
  * @param  DeviceAddress: Device address to assign
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SetAddress(USBH_HandleTypeDef *phost,
                                   uint8_t DeviceAddress)
{
  if (phost->RequestState == CMD_SEND)
  {
    phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_DEVICE | \
                                           USB_REQ_TYPE_STANDARD;

    phost->Control.setup.b.bRequest = USB_REQ_SET_ADDRESS;

    phost->Control.setup.b.wValue.w = (uint16_t)DeviceAddress;
    phost->Control.setup.b.wIndex.w = 0U;
    phost->Control.setup.b.wLength.w = 0U;
  }

  return USBH_CtlReq(phost, NULL, 0U);
}


/**
  * @brief  USBH_SetCfg
  *         The command sets the configuration value to the connected device
  * @param  phost: Host Handle
  * @param  cfg_idx: Configuration value
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SetCfg(USBH_HandleTypeDef *phost, uint16_t cfg_idx)
{
  if (phost->RequestState == CMD_SEND)
  {
    phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_DEVICE
                                           | USB_REQ_TYPE_STANDARD;

    phost->Control.setup.b.bRequest = USB_REQ_SET_CONFIGURATION;
    phost->Control.setup.b.wValue.w = cfg_idx;
    phost->Control.setup.b.wIndex.w = 0U;
    phost->Control.setup.b.wLength.w = 0U;
  }

  return USBH_CtlReq(phost, NULL, 0U);
}


/**
  * @brief  USBH_SetInterface
  *         The command sets the Interface value to the connected device
  * @param  phost: Host Handle
  * @param  altSetting: Interface value
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_SetInterface(USBH_HandleTypeDef *phost, uint8_t ep_num,
                                     uint8_t altSetting)
{
  if (phost->RequestState == CMD_SEND)
  {
    phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_INTERFACE
                                           | USB_REQ_TYPE_STANDARD;

    phost->Control.setup.b.bRequest = USB_REQ_SET_INTERFACE;
    phost->Control.setup.b.wValue.w = altSetting;
    phost->Control.setup.b.wIndex.w = ep_num;
    phost->Control.setup.b.wLength.w = 0U;
  }

  return USBH_CtlReq(phost, NULL, 0U);
}


/**
  * @brief  USBH_SetFeature
  *         The command sets the device features (remote wakeup feature,..)
  * @param  pdev: Selected device
  * @param  itf_idx
  * @retval Status
  */
USBH_StatusTypeDef USBH_SetFeature(USBH_HandleTypeDef *phost, uint8_t wValue)
{
  if (phost->RequestState == CMD_SEND)
  {
    phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_DEVICE
                                           | USB_REQ_TYPE_STANDARD;

    phost->Control.setup.b.bRequest = USB_REQ_SET_FEATURE;
    phost->Control.setup.b.wValue.w = wValue;
    phost->Control.setup.b.wIndex.w = 0U;
    phost->Control.setup.b.wLength.w = 0U;
  }

  return USBH_CtlReq(phost, NULL, 0U);
}


/**
  * @brief  USBH_ClrFeature
  *         This request is used to clear or disable a specific feature.
  * @param  phost: Host Handle
  * @param  ep_num: endpoint number
  * @param  hc_num: Host channel number
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_ClrFeature(USBH_HandleTypeDef *phost, uint8_t ep_num)
{
  if (phost->RequestState == CMD_SEND)
  {
    phost->Control.setup.b.bmRequestType = USB_H2D | USB_REQ_RECIPIENT_ENDPOINT
                                           | USB_REQ_TYPE_STANDARD;

    phost->Control.setup.b.bRequest = USB_REQ_CLEAR_FEATURE;
    phost->Control.setup.b.wValue.w = FEATURE_SELECTOR_ENDPOINT;
    phost->Control.setup.b.wIndex.w = ep_num;
    phost->Control.setup.b.wLength.w = 0U;
  }
  return USBH_CtlReq(phost, NULL, 0U);
}


/**
  * @brief  USBH_ParseDevDesc
  *         This function Parses the device descriptor
  * @param  dev_desc: device_descriptor destination address
  * @param  buf: Buffer where the source descriptor is available
  * @param  length: Length of the descriptor
  * @retval None
  */
static void  USBH_ParseDevDesc(USBH_DevDescTypeDef *dev_desc, uint8_t *buf,
                               uint16_t length)
{
  dev_desc->bLength            = *(uint8_t *)(buf +  0);
  dev_desc->bDescriptorType    = *(uint8_t *)(buf +  1);
  dev_desc->bcdUSB             = LE16(buf +  2);
  dev_desc->bDeviceClass       = *(uint8_t *)(buf +  4);
  dev_desc->bDeviceSubClass    = *(uint8_t *)(buf +  5);
  dev_desc->bDeviceProtocol    = *(uint8_t *)(buf +  6);
  dev_desc->bMaxPacketSize     = *(uint8_t *)(buf +  7);

  /* Make sure that the max packet size is either 8, 16, 32, 64 or force it to 64 */
  switch (dev_desc->bMaxPacketSize)
  {
    case 8:
    case 16:
    case 32:
    case 64:
      dev_desc->bMaxPacketSize = dev_desc->bMaxPacketSize;
      break;

    default:
      /*set the size to 64 in case the device has answered with incorrect size */
      dev_desc->bMaxPacketSize = 64U;
      break;
  }

  if (length > 8U)
  {
    /* For 1st time after device connection, Host may issue only 8 bytes for
    Device Descriptor Length  */
    dev_desc->idVendor           = LE16(buf +  8);
    dev_desc->idProduct          = LE16(buf + 10);
    dev_desc->bcdDevice          = LE16(buf + 12);
    dev_desc->iManufacturer      = *(uint8_t *)(buf + 14);
    dev_desc->iProduct           = *(uint8_t *)(buf + 15);
    dev_desc->iSerialNumber      = *(uint8_t *)(buf + 16);
    dev_desc->bNumConfigurations = *(uint8_t *)(buf + 17);
  }
}


/**
  * @brief  USBH_ParseCfgDesc
  *         This function Parses the configuration descriptor
  * @param  phost: USB Host handler
  * @param  buf: Buffer where the source descriptor is available
  * @param  length: Length of the descriptor
  * @retval USBH statuse
  */
static USBH_StatusTypeDef USBH_ParseCfgDesc(USBH_HandleTypeDef *phost, uint8_t *buf, uint16_t length)
{
  USBH_CfgDescTypeDef *cfg_desc = &phost->device.CfgDesc;
  USBH_StatusTypeDef           status = USBH_OK;
  USBH_InterfaceDescTypeDef    *pif ;
  USBH_EpDescTypeDef           *pep;
  USBH_DescHeader_t            *pdesc = (USBH_DescHeader_t *)(void *)buf;
  uint16_t                     ptr;
  uint8_t                      if_ix = 0U;
  uint8_t                      ep_ix = 0U;

  pdesc   = (USBH_DescHeader_t *)(void *)buf;

  /* Parse configuration descriptor */
  cfg_desc->bLength             = *(uint8_t *)(buf + 0);
  cfg_desc->bDescriptorType     = *(uint8_t *)(buf + 1);
  cfg_desc->wTotalLength        = MIN(((uint16_t) LE16(buf + 2)), ((uint16_t)USBH_MAX_SIZE_CONFIGURATION));
  cfg_desc->bNumInterfaces      = *(uint8_t *)(buf + 4);
  cfg_desc->bConfigurationValue = *(uint8_t *)(buf + 5);
  cfg_desc->iConfiguration      = *(uint8_t *)(buf + 6);
  cfg_desc->bmAttributes        = *(uint8_t *)(buf + 7);
  cfg_desc->bMaxPower           = *(uint8_t *)(buf + 8);

  /* Make sure that the Confguration descriptor's bLength is equal to USB_CONFIGURATION_DESC_SIZE */
  if (cfg_desc->bLength  != USB_CONFIGURATION_DESC_SIZE)
  {
    cfg_desc->bLength = USB_CONFIGURATION_DESC_SIZE;
  }

  if (length > USB_CONFIGURATION_DESC_SIZE)
  {
    ptr = USB_LEN_CFG_DESC;
    pif = (USBH_InterfaceDescTypeDef *)NULL;

    while ((if_ix < USBH_MAX_NUM_INTERFACES) && (ptr < cfg_desc->wTotalLength))
    {
      pdesc = USBH_GetNextDesc((uint8_t *)(void *)pdesc, &ptr);
      if (pdesc->bDescriptorType == USB_DESC_TYPE_INTERFACE)
      {
        /* Make sure that the interface descriptor's bLength is equal to USB_INTERFACE_DESC_SIZE */
        if (pdesc->bLength != USB_INTERFACE_DESC_SIZE)
        {
          pdesc->bLength = USB_INTERFACE_DESC_SIZE;
        }

        pif = &cfg_desc->Itf_Desc[if_ix];
        USBH_ParseInterfaceDesc(pif, (uint8_t *)(void *)pdesc);

        ep_ix = 0U;
        pep = (USBH_EpDescTypeDef *)NULL;

        while ((ep_ix < pif->bNumEndpoints) && (ptr < cfg_desc->wTotalLength))
        {
          pdesc = USBH_GetNextDesc((uint8_t *)(void *)pdesc, &ptr);

          if (pdesc->bDescriptorType == USB_DESC_TYPE_ENDPOINT)
          {
            /* Check if the endpoint is appartening to an audio streaming interface */
            if ((pif->bInterfaceClass == 0x01U) && (pif->bInterfaceSubClass == 0x02U))
            {
              /* Check if it is supporting the USB AUDIO 01 class specification */
              if ((pif->bInterfaceProtocol == 0x00U) && (pdesc->bLength != 0x09U))
              {
                pdesc->bLength = 0x09U;
              }
            }
            /* Make sure that the endpoint descriptor's bLength is equal to
               USB_ENDPOINT_DESC_SIZE for all other endpoints types */
            else if (pdesc->bLength != USB_ENDPOINT_DESC_SIZE)
            {
              pdesc->bLength = USB_ENDPOINT_DESC_SIZE;
            }
            else
            {
              /* ... */
            }

            pep = &cfg_desc->Itf_Desc[if_ix].Ep_Desc[ep_ix];

            status = USBH_ParseEPDesc(phost, pep, (uint8_t *)(void *)pdesc);

            ep_ix++;
          }
        }

        /* Check if the required endpoint(s) data are parsed */
        if (ep_ix < pif->bNumEndpoints)
        {
          return USBH_NOT_SUPPORTED;
        }

        if_ix++;
      }
    }

    /* Check if the required interface(s) data are parsed */
    if (if_ix < MIN(cfg_desc->bNumInterfaces, (uint8_t)USBH_MAX_NUM_INTERFACES))
    {
      return USBH_NOT_SUPPORTED;
    }
  }

  return status;
}


/**
  * @brief  USBH_ParseInterfaceDesc
  *         This function Parses the interface descriptor
  * @param  if_descriptor : Interface descriptor destination
  * @param  buf: Buffer where the descriptor data is available
  * @retval None
  */
static void  USBH_ParseInterfaceDesc(USBH_InterfaceDescTypeDef *if_descriptor,
                                     uint8_t *buf)
{
  if_descriptor->bLength            = *(uint8_t *)(buf + 0);
  if_descriptor->bDescriptorType    = *(uint8_t *)(buf + 1);
  if_descriptor->bInterfaceNumber   = *(uint8_t *)(buf + 2);
  if_descriptor->bAlternateSetting  = *(uint8_t *)(buf + 3);
  if_descriptor->bNumEndpoints      = *(uint8_t *)(buf + 4);
  if_descriptor->bInterfaceClass    = *(uint8_t *)(buf + 5);
  if_descriptor->bInterfaceSubClass = *(uint8_t *)(buf + 6);
  if_descriptor->bInterfaceProtocol = *(uint8_t *)(buf + 7);
  if_descriptor->iInterface         = *(uint8_t *)(buf + 8);
}


/**
  * @brief  USBH_ParseEPDesc
  *         This function Parses the endpoint descriptor
  * @param  phost: USB Host handler
  * @param  ep_descriptor: Endpoint descriptor destination address
  * @param  buf: Buffer where the parsed descriptor stored
  * @retval USBH Status
  */
static USBH_StatusTypeDef  USBH_ParseEPDesc(USBH_HandleTypeDef *phost, USBH_EpDescTypeDef  *ep_descriptor,
                                            uint8_t *buf)
{
  USBH_StatusTypeDef status = USBH_OK;
  ep_descriptor->bLength          = *(uint8_t *)(buf + 0);
  ep_descriptor->bDescriptorType  = *(uint8_t *)(buf + 1);
  ep_descriptor->bEndpointAddress = *(uint8_t *)(buf + 2);
  ep_descriptor->bmAttributes     = *(uint8_t *)(buf + 3);
  ep_descriptor->wMaxPacketSize   = LE16(buf + 4);
  ep_descriptor->bInterval        = *(uint8_t *)(buf + 6);

  /* Make sure that wMaxPacketSize is different from 0 */
  if (ep_descriptor->wMaxPacketSize == 0x00U)
  {
    status = USBH_NOT_SUPPORTED;
  }
  else if (USBH_MAX_EP_PACKET_SIZE < (uint16_t)USBH_MAX_DATA_BUFFER)
  {
    /* Make sure that maximum packet size (bits 0..10) does not exceed the max endpoint packet size */
    ep_descriptor->wMaxPacketSize &= ~0x7FFU;
    ep_descriptor->wMaxPacketSize |=  MIN((uint16_t)(LE16(buf + 4) & 0x7FFU), (uint16_t)USBH_MAX_EP_PACKET_SIZE);

  }
  else if ((uint16_t)USBH_MAX_DATA_BUFFER < USBH_MAX_EP_PACKET_SIZE)
  {
    /* Make sure that maximum packet size (bits 0..10) does not exceed the total buffer length */
    ep_descriptor->wMaxPacketSize &= ~0x7FFU;
    ep_descriptor->wMaxPacketSize |= MIN((uint16_t)(LE16(buf + 4) & 0x7FFU), (uint16_t)USBH_MAX_DATA_BUFFER);
  }
  else
  {
    /* ... */
  }

  /* For high-speed interrupt/isochronous endpoints, bInterval can vary from 1 to 16 */
  if (phost->device.speed == (uint8_t)USBH_SPEED_HIGH)
  {
    if (((ep_descriptor->bmAttributes & EP_TYPE_MSK) == EP_TYPE_ISOC) ||
        ((ep_descriptor->bmAttributes & EP_TYPE_MSK) == EP_TYPE_INTR))
    {
      if ((ep_descriptor->bInterval == 0U) || (ep_descriptor->bInterval > 0x10U))
      {
        status = USBH_NOT_SUPPORTED;
      }
    }
  }
  else
  {
    /* For full-speed isochronous endpoints, the value of bInterval must be in the range from 1 to 16.*/
    if ((ep_descriptor->bmAttributes & EP_TYPE_MSK) == EP_TYPE_ISOC)
    {
      if ((ep_descriptor->bInterval == 0U) || (ep_descriptor->bInterval > 0x10U))
      {
        status = USBH_NOT_SUPPORTED;
      }
    }
    /* For full-/low-speed interrupt endpoints, the value of bInterval may be from 1 to 255.*/
    else if ((ep_descriptor->bmAttributes & EP_TYPE_MSK) == EP_TYPE_INTR)
    {
      if (ep_descriptor->bInterval == 0U)
      {
        status = USBH_NOT_SUPPORTED;
      }
    }
    else
    {
      /* ... */
    }
  }

  return status;
}


/**
  * @brief  USBH_ParseStringDesc
  *         This function Parses the string descriptor
  * @param  psrc: Source pointer containing the descriptor data
  * @param  pdest: Destination address pointer
  * @param  length: Length of the descriptor
  * @retval None
  */
static void USBH_ParseStringDesc(uint8_t *psrc, uint8_t *pdest, uint16_t length)
{
  uint16_t strlength;
  uint16_t idx;

  /* The UNICODE string descriptor is not NULL-terminated. The string length is
  computed by subtracting two from the value of the first byte of the descriptor.
  */

  /* Check which is lower size, the Size of string or the length of bytes read
  from the device */

  if (psrc[1] == USB_DESC_TYPE_STRING)
  {
    /* Make sure the Descriptor is String Type */

    /* psrc[0] contains Size of Descriptor, subtract 2 to get the length of string */
    strlength = ((((uint16_t)psrc[0] - 2U) <= length) ? ((uint16_t)psrc[0] - 2U) : length);

    /* Adjust the offset ignoring the String Len and Descriptor type */
    psrc += 2U;

    for (idx = 0U; idx < strlength; idx += 2U)
    {
      /* Copy Only the string and ignore the UNICODE ID, hence add the src */
      *pdest =  psrc[idx];
      pdest++;
    }
    *pdest = 0U; /* mark end of string */
  }
}


/**
  * @brief  USBH_GetNextDesc
  *         This function return the next descriptor header
  * @param  buf: Buffer where the cfg descriptor is available
  * @param  ptr: data pointer inside the cfg descriptor
  * @retval next header
  */
USBH_DescHeader_t  *USBH_GetNextDesc(uint8_t   *pbuf, uint16_t  *ptr)
{
  USBH_DescHeader_t  *pnext;

  *ptr += ((USBH_DescHeader_t *)(void *)pbuf)->bLength;
  pnext = (USBH_DescHeader_t *)(void *)((uint8_t *)(void *)pbuf + \
                                        ((USBH_DescHeader_t *)(void *)pbuf)->bLength);

  return (pnext);
}


/**
  * @brief  USBH_CtlReq
  *         USBH_CtlReq sends a control request and provide the status after
  *            completion of the request
  * @param  phost: Host Handle
  * @param  req: Setup Request Structure
  * @param  buff: data buffer address to store the response
  * @param  length: length of the response
  * @retval USBH Status
  */
USBH_StatusTypeDef USBH_CtlReq(USBH_HandleTypeDef *phost, uint8_t *buff,
                               uint16_t length)
{
  USBH_StatusTypeDef status;
  status = USBH_BUSY;

  switch (phost->RequestState)
  {
    case CMD_SEND:
      /* Start a SETUP transfer */
      phost->Control.buff = buff;
      phost->Control.length = length;
      phost->Control.state = CTRL_SETUP;
      phost->RequestState = CMD_WAIT;
      status = USBH_BUSY;

#if (USBH_USE_OS == 1U)
      phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
      (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
      (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      break;

    case CMD_WAIT:
      status = USBH_HandleControl(phost);
      if ((status == USBH_OK) || (status == USBH_NOT_SUPPORTED))
      {
        /* Transaction completed, move control state to idle */
        phost->RequestState = CMD_SEND;
        phost->Control.state = CTRL_IDLE;
      }
      else if (status == USBH_FAIL)
      {
        /* Failure Mode */
        phost->RequestState = CMD_SEND;
      }
      else
      {
        /* .. */
      }
#if (USBH_USE_OS == 1U)
      phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
      (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
      (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      break;

    default:
      break;
  }
  return status;
}


/**
  * @brief  USBH_HandleControl
  *         Handles the USB control transfer state machine
  * @param  phost: Host Handle
  * @retval USBH Status
  */
static USBH_StatusTypeDef USBH_HandleControl(USBH_HandleTypeDef *phost)
{
  uint8_t direction;
  USBH_StatusTypeDef status = USBH_BUSY;
  USBH_URBStateTypeDef URB_Status = USBH_URB_IDLE;

  switch (phost->Control.state)
  {
    case CTRL_SETUP:
      /* send a SETUP packet */
      (void)USBH_CtlSendSetup(phost, (uint8_t *)(void *)phost->Control.setup.d8,
                              phost->Control.pipe_out);

      phost->Control.state = CTRL_SETUP_WAIT;
      break;

    case CTRL_SETUP_WAIT:

      URB_Status = USBH_LL_GetURBState(phost, phost->Control.pipe_out);
      /* case SETUP packet sent successfully */
      if (URB_Status == USBH_URB_DONE)
      {
        direction = (phost->Control.setup.b.bmRequestType & USB_REQ_DIR_MASK);

        /* check if there is a data stage */
        if (phost->Control.setup.b.wLength.w != 0U)
        {
          if (direction == USB_D2H)
          {
            /* Data Direction is IN */
            phost->Control.state = CTRL_DATA_IN;
          }
          else
          {
            /* Data Direction is OUT */
            phost->Control.state = CTRL_DATA_OUT;
          }
        }
        /* No DATA stage */
        else
        {
          /* If there is No Data Transfer Stage */
          if (direction == USB_D2H)
          {
            /* Data Direction is IN */
            phost->Control.state = CTRL_STATUS_OUT;
          }
          else
          {
            /* Data Direction is OUT */
            phost->Control.state = CTRL_STATUS_IN;
          }
        }

#if (USBH_USE_OS == 1U)
        phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
        (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
        (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      }
      else
      {
        if ((URB_Status == USBH_URB_ERROR) || (URB_Status == USBH_URB_NOTREADY))
        {
          phost->Control.state = CTRL_ERROR;

#if (USBH_USE_OS == 1U)
          phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
          (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
          (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
        }
      }
      break;

    case CTRL_DATA_IN:
      /* Issue an IN token */
      phost->Control.timer = (uint16_t)phost->Timer;
      (void)USBH_CtlReceiveData(phost, phost->Control.buff,
                                phost->Control.length, phost->Control.pipe_in);

      phost->Control.state = CTRL_DATA_IN_WAIT;
      break;

    case CTRL_DATA_IN_WAIT:

      URB_Status = USBH_LL_GetURBState(phost, phost->Control.pipe_in);

      /* check is DATA packet transferred successfully */
      if (URB_Status == USBH_URB_DONE)
      {
        phost->Control.state = CTRL_STATUS_OUT;

#if (USBH_USE_OS == 1U)
        phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
        (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
        (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      }

      /* manage error cases*/
      if (URB_Status == USBH_URB_STALL)
      {
        /* In stall case, return to previous machine state*/
        status = USBH_NOT_SUPPORTED;

#if (USBH_USE_OS == 1U)
        phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
        (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
        (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      }
      else
      {
        if (URB_Status == USBH_URB_ERROR)
        {
          /* Device error */
          phost->Control.state = CTRL_ERROR;

#if (USBH_USE_OS == 1U)
          phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
          (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
          (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
        }
      }
      break;

    case CTRL_DATA_OUT:

      (void)USBH_CtlSendData(phost, phost->Control.buff, phost->Control.length,
                             phost->Control.pipe_out, 1U);

      phost->Control.timer = (uint16_t)phost->Timer;
      phost->Control.state = CTRL_DATA_OUT_WAIT;
      break;

    case CTRL_DATA_OUT_WAIT:

      URB_Status = USBH_LL_GetURBState(phost, phost->Control.pipe_out);

      if (URB_Status == USBH_URB_DONE)
      {
        /* If the Setup Pkt is sent successful, then change the state */
        phost->Control.state = CTRL_STATUS_IN;

#if (USBH_USE_OS == 1U)
        phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
        (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
        (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      }

      /* handle error cases */
      else if (URB_Status == USBH_URB_STALL)
      {
        /* In stall case, return to previous machine state*/
        phost->Control.state = CTRL_STALLED;
        status = USBH_NOT_SUPPORTED;

#if (USBH_USE_OS == 1U)
        phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
        (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
        (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      }
      else if (URB_Status == USBH_URB_NOTREADY)
      {
        /* Nack received from device */
        phost->Control.state = CTRL_DATA_OUT;

#if (USBH_USE_OS == 1U)
        phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
        (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
        (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      }
      else
      {
        if (URB_Status == USBH_URB_ERROR)
        {
          /* device error */
          phost->Control.state = CTRL_ERROR;
          status = USBH_FAIL;

#if (USBH_USE_OS == 1U)
          phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
          (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
          (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
        }
      }
      break;

    case CTRL_STATUS_IN:
      /* Send 0 bytes out packet */
      (void)USBH_CtlReceiveData(phost, NULL, 0U, phost->Control.pipe_in);

      phost->Control.timer = (uint16_t)phost->Timer;
      phost->Control.state = CTRL_STATUS_IN_WAIT;

      break;

    case CTRL_STATUS_IN_WAIT:

      URB_Status = USBH_LL_GetURBState(phost, phost->Control.pipe_in);

      if (URB_Status == USBH_URB_DONE)
      {
        /* Control transfers completed, Exit the State Machine */
        phost->Control.state = CTRL_COMPLETE;
        status = USBH_OK;

#if (USBH_USE_OS == 1U)
        phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
        (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
        (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      }
      else if (URB_Status == USBH_URB_ERROR)
      {
        phost->Control.state = CTRL_ERROR;

#if (USBH_USE_OS == 1U)
        phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
        (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
        (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      }
      else
      {
        if (URB_Status == USBH_URB_STALL)
        {
          /* Control transfers completed, Exit the State Machine */
          status = USBH_NOT_SUPPORTED;

#if (USBH_USE_OS == 1U)
          phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
          (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
          (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
        }
      }
      break;

    case CTRL_STATUS_OUT:
      (void)USBH_CtlSendData(phost, NULL, 0U, phost->Control.pipe_out, 1U);

      phost->Control.timer = (uint16_t)phost->Timer;
      phost->Control.state = CTRL_STATUS_OUT_WAIT;
      break;

    case CTRL_STATUS_OUT_WAIT:
      URB_Status = USBH_LL_GetURBState(phost, phost->Control.pipe_out);
      if (URB_Status == USBH_URB_DONE)
      {
        status = USBH_OK;
        phost->Control.state = CTRL_COMPLETE;

#if (USBH_USE_OS == 1U)
        phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
        (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
        (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      }
      else if (URB_Status == USBH_URB_NOTREADY)
      {
        phost->Control.state = CTRL_STATUS_OUT;

#if (USBH_USE_OS == 1U)
        phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
        (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
        (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
      }
      else
      {
        if (URB_Status == USBH_URB_ERROR || URB_Status == USBH_URB_STALL)
        {
          phost->Control.state = CTRL_ERROR;

#if (USBH_USE_OS == 1U)
          phost->os_msg = (uint32_t)USBH_CONTROL_EVENT;
#if (osCMSIS < 0x20000U)
          (void)osMessagePut(phost->os_event, phost->os_msg, 0U);
#else
          (void)osMessageQueuePut(phost->os_event, &phost->os_msg, 0U, 0U);
#endif
#endif
        }
      }
      break;

    case CTRL_ERROR:
      /*
      After a halt condition is encountered or an error is detected by the
      host, a control endpoint is allowed to recover by accepting the next Setup
      PID; i.e., recovery actions via some other pipe are not required for control
      endpoints. For the Default Control Pipe, a device reset will ultimately be
      required to clear the halt or error condition if the next Setup PID is not
      accepted.
      */
      if (++phost->Control.errorcount <= USBH_MAX_ERROR_COUNT)
      {
        /* Do the transmission again, starting from SETUP Packet */
        phost->Control.state = CTRL_SETUP;
        phost->RequestState = CMD_SEND;
      }
      else
      {
        phost->pUser(phost, HOST_USER_UNRECOVERED_ERROR);
        phost->Control.errorcount = 0U;
        USBH_ErrLog("Control error: Device not responding");

        /* Free control pipes */
        (void)USBH_FreePipe(phost, phost->Control.pipe_out);
        (void)USBH_FreePipe(phost, phost->Control.pipe_in);

        phost->gState = HOST_IDLE;
        status = USBH_FAIL;
      }
      break;

    default:
      break;
  }

  return status;
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
