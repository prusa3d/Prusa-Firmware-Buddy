/**
 * @file usbd_rndis.c
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

//Switch to the appropriate trace level
#define TRACE_LEVEL TRACE_LEVEL_INFO

//Dependencies
#include "usbd_ctlreq.h"
#include "usbd_desc.h"
#include "usbd_rndis.h"
#include "core/net.h"
#include "rndis.h"
#include "rndis_driver.h"
#include "rndis_debug.h"
#include "debug.h"

//Debug macros
#if (TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   #undef TRACE_DEBUG
   #define TRACE_DEBUG(...) fprintf(stderr, __VA_ARGS__)
   #undef TRACE_DEBUG_ARRAY
   #define TRACE_DEBUG_ARRAY(p, a, n) debugDisplayArray(stderr, p, a, n)
#endif


/**
 * @brief RNDIS class callbacks
 **/

USBD_ClassTypeDef usbdRndisClass =
{
   usbdRndisInit,
   usbdRndisDeInit,
   usbdRndisSetup,
   NULL,
   usbdRndisEp0RxReady,
   usbdRndisDataIn,
   usbdRndisDataOut,
   NULL,
   NULL,
   NULL,
   usbdRndisGetHighSpeedConfigDesc,
   usbdRndisGetFullSpeedConfigDesc,
   usbdRndisGetOtherSpeedConfigDesc,
   usbdRndisGetDeviceQualifierDesc,
};


/**
 * @brief RNDIS class initialization
 * @param[in] pdev Pointer to a USBD_HandleTypeDef structure
 * @param[in] cfgidx Configuration index
 * @return Status code
 **/

uint8_t usbdRndisInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
   //Check current speed
   if(pdev->dev_speed == USBD_SPEED_HIGH)
   {
      //Open DATA IN endpoint
      USBD_LL_OpenEP(pdev, RNDIS_DATA_IN_EP,
         USBD_EP_TYPE_BULK, RNDIS_DATA_IN_EP_MPS_HS);

      //Open DATA OUT endpoint
      USBD_LL_OpenEP(pdev, RNDIS_DATA_OUT_EP,
         USBD_EP_TYPE_BULK, RNDIS_DATA_OUT_EP_MPS_HS);
   }
   else
   {
      //Open DATA IN endpoint
      USBD_LL_OpenEP(pdev, RNDIS_DATA_IN_EP,
         USBD_EP_TYPE_BULK, RNDIS_DATA_IN_EP_MPS_FS);

      //Open DATA OUT endpoint
      USBD_LL_OpenEP(pdev, RNDIS_DATA_OUT_EP,
         USBD_EP_TYPE_BULK, RNDIS_DATA_OUT_EP_MPS_FS);
   }

   //Open notification endpoint
   USBD_LL_OpenEP(pdev, RNDIS_NOTIFICATION_EP,
      USBD_EP_TYPE_INTR, RNDIS_NOTIFICATION_EP_MPS);

   //Initialize RNDIS class context
   rndisInit();

   //Link the RNDIS class context
   pdev->pClassData = &rndisContext;

   //Debug message
   TRACE_DEBUG("### usbdRndisReceivePacket 000 ###\r\n");

   //Prepare DATA OUT endpoint for reception
   USBD_LL_PrepareReceive(pdev, RNDIS_DATA_OUT_EP,
      rndisContext.rxBuffer, RNDIS_DATA_OUT_EP_MPS_FS);

   //Reception is active
   rndisContext.rxState = TRUE;

   //Switch to the RNDIS_BUS_INITIALIZED state
   rndisChangeState(RNDIS_STATE_BUS_INITIALIZED);

   //Successful initialization
   return USBD_OK;
}


/**
 * @brief RNDIS class de-initialization
 * @param[in] pdev Pointer to a USBD_HandleTypeDef structure
 * @param[in] cfgidx Configuration index
 * @return Status code
 **/

uint8_t usbdRndisDeInit(USBD_HandleTypeDef *pdev, uint8_t cfgidx)
{
   //Close DATA IN endpoint
   USBD_LL_CloseEP(pdev, RNDIS_DATA_IN_EP);
   //Close DATA OUT endpoint
   USBD_LL_CloseEP(pdev, RNDIS_DATA_OUT_EP);
   //Close notification endpoint
   USBD_LL_CloseEP(pdev, RNDIS_NOTIFICATION_EP);

   //Unlink the RNDIS class context
   pdev->pClassData = NULL;

   //Switch to the RNDIS_UNINITIALIZED state
   rndisChangeState(RNDIS_STATE_UNINITIALIZED);

   //Successful processing
   return USBD_OK;
}


/**
 * @brief Process incoming setup request
 * @param[in] pdev Pointer to a USBD_HandleTypeDef structure
 * @param[in] req Pointer to the setup request
 * @return Status code
 **/

uint8_t usbdRndisSetup(USBD_HandleTypeDef *pdev, USBD_SetupReqTypedef *req)
{
   //Debug message
   TRACE_DEBUG("USB setup packet received...\r\n");
   TRACE_DEBUG("  bmRequest = 0x%02" PRIX8 "\r\n", req->bmRequest);
   TRACE_DEBUG("  bRequest = 0x%02" PRIX8 "\r\n", req->bRequest);
   TRACE_DEBUG("  wValue = 0x%04" PRIX16 "\r\n", req->wValue);
   TRACE_DEBUG("  wIndex = 0x%04" PRIX16 "\r\n", req->wIndex);
   TRACE_DEBUG("  wLength = 0x%04" PRIX16 "\r\n", req->wLength);

   //Check request type
   switch(req->bmRequest & USB_REQ_TYPE_MASK)
   {
   //Standard request?
   case USB_REQ_TYPE_STANDARD:
      //GET INTERFACE request?
      if(req->bRequest == USB_REQ_GET_INTERFACE)
      {
         //A single alternate setting is supported
         static uint8_t alternateSetting = 0;
         //Return data back to the host
         USBD_CtlSendData(pdev, &alternateSetting, 1);
      }
      //SET INTERFACE request
      else if(req->bRequest == USB_REQ_SET_INTERFACE)
      {
         //The device only supports a single alternate setting
      }
      break;
   //Class specific request?
   case USB_REQ_TYPE_CLASS:
      //Check direction
      if((req->bmRequest & 0x80) != 0)
      {
         //GET ENCAPSULATED RESPONSE request?
         if(req->bRequest == RNDIS_GET_ENCAPSULATED_RESPONSE)
         {
            //If for some reason the device receives a GET ENCAPSULATED RESPONSE
            //and is unable to respond with a valid data on the Control endpoint,
            //then it should return a one-byte packet set to 0x00, rather than
            //stalling the Control endpoint
            if(rndisContext.encapsulatedRespLen == 0)
            {
               rndisContext.encapsulatedResp[0] = 0;
               rndisContext.encapsulatedRespLen = 1;
            }

            //Debug message
            TRACE_DEBUG("Sending encapsulated response (%" PRIuSIZE " bytes)...\r\n", rndisContext.encapsulatedRespLen);
            TRACE_DEBUG_ARRAY("  ", rndisContext.encapsulatedResp, rndisContext.encapsulatedRespLen);

            //Debug message
            TRACE_DEBUG("Sending RNDIS message (%" PRIuSIZE " bytes)...\r\n", rndisContext.encapsulatedRespLen);
            //Dump RNDIS message contents
            rndisDumpMsg((RndisMsg *) rndisContext.encapsulatedResp, rndisContext.encapsulatedRespLen);

            //Return data back to the host
            USBD_CtlSendData(pdev, rndisContext.encapsulatedResp, rndisContext.encapsulatedRespLen);

            //Flush the response buffer
            rndisContext.encapsulatedRespLen = 0;
         }
         else
         {
            //Return data back to the host
            USBD_CtlSendData(pdev, NULL, 0);
         }
      }
      else
      {
         if(req->wLength != 0)
         {
            //Save request type
            rndisContext.CmdOpCode = req->bRequest;
            rndisContext.CmdLength = req->wLength;

            //Prepare receiving data on EP0
            USBD_CtlPrepareRx(pdev, (uint8_t *) rndisContext.data, req->wLength);
         }
      }
      break;
   //Unknown request?
   default:
      break;
   }

   //Successful processing
   return USBD_OK;
}


/**
 * @brief Handle data stage (control endpoint)
 * @param[in] pdev Pointer to a USBD_HandleTypeDef structure
 * @return Status code
 **/

uint8_t usbdRndisEp0RxReady(USBD_HandleTypeDef *pdev)
{
   //Debug message
   TRACE_DEBUG("  Data (opcode=0x%02" PRIX8 ", length=%u)\r\n", rndisContext.CmdOpCode, rndisContext.CmdLength);
   TRACE_DEBUG_ARRAY("    ", rndisContext.data, rndisContext.CmdLength);

   if(rndisContext.CmdOpCode != 0xFF)
   {
      //Process RNDIS message
      rndisProcessMsg((RndisMsg *) rndisContext.data, rndisContext.CmdLength);

      rndisContext.CmdOpCode = 0xFF;
   }

   //Successful processing
   return USBD_OK;
}


/**
 * @brief DATA IN callback
 * @param[in] pdev Pointer to a USBD_HandleTypeDef structure
 * @param[in] epnum Endpoint number
 * @return Status code
 **/

uint8_t usbdRndisDataIn(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
   //DATA IN endpoint?
   if((epnum & 0x7F) == (RNDIS_DATA_IN_EP & 0x7F))
   {
      //Debug message
      TRACE_DEBUG("########## USB DATA IN EP sent #############\r\n");

      //The current buffer has been transmitted and is now available for writing
      rndisTxBuffer[rndisTxReadIndex].ready = FALSE;

      //Increment index and wrap around if necessary
      if(++rndisTxReadIndex >= RNDIS_TX_BUFFER_COUNT)
         rndisTxReadIndex = 0;

      //Check whether the next buffer is ready
      if(rndisTxBuffer[rndisTxReadIndex].ready)
      {
         //Start transmitting data
         USBD_LL_Transmit(pdev, RNDIS_DATA_IN_EP,
            rndisTxBuffer[rndisTxReadIndex].data,
            rndisTxBuffer[rndisTxReadIndex].length);
      }
      else
      {
         //Suspend transmission
         rndisContext.txState = FALSE;
      }

      //The transmitter can accept another packet
      osSetEventFromIsr(&rndisDriverInterface->nicTxEvent);
   }

   //Successful processing
   return USBD_OK;
}


/**
 * @brief DATA OUT callback
 * @param[in] pdev Pointer to a USBD_HandleTypeDef structure
 * @param[in] epnum Endpoint number
 * @return Status code
 **/

uint8_t usbdRndisDataOut(USBD_HandleTypeDef *pdev, uint8_t epnum)
{
   size_t length;
   RndisRxBufferDesc *rxBufferDesc;

   //DATA OUT endpoint?
   if((epnum & 0x7F) == (RNDIS_DATA_OUT_EP & 0x7F))
   {
      //Retrieve the length of the packet
      length = USBD_LL_GetRxDataSize(pdev, epnum);

      //Debug message
      TRACE_DEBUG("Data received on DATA OUT endpoint (%" PRIuSIZE " bytes)\r\n", length);

      //Make sure the total length is acceptable
      if((rndisContext.rxBufferLen + length) <= RNDIS_MAX_TRANSFER_SIZE)
      {
         //Point to the current buffer descriptor
         rxBufferDesc = &rndisRxBuffer[rndisRxWriteIndex];

         //Copy data
         osMemcpy(rxBufferDesc->data + rndisContext.rxBufferLen,
            rndisContext.rxBuffer, length);

         //Update the length of the RX buffer
         rndisContext.rxBufferLen += length;

         //Last packet?
         if(length < RNDIS_DATA_OUT_EP_MPS_FS)
         {
            //Debug message
            TRACE_DEBUG("RNDIS Packet message received (%" PRIuSIZE " bytes)...\r\n",
               rxBufferDesc->length);
            //Dump RNDIS Packet message contents
            rndisDumpMsg((RndisMsg *) rxBufferDesc->data, rxBufferDesc->length);

            //Store the length of the message
            rxBufferDesc->length = rndisContext.rxBufferLen;
            //The current buffer is available for reading
            rxBufferDesc->ready = TRUE;

            //Increment index and wrap around if necessary
            if(++rndisRxWriteIndex >= RNDIS_RX_BUFFER_COUNT)
               rndisRxWriteIndex = 0;

            //Suspend reception
            rndisContext.rxState = FALSE;

            //Set event flag
            rndisDriverInterface->nicEvent = TRUE;
            //Notify the TCP/IP stack of the event
            osSetEventFromIsr(&netEvent);

            //Flush RX buffer
            rndisContext.rxBufferLen = 0;
         }
         else
         {
            //Debug message
            TRACE_DEBUG("### usbdRndisReceivePacket 222 ###\r\n");

            //Prepare DATA OUT endpoint for reception
            USBD_LL_PrepareReceive(&USBD_Device, RNDIS_DATA_OUT_EP,
               rndisContext.rxBuffer, RNDIS_DATA_OUT_EP_MPS_FS);
         }
      }
      else
      {
         //Flush RX buffer
         rndisContext.rxBufferLen = 0;

         //Debug message
         TRACE_DEBUG("### usbdRndisReceivePacket 333 ###\r\n");

         //Prepare DATA OUT endpoint for reception
         USBD_LL_PrepareReceive(&USBD_Device, RNDIS_DATA_OUT_EP,
            rndisContext.rxBuffer, RNDIS_DATA_OUT_EP_MPS_FS);
      }
   }

   //Successful processing
   return USBD_OK;
}


/**
 * @brief Retrieve configuration descriptor (high speed)
 * @param[out] length Length of the descriptor, in bytes
 * @return Pointer to the descriptor
 **/

uint8_t *usbdRndisGetHighSpeedConfigDesc(uint16_t *length)
{
   //Not implemented
   *length = 0;
   return NULL;
}


/**
 * @brief Retrieve configuration descriptor (full speed)
 * @param[out] length Length of the descriptor, in bytes
 * @return Pointer to the descriptor
 **/

uint8_t *usbdRndisGetFullSpeedConfigDesc(uint16_t *length)
{
   //Not implemented
   *length = sizeof(usbdConfigDescriptors);
   return (uint8_t *) &usbdConfigDescriptors;
}


/**
 * @brief Retrieve configuration descriptor (other speed)
 * @param[out] length Length of the descriptor, in bytes
 * @return Pointer to the descriptor
 **/

uint8_t *usbdRndisGetOtherSpeedConfigDesc(uint16_t *length)
{
   //Not implemented
   *length = 0;
   return NULL;
}


/**
 * @brief Retrieve device qualifier descriptor
 * @param[out] length Length of the descriptor, in bytes
 * @return Pointer to the descriptor
 **/

uint8_t *usbdRndisGetDeviceQualifierDesc(uint16_t *length)
{
   //Not implemented
   *length = 0;
   return NULL;
}
