/**
 * @file rndis.c
 * @brief RNDIS (Remote Network Driver Interface Specification)
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
#include "usbd_def.h"
#include "usbd_rndis.h"
#include "os_port.h"
#include "core/net.h"
#include "usbd_desc.h"
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

//RNDIS context
RndisContext rndisContext;

//List of supported OIDs
const uint32_t rndisSupportOidList[] =
{
   //Mandatory general OIDs
   OID_GEN_SUPPORTED_LIST,
   OID_GEN_HARDWARE_STATUS,
   OID_GEN_MEDIA_SUPPORTED,
   OID_GEN_MEDIA_IN_USE,
   OID_GEN_MAXIMUM_FRAME_SIZE,
   OID_GEN_LINK_SPEED,
   OID_GEN_TRANSMIT_BLOCK_SIZE,
   OID_GEN_RECEIVE_BLOCK_SIZE,
   OID_GEN_VENDOR_ID,
   OID_GEN_VENDOR_DESCRIPTION,
   OID_GEN_CURRENT_PACKET_FILTER,
   OID_GEN_MAXIMUM_TOTAL_SIZE,
   OID_GEN_MEDIA_CONNECT_STATUS,
   OID_GEN_MAXIMUM_SEND_PACKETS,
   //Mandatory 802.3 OIDs
   OID_802_3_PERMANENT_ADDRESS,
   OID_802_3_CURRENT_ADDRESS,
   OID_802_3_MULTICAST_LIST,
   OID_802_3_MAXIMUM_LIST_SIZE,
   OID_802_3_RCV_ERROR_ALIGNMENT,
   OID_802_3_XMIT_ONE_COLLISION,
   OID_802_3_XMIT_MORE_COLLISIONS,
};


/**
 * @brief RNDIS core initialization
 **/

void rndisInit(void)
{
   //Initialize RNDIS context
   rndisContext.state = RNDIS_STATE_UNINITIALIZED;
   rndisContext.linkState = FALSE;
   rndisContext.txState = FALSE;
   rndisContext.rxState = FALSE;
   rndisContext.packetFilter = 0;
   rndisContext.rxBufferLen = 0;
   rndisContext.encapsulatedRespLen = 0;
   rndisContext.CmdOpCode = 0;
   rndisContext.CmdLength = 0;
}


/**
 * @brief Process incoming RNDIS message
 * @param[in] message Pointer to the RNDIS message
 * @param[in] length Length of the RNDIS message, in bytes
 * @return Error code
 **/

error_t rndisProcessMsg(const RndisMsg *message, size_t length)
{
   error_t error;

   //Check the length of the message
   if(length < sizeof(RndisMsg))
      return ERROR_INVALID_LENGTH;

   //Debug message
   TRACE_DEBUG("RNDIS message received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump RNDIS message contents
   rndisDumpMsg(message, length);

   //Check message type
   switch(message->messageType)
   {
   //RNDIS Initialize message?
   case RNDIS_INITIALIZE_MSG:
      error = rndisProcessInitializeMsg((RndisInitializeMsg *) message, length);
      break;
   //RNDIS Halt message?
   case RNDIS_HALT_MSG:
      error = rndisProcessHaltMsg((RndisHaltMsg *) message, length);
      break;
   //RNDIS Query message?
   case RNDIS_QUERY_MSG:
      error = rndisProcessQueryMsg((RndisQueryMsg *) message, length);
      break;
   //RNDIS Set message?
   case RNDIS_SET_MSG:
      error = rndisProcessSetMsg((RndisSetMsg *) message, length);
      break;
   //RNDIS Reset message?
   case RNDIS_RESET_MSG:
      error = rndisProcessResetMsg((RndisResetMsg *) message, length);
      break;
   //RNDIS Keep-Alive message?
   case RNDIS_KEEPALIVE_MSG:
      error = rndisProcessKeepAliveMsg((RndisKeepAliveMsg *) message, length);
      break;
   //Unknown message type?
   default:
      error = ERROR_INVALID_TYPE;
      break;
   }

   //Return status code
   return error;
}


/**
 * @brief Process RNDIS Initialize message
 * @param[in] message Pointer to the RNDIS message
 * @param[in] length Length of the RNDIS message, in bytes
 * @return Error code
 **/

error_t rndisProcessInitializeMsg(const RndisInitializeMsg *message, size_t length)
{
   //Check the length of the message
   if(length < sizeof(RndisInitializeMsg))
      return ERROR_INVALID_LENGTH;

   //Debug message
   TRACE_DEBUG("RNDIS Initialize message received (%" PRIuSIZE " bytes)...\r\n", length);

   //Format the response to the Initialize message
   rndisFormatInitializeCmplt(message->requestId);

   //Send a notification to the host
   rndisSendNotification(RNDIS_NOTIFICATION_RESP_AVAILABLE);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process RNDIS Halt message
 * @param[in] message Pointer to the RNDIS message
 * @param[in] length Length of the RNDIS message, in bytes
 * @return Error code
 **/

error_t rndisProcessHaltMsg(const RndisHaltMsg *message, size_t length)
{
   //Check the length of the message
   if(length < sizeof(RndisHaltMsg))
      return ERROR_INVALID_LENGTH;

   //Debug message
   TRACE_DEBUG("RNDIS Halt message received (%" PRIuSIZE " bytes)...\r\n", length);

   //Switch to the RNDIS_UNINITIALIZED state
   rndisChangeState(RNDIS_STATE_UNINITIALIZED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process RNDIS Query message
 * @param[in] message Pointer to the RNDIS message
 * @param[in] length Length of the RNDIS message, in bytes
 * @return Error code
 **/

error_t rndisProcessQueryMsg(const RndisQueryMsg *message, size_t length)
{
   size_t n;
   uint32_t status;
   uint8_t *buffer;
   MacAddr macAddr;

   //Check the length of the message
   if(length < sizeof(RndisQueryMsg))
      return ERROR_INVALID_LENGTH;

   //Debug message
   TRACE_DEBUG("RNDIS Query message received (%" PRIuSIZE " bytes)...\r\n", length);

   //Point to the buffer where to format the response data
   buffer = rndisContext.encapsulatedResp + sizeof(RndisQueryCmplt);

   //Clear status code
   status = RNDIS_STATUS_SUCCESS;

   //Check the identifier of the object being queried for
   switch(message->oid)
   {
   case OID_GEN_SUPPORTED_LIST:
      //Retrieve the length of the list
      n = sizeof(rndisSupportOidList);
      //Copy the list
      osMemcpy(buffer, rndisSupportOidList, n);
      break;
   case OID_GEN_HARDWARE_STATUS:
      //Current hardware status of the underlying NIC
      STORE32LE(RNDIS_HARDWARE_STATUS_READY, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_GEN_MEDIA_SUPPORTED:
      //Media types that a NIC can support
      STORE32LE(RNDIS_MEDIUM_802_3, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_GEN_MEDIA_IN_USE:
      //Complete list of the media types that the NIC currently uses
      STORE32LE(RNDIS_MEDIUM_802_3, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_GEN_MAXIMUM_FRAME_SIZE:
      //Maximum network packet size, in bytes, that the NIC supports
      STORE32LE(1500, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_GEN_LINK_SPEED:
      //Maximum speed of the NIC
      STORE32LE(10000000 / 100, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_GEN_TRANSMIT_BLOCK_SIZE:
      //Minimum number of bytes that a single net packet occupies in
      //the transmit buffer space of the NIC
      STORE32LE(1518, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_GEN_RECEIVE_BLOCK_SIZE:
      //Amount of storage, in bytes, that a single packet occupies in
      //the receive buffer space of the NIC
      STORE32LE(1518, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_GEN_VENDOR_ID:
      //Three-byte IEEE-registered vendor code, followed by a single byte
      //that the vendor assigns to identify a particular NIC
      buffer[0] = 0xFF;
      buffer[1] = 0xFF;
      buffer[2] = 0xFF;
      buffer[3] = 0x00;
      n = 4;
      break;
   case OID_GEN_VENDOR_DESCRIPTION:
      //NULL-terminated string describing the NIC
      osStrcpy((char_t *) buffer, RNDIS_VENDOR_DESCRIPTION);
      n = osStrlen(RNDIS_VENDOR_DESCRIPTION) + 1;
      break;
   case OID_GEN_CURRENT_PACKET_FILTER:
      //Types of net packets for which a protocol receives indications
      //from a miniport driver
      STORE32LE(rndisContext.packetFilter, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_GEN_MAXIMUM_TOTAL_SIZE:
      //Maximum total packet length, in bytes, the NIC supports
      STORE32LE(1518, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_GEN_MEDIA_CONNECT_STATUS:
      //Connection status of the NIC on the network
      STORE32LE(RNDIS_MEDIA_STATE_CONNECTED, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_GEN_MAXIMUM_SEND_PACKETS:
      //Maximum number of send packet descriptors that can be accepted
      STORE32LE(1, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_802_3_PERMANENT_ADDRESS:
      //The address of the NIC encoded in the hardware
   case OID_802_3_CURRENT_ADDRESS:
      //The address the NIC is currently using
      macStringToAddr(RNDIS_MAC_ADDR, &macAddr);
      macCopyAddr(buffer, &macAddr);
      n = sizeof(MacAddr);
      break;
   case OID_802_3_MAXIMUM_LIST_SIZE:
      //Maximum number of 6-byte addresses that the multicast address list can hold
      STORE32LE(16, buffer);
      n = sizeof(uint32_t);
      break;
   case OID_802_3_RCV_ERROR_ALIGNMENT:
      //Number of frames received with alignment errors
   case OID_802_3_XMIT_ONE_COLLISION:
      //Number of frames successfully transmitted after exactly one collision
   case OID_802_3_XMIT_MORE_COLLISIONS:
      //Number of frames successfully transmitted after more than one collision
      STORE32LE(0, buffer);
      n = sizeof(uint32_t);
      break;
   default:
      //Unknown OID
      n = 0;
      //Report an error
      status = RNDIS_STATUS_FAILURE;
      break;
   }

   //Format the response to the Query message
   rndisFormatQueryCmplt(message->requestId, status, n);

   //Send a notification to the host
   rndisSendNotification(RNDIS_NOTIFICATION_RESP_AVAILABLE);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process RNDIS Set message
 * @param[in] message Pointer to the RNDIS message
 * @param[in] length Length of the RNDIS message, in bytes
 * @return Error code
 **/

error_t rndisProcessSetMsg(const RndisSetMsg *message, size_t length)
{
   uint32_t status;

   //Check the length of the message
   if(length < sizeof(RndisSetMsg))
      return ERROR_INVALID_LENGTH;

   //Debug message
   TRACE_DEBUG("RNDIS Set message received (%" PRIuSIZE " bytes)...\r\n", length);

   //Clear status code
   status = RNDIS_STATUS_SUCCESS;

   //Check the identifier of the object being queried for
   switch(message->oid)
   {
   case OID_GEN_CURRENT_PACKET_FILTER:
      //Types of net packets for which a protocol receives indications
      //from a miniport driver
      rndisContext.packetFilter = LOAD32LE(message->oidInputBuffer);
      break;
   case OID_802_3_MULTICAST_LIST:
      //List of multicast addresses on a miniport adapter
      break;
   default:
      //Report an error
      status = RNDIS_STATUS_FAILURE;
      break;
   }

   //Format the response to the Set message
   rndisFormatSetCmplt(message->requestId, status);

   //Send a notification to the host
   rndisSendNotification(RNDIS_NOTIFICATION_RESP_AVAILABLE);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process RNDIS Reset message
 * @param[in] message Pointer to the RNDIS message
 * @param[in] length Length of the RNDIS message, in bytes
 * @return Error code
 **/

error_t rndisProcessResetMsg(const RndisResetMsg *message, size_t length)
{
   //Check the length of the message
   if(length < sizeof(RndisResetMsg))
      return ERROR_INVALID_LENGTH;

   //Debug message
   TRACE_DEBUG("RNDIS Reset message received (%" PRIuSIZE " bytes)...\r\n", length);

   //Switch to the RNDIS_UNINITIALIZED state
   rndisChangeState(RNDIS_STATE_UNINITIALIZED);

   //Format the response to the Reset message
   rndisFormatResetCmplt();

   //Send a notification to the host
   rndisSendNotification(RNDIS_NOTIFICATION_RESP_AVAILABLE);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process RNDIS Keep-Alive message
 * @param[in] message Pointer to the RNDIS message
 * @param[in] length Length of the RNDIS message, in bytes
 * @return Error code
 **/

error_t rndisProcessKeepAliveMsg(const RndisKeepAliveMsg *message, size_t length)
{
   //Check the length of the message
   if(length < sizeof(RndisKeepAliveMsg))
      return ERROR_INVALID_LENGTH;

   //Debug message
   TRACE_DEBUG("RNDIS Keep-Alive message received (%" PRIuSIZE " bytes)...\r\n", length);

   //Format the response to the Keep-Alive message
   rndisFormatKeepAliveCmplt(message->requestId);

   //Send a notification to the host
   rndisSendNotification(RNDIS_NOTIFICATION_RESP_AVAILABLE);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format RNDIS Halt message
 * @return Error code
 **/

error_t rndisFormatHaltMsg(void)
{
   RndisHaltMsg *message;

   //Point to the buffer where to format the message
   message = (RndisHaltMsg *) rndisContext.encapsulatedResp;

   //Format the RNDIS Halt message
   message->messageType = RNDIS_HALT_MSG;
   message->messageLength = sizeof(RndisHaltMsg);
   message->requestId = 0;

   //Set the length of the response
   rndisContext.encapsulatedRespLen = message->messageLength;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format RNDIS Indicate Status message
 * @param[in] status RNDIS status code
 * @return Error code
 **/

error_t rndisFormatIndicateStatusMsg(uint32_t status)
{
   RndisIndicateStatusMsg *message;

   //Point to the buffer where to format the message
   message = (RndisIndicateStatusMsg *) rndisContext.encapsulatedResp;

   //Format the RNDIS Indicate Status message
   message->messageType = RNDIS_INDICATE_STATUS_MSG;
   message->messageLength = sizeof(RndisIndicateStatusMsg);
   message->status = status;
   message->statusBufferLength = 0;
   message->statusBufferOffset = 0;

   //Set the length of the response
   rndisContext.encapsulatedRespLen = message->messageLength;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format RNDIS Initialize Cmplt message
 * @param[in] requestId Request identifier
 * @return Error code
 **/

error_t rndisFormatInitializeCmplt(uint32_t requestId)
{
   RndisInitializeCmplt *message;

   //Point to the buffer where to format the response
   message = (RndisInitializeCmplt *) rndisContext.encapsulatedResp;

   //Format the RNDIS Initialize Cmplt message
   message->messageType = RNDIS_INITIALIZE_CMPLT;
   message->messageLength = sizeof(RndisInitializeCmplt);
   message->requestId = requestId;
   message->status = RNDIS_STATUS_SUCCESS;
   message->majorVersion = RNDIS_MAJOR_VERSION;
   message->minorVersion = RNDIS_MINOR_VERSION;
   message->deviceFlags = RNDIS_DF_CONNECTIONLESS;
   message->medium = RNDIS_MEDIUM_802_3;
   message->maxPacketsPerTransfer = 1;
   message->maxTransferSize = RNDIS_MAX_TRANSFER_SIZE;
   message->packetAlignmentFactor = 0;
   message->afListOffset = 0;
   message->afListSize = 0;

   //Set the length of the response
   rndisContext.encapsulatedRespLen = message->messageLength;

   //Switch to the RNDIS_INITIALIZED state
   rndisChangeState(RNDIS_STATE_INITIALIZED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format RNDIS Query Cmplt message
 * @param[in] requestId Request identifier
 * @param[in] status RNDIS status code
 * @param[in] length Length of the response data, in bytes
 * @return Error code
 **/

error_t rndisFormatQueryCmplt(uint32_t requestId, uint32_t status, uint32_t length)
{
   RndisQueryCmplt *message;

   //Point to the buffer where to format the response
   message = (RndisQueryCmplt *) rndisContext.encapsulatedResp;

   //Format the Query Cmplt message
   message->messageType = RNDIS_QUERY_CMPLT;
   message->messageLength = sizeof(RndisQueryCmplt) + length;
   message->requestId = requestId;
   message->status = status;
   message->infoBufferLength = length;
   message->infoBufferOffset = sizeof(RndisQueryCmplt) - 8;

   //Save the length of the response
   rndisContext.encapsulatedRespLen = message->messageLength;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format RNDIS Set Cmplt message
 * @param[in] requestId Request identifier
 * @param[in] status RNDIS status code
 * @return Error code
 **/

error_t rndisFormatSetCmplt(uint32_t requestId, uint32_t status)
{
   RndisSetCmplt *message;

   //Point to the buffer where to format the response
   message = (RndisSetCmplt *) rndisContext.encapsulatedResp;

   //Format the RNDIS Set Cmplt message
   message->messageType = RNDIS_SET_CMPLT;
   message->messageLength = sizeof(RndisSetCmplt);
   message->requestId = requestId;
   message->status = status;

   //Set the length of the response
   rndisContext.encapsulatedRespLen = message->messageLength;

   //Switch to the RNDIS_DATA_INITIALIZED state
   rndisChangeState(RNDIS_STATE_DATA_INITIALIZED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format RNDIS Reset Cmplt message
 * @return Error code
 **/

error_t rndisFormatResetCmplt(void)
{
   RndisResetCmplt *message;

   //Point to the buffer where to format the response
   message = (RndisResetCmplt *) rndisContext.encapsulatedResp;

   //Format the RNDIS Reset Cmplt message
   message->messageType = RNDIS_RESET_CMPLT;
   message->messageLength = sizeof(RndisResetCmplt);
   message->status = RNDIS_STATUS_SUCCESS;
   message->addressingReset = 1;

   //Set the length of the response
   rndisContext.encapsulatedRespLen = message->messageLength;

   //Switch to the RNDIS_INITIALIZED state
   rndisChangeState(RNDIS_STATE_INITIALIZED);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format RNDIS Keep-Alive Cmplt message
 * @param[in] requestId Request identifier
 * @return Error code
 **/

error_t rndisFormatKeepAliveCmplt(uint32_t requestId)
{
   RndisKeepAliveCmplt *message;

   //Point to the buffer where to format the response
   message = (RndisKeepAliveCmplt *) rndisContext.encapsulatedResp;

   //Format the RNDIS Keep-Alive Cmplt message
   message->messageType = RNDIS_KEEPALIVE_CMPLT;
   message->messageLength = sizeof(RndisKeepAliveCmplt);
   message->requestId = requestId;
   message->status = RNDIS_STATUS_SUCCESS;

   //Set the length of the response
   rndisContext.encapsulatedRespLen = message->messageLength;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Send RNDIS notification message
 * @param[in] notification Device notification
 * @return Error code
 **/

error_t rndisSendNotification(uint32_t notification)
{
   static RndisNotificationMsg message;

   //Prepare a notification message
   message.notification = RNDIS_NOTIFICATION_RESP_AVAILABLE;
   message.reserved = 0;

   //Debug message
   TRACE_DEBUG("Sending RNDIS notification...\r\n");
   TRACE_DEBUG("  Notification = 0x%08" PRIX32 "\r\n", message.notification);
   TRACE_DEBUG("  Reserved = 0x%08" PRIX32 "\r\n", message.reserved);

   //Send the notification to the USB host
   USBD_LL_Transmit(&USBD_Device, RNDIS_NOTIFICATION_EP,
      (uint8_t *) &message, sizeof(RndisNotificationMsg));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Update RNDIS state
 * @param[in] newState New RNDIS state to switch to
 **/

void rndisChangeState(RndisState newState)
{
   //Check transition event
   if(rndisContext.state != RNDIS_STATE_DATA_INITIALIZED &&
      newState == RNDIS_STATE_DATA_INITIALIZED)
   {
      //The link is up
      rndisContext.linkState = TRUE;
      rndisContext.linkEvent = TRUE;

      //Sanity check
      if(rndisDriverInterface != NULL)
      {
         //Notify the user that the link state has changed
         rndisDriverInterface->nicEvent = TRUE;
         osSetEventFromIsr(&netEvent);
      }
   }
   else if(rndisContext.state != RNDIS_STATE_UNINITIALIZED &&
      newState == RNDIS_STATE_UNINITIALIZED)
   {
      //The link is down
      rndisContext.linkState = FALSE;
      rndisContext.linkEvent = TRUE;

      //Sanity check
      if(rndisDriverInterface != NULL)
      {
         //Notify the user that the link state has changed
         rndisDriverInterface->nicEvent = TRUE;
         osSetEventFromIsr(&netEvent);
      }
   }

   //Switch to the new state
   rndisContext.state = newState;
}
