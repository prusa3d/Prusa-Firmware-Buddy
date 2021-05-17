/**
 * @file ppp_misc.c
 * @brief PPP miscellaneous functions
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
#define TRACE_LEVEL PPP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "ppp/ppp_misc.h"
#include "ppp/ppp_debug.h"
#include "ppp/lcp.h"
#include "ppp/ipcp.h"
#include "ppp/ipv6cp.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (PPP_SUPPORT == ENABLED)


/**
 * @brief Send Configure-Ack, Nak or Reject packet
 * @param[in] context PPP context
 * @param[in] configureReqPacket Pointer to the incoming Configure-Request
 * @param[in] protocol Protocol field
 * @param[in] code Code field
 * @return Error code
 **/

error_t pppSendConfigureAckNak(PppContext *context,
   const PppConfigurePacket *configureReqPacket, PppProtocol protocol, PppCode code)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PppConfigurePacket *configureAckNakPacket;
   PppOption *option;

   //Initialize status code
   error = NO_ERROR;
   //Retrieve the length of the Configure-Request packet
   length = ntohs(configureReqPacket->length);

   //Allocate a buffer memory to hold the Configure-Ack, Nak or Reject packet
   buffer = pppAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the beginning of the packet
   configureAckNakPacket = netBufferAt(buffer, offset);

   //Format packet header
   configureAckNakPacket->code = code;
   configureAckNakPacket->identifier = configureReqPacket->identifier;
   configureAckNakPacket->length = sizeof(PppConfigurePacket);

   //Retrieve the length of the option list
   length -= sizeof(PppConfigurePacket);
   //Point to the first option
   option = (PppOption *) configureReqPacket->options;

   //Parse configuration options
   while(length > 0)
   {
      //LCP protocol?
      if(protocol == PPP_PROTOCOL_LCP)
      {
         //Parse LCP option
         lcpParseOption(context, option, length, configureAckNakPacket);
      }
#if (IPV4_SUPPORT == ENABLED)
      //IPCP protocol?
      else if(protocol == PPP_PROTOCOL_IPCP)
      {
         //Parse IPCP option
         ipcpParseOption(context, option, length, configureAckNakPacket);
      }
#endif
#if (IPV6_SUPPORT == ENABLED)
      //IPV6CP protocol?
      else if(protocol == PPP_PROTOCOL_IPV6CP)
      {
         //Parse IPV6CP option
         ipv6cpParseOption(context, option, length, configureAckNakPacket);
      }
#endif

      //Remaining bytes to process
      length -= option->length;
      //Jump to the next option
      option = (PppOption *) ((uint8_t *) option + option->length);
   }

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + configureAckNakPacket->length);
   //Convert length field to network byte order
   configureAckNakPacket->length = htons(configureAckNakPacket->length);

   //Debug message
   if(code == PPP_CODE_CONFIGURE_ACK)
   {
      TRACE_INFO("Sending Configure-Ack packet (%" PRIuSIZE " bytes)...\r\n",
         ntohs(configureAckNakPacket->length));
   }
   else if(code == PPP_CODE_CONFIGURE_NAK)
   {
      TRACE_INFO("Sending Configure-Nak packet (%" PRIuSIZE " bytes)...\r\n",
         ntohs(configureAckNakPacket->length));
   }
   else if(code == PPP_CODE_CONFIGURE_REJ)
   {
      TRACE_INFO("Sending Configure-Reject packet (%" PRIuSIZE " bytes)...\r\n",
         ntohs(configureAckNakPacket->length));
   }

   //Dump packet contents for debugging purpose
   pppDumpPacket((PppPacket *) configureAckNakPacket,
      ntohs(configureAckNakPacket->length), protocol);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, protocol);

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send Terminate-Request packet
 * @param[in] context PPP context
 * @param[in] identifier Identifier field
 * @param[in] protocol Protocol field
 * @return Error code
 **/

error_t pppSendTerminateReq(PppContext *context,
   uint8_t identifier, PppProtocol protocol)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PppTerminatePacket *terminateReqPacket;

   //Length of the Terminate-Request packet
   length = sizeof(PppTerminatePacket);

   //Allocate a buffer memory to hold the Terminate-Request packet
   buffer = pppAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Terminate-Request packet
   terminateReqPacket = netBufferAt(buffer, offset);

   //Format packet header
   terminateReqPacket->code = PPP_CODE_TERMINATE_REQ;
   terminateReqPacket->identifier = identifier;
   terminateReqPacket->length = htons(length);

   //Debug message
   TRACE_INFO("Sending Terminate-Request packet (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump packet contents for debugging purpose
   pppDumpPacket((PppPacket *) terminateReqPacket, length, protocol);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, protocol);

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send Terminate-Ack packet
 * @param[in] context PPP context
 * @param[in] identifier Identifier field
 * @param[in] protocol Protocol field
 * @return Error code
 **/

error_t pppSendTerminateAck(PppContext *context,
   uint8_t identifier, PppProtocol protocol)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PppTerminatePacket *terminateAckPacket;

   //Length of the Terminate-Ack packet
   length = sizeof(PppTerminatePacket);

   //Allocate a buffer memory to hold the Terminate-Ack packet
   buffer = pppAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Terminate-Ack packet
   terminateAckPacket = netBufferAt(buffer, offset);

   //Format packet header
   terminateAckPacket->code = PPP_CODE_TERMINATE_ACK;
   terminateAckPacket->identifier = identifier;
   terminateAckPacket->length = htons(length);

   //Debug message
   TRACE_INFO("Sending Terminate-Ack packet (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump packet contents for debugging purpose
   pppDumpPacket((PppPacket *) terminateAckPacket, length, protocol);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, protocol);

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send Code-Reject packet
 * @param[in] context PPP context
 * @param[in] packet Un-interpretable packet received from the peer
 * @param[in] identifier Identifier field
 * @param[in] protocol Protocol field
 * @return Error code
 **/

error_t pppSendCodeRej(PppContext *context, const PppPacket *packet,
   uint8_t identifier, PppProtocol protocol)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PppCodeRejPacket *codeRejPacket;

   //Calculate the length of the Code-Reject packet
   length = ntohs(packet->length) + sizeof(PppCodeRejPacket);

   //The rejected packet must be truncated to comply with
   //the peer's established MRU
   length = MIN(length, context->peerConfig.mru);

   //Allocate a buffer memory to hold the Code-Reject packet
   buffer = pppAllocBuffer(sizeof(PppCodeRejPacket), &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Code-Reject packet
   codeRejPacket = netBufferAt(buffer, offset);

   //Format packet header
   codeRejPacket->code = PPP_CODE_CODE_REJ;
   codeRejPacket->identifier = identifier;
   codeRejPacket->length = htons(length);

   //The Rejected-Packet field contains a copy of the packet which is being rejected
   error = netBufferAppend(buffer, packet, length - sizeof(PppCodeRejPacket));

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending Code-Reject packet (%" PRIuSIZE " bytes)...\r\n", length);

      //Send PPP frame
      error = pppSendFrame(context->interface, buffer, offset, protocol);
   }

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send Protocol-Reject packet
 * @param[in] context PPP context
 * @param[in] identifier Identifier field
 * @param[in] protocol Rejected protocol
 * @param[in] information Rejected information
 * @param[in] length Length of the rejected information
 * @return Error code
 **/

error_t pppSendProtocolRej(PppContext *context, uint8_t identifier,
   uint16_t protocol, const uint8_t *information, size_t length)
{
   error_t error;
   size_t offset;
   NetBuffer *buffer;
   PppProtocolRejPacket *protocolRejPacket;

   //Calculate the length of the Protocol-Reject packet
   length += sizeof(PppProtocolRejPacket);

   //The Rejected-Information must be truncated to comply with
   //the peer's established MRU
   length = MIN(length, context->peerConfig.mru);

   //Allocate a buffer memory to hold the Protocol-Reject packet
   buffer = pppAllocBuffer(sizeof(PppProtocolRejPacket), &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Protocol-Reject packet
   protocolRejPacket = netBufferAt(buffer, offset);

   //Format packet header
   protocolRejPacket->code = PPP_CODE_PROTOCOL_REJ;
   protocolRejPacket->identifier = identifier;
   protocolRejPacket->length = htons(length);
   protocolRejPacket->rejectedProtocol = htons(protocol);

   //The Rejected-Information field contains a copy of the
   //packet which is being rejected
   error = netBufferAppend(buffer, information,
      length - sizeof(PppProtocolRejPacket));

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending Protocol-Reject packet (%" PRIuSIZE " bytes)...\r\n", length);

      //Send PPP frame
      error = pppSendFrame(context->interface, buffer, offset, PPP_PROTOCOL_LCP);
   }

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send Echo-Reply packet
 * @param[in] context PPP context
 * @param[in] echoReqPacket Echo-Request packet received from the peer
 * @param[in] protocol Protocol field
 * @return Error code
 **/

error_t pppSendEchoRep(PppContext *context,
   const PppEchoPacket *echoReqPacket, PppProtocol protocol)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PppEchoPacket *echoRepPacket;

   //Retrieve the length of the Echo-Request packet
   length = ntohs(echoReqPacket->length);

   //Make sure the length is valid
   if(length < sizeof(PppEchoPacket))
      return ERROR_INVALID_LENGTH;
   if(length > context->peerConfig.mru)
      return ERROR_INVALID_LENGTH;

   //Allocate a buffer memory to hold the Echo-Reply packet
   buffer = pppAllocBuffer(sizeof(PppEchoPacket), &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Echo-Reply packet
   echoRepPacket = netBufferAt(buffer, offset);

   //Format packet header
   echoRepPacket->code = PPP_CODE_ECHO_REP;
   echoRepPacket->identifier = echoReqPacket->identifier;
   echoRepPacket->length = htons(length);
   echoRepPacket->magicNumber = context->localConfig.magicNumber;

   //The data field of the Echo-Request packet is copied into the data
   //field of the Echo-Reply packet
   error = netBufferAppend(buffer, echoReqPacket->data, length - sizeof(PppEchoPacket));

   //Check status code
   if(!error)
   {
      //Debug message
      TRACE_INFO("Sending Echo-Reply packet (%" PRIuSIZE " bytes)...\r\n", length);

      //Send PPP frame
      error = pppSendFrame(context->interface, buffer, offset, protocol);
   }

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Add an option to a Configure packet
 * @param[in,out] packet Pointer to the Configure packet
 * @param[in] optionType Option type
 * @param[in] optionValue Option value
 * @param[in] optionLen Length of the option value
 * @return Error code
 **/

error_t pppAddOption(PppConfigurePacket *packet, uint8_t optionType,
   const void *optionValue, uint8_t optionLen)
{
   PppOption *option;

   //Make sure the length is valid
   if(optionLen > (UINT8_MAX - sizeof(PppOption)))
      return ERROR_INVALID_LENGTH;

   //Point to the end of the Configure packet
   option = (PppOption *) ((uint8_t *) packet + packet->length);

   //Write specified option at current location
   option->type = optionType;
   option->length = optionLen + sizeof(PppOption);
   //Copy option data
   osMemcpy(option->data, optionValue, optionLen);

   //Update the length of the Configure packet
   packet->length += optionLen + sizeof(PppOption);

   //Successful processing
   return NO_ERROR;
}

#endif
