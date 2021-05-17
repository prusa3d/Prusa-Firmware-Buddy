/**
 * @file ipv6cp.c
 * @brief IPV6CP (PPP IPv6 Control Protocol)
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
#include "ipv6/ipv6.h"
#include "ipv6/ipv6_misc.h"
#include "ppp/ppp_fsm.h"
#include "ppp/ppp_misc.h"
#include "ppp/ppp_debug.h"
#include "ppp/ipv6cp.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (PPP_SUPPORT == ENABLED && IPV6_SUPPORT == ENABLED)


/**
 * @brief IPV6CP FSM callbacks
 **/

const PppCallbacks ipv6cpCallbacks =
{
   ipv6cpThisLayerUp,
   ipv6cpThisLayerDown,
   ipv6cpThisLayerStarted,
   ipv6cpThisLayerFinished,
   ipv6cpInitRestartCount,
   ipv6cpZeroRestartCount,
   ipv6cpSendConfigureReq,
   ipv6cpSendConfigureAck,
   ipv6cpSendConfigureNak,
   ipv6cpSendConfigureRej,
   ipv6cpSendTerminateReq,
   ipv6cpSendTerminateAck,
   ipv6cpSendCodeRej,
   NULL
};


/**
 * @brief IPV6CP Open event
 * @param[in] context PPP context
 * @return Error code
 **/

error_t ipv6cpOpen(PppContext *context)
{
   //Debug message
   TRACE_INFO("\r\nIPV6CP Open event\r\n");

   //The link is administratively available for traffic
   pppOpenEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks);
   //The lower layer is ready to carry packets
   pppUpEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief IPV6CP Close event
 * @param[in] context PPP context
 * @return Error code
 **/

error_t ipv6cpClose(PppContext *context)
{
   //Debug message
   TRACE_INFO("\r\nIPV6CP Close event\r\n");

   //The lower layer is no longer ready to carry packets
   pppDownEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks);
   //The link is no longer available for traffic
   pppCloseEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief IPV6CP timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * manage retransmissions
 *
 * @param[in] context PPP context
 **/

void ipv6cpTick(PppContext *context)
{
   //Check whether the restart timer is running
   if(context->ipv6cpFsm.state >= PPP_STATE_4_CLOSING &&
      context->ipv6cpFsm.state <= PPP_STATE_8_ACK_SENT)
   {
      //Get current time
      systime_t time = osGetSystemTime();

      //Check restart timer
      if((time - context->ipv6cpFsm.timestamp) >= PPP_RESTART_TIMER)
      {
         //Debug message
         TRACE_INFO("\r\nIPV6CP Timeout event\r\n");

         //The restart timer is used to retransmit Configure-Request
         //and Terminate-Request packets
         pppTimeoutEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks);
      }
   }
}


/**
 * @brief Process an incoming IPV6CP packet
 * @param[in] context PPP context
 * @param[in]  packet IPV6CP packet received from the peer
 * @param[in] length Length of the packet, in bytes
 **/

void ipv6cpProcessPacket(PppContext *context, const PppPacket *packet, size_t length)
{
   //Ensure the length of the incoming IPV6CP packet is valid
   if(length < sizeof(PppPacket))
      return;

   //Check the length field
   if(ntohs(packet->length) > length)
      return;
   if(ntohs(packet->length) < sizeof(PppPacket))
      return;

   //Save the length of the IPV6CP packet
   length = ntohs(packet->length);

   //Debug message
   TRACE_INFO("IPV6CP packet received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump IPV6CP packet contents for debugging purpose
   pppDumpPacket(packet, length, PPP_PROTOCOL_IPV6CP);

   //Check IPV6CP code field
   switch(packet->code)
   {
   //Configure-Request packet?
   case PPP_CODE_CONFIGURE_REQ:
      //Process Configure-Request packet
      ipv6cpProcessConfigureReq(context, (PppConfigurePacket *) packet);
      break;
   //Configure-Ack packet?
   case PPP_CODE_CONFIGURE_ACK:
      //Process Configure-Ack packet
      ipv6cpProcessConfigureAck(context, (PppConfigurePacket *) packet);
      break;
   //Configure-Nak packet?
   case PPP_CODE_CONFIGURE_NAK:
      //Process Configure-Nak packet
      ipv6cpProcessConfigureNak(context, (PppConfigurePacket *) packet);
      break;
   //Configure-Reject packet?
   case PPP_CODE_CONFIGURE_REJ:
      //Process Configure-Reject packet
      ipv6cpProcessConfigureReject(context, (PppConfigurePacket *) packet);
      break;
   //Terminate-Request packet?
   case PPP_CODE_TERMINATE_REQ:
      //Process Terminate-Request packet
      ipv6cpProcessTerminateReq(context, (PppTerminatePacket *) packet);
      break;
   //Terminate-Ack packet?
   case PPP_CODE_TERMINATE_ACK:
      //Process Terminate-Ack packet
      ipv6cpProcessTerminateAck(context, (PppTerminatePacket *) packet);
      break;
   //Code-Reject packet?
   case PPP_CODE_CODE_REJ:
      //Process Code-Reject packet
      ipv6cpProcessCodeRej(context, (PppCodeRejPacket *) packet);
      break;
   //Unknown code field
   default:
      //The packet is un-interpretable
      ipv6cpProcessUnknownCode(context, packet);
      break;
   }
}


/**
 * @brief Process Configure-Request packet
 * @param[in] context PPP context
 * @param[in] configureReqPacket Packet received from the peer
 * @return Error code
 **/

error_t ipv6cpProcessConfigureReq(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   error_t error;
   size_t length;
   bool_t notRecognizable;
   bool_t notAcceptable;
   PppOption *option;

   //Debug message
   TRACE_INFO("\r\nIPV6CP Receive-Configure-Request event\r\n");

   //Initialize variables
   error = NO_ERROR;
   notRecognizable = FALSE;
   notAcceptable = FALSE;

   //Retrieve the length of the option list
   length = ntohs(configureReqPacket->length) - sizeof(PppConfigurePacket);
   //Point to the first option
   option = (PppOption *) configureReqPacket->options;

   //Parse configuration options
   while(length > 0)
   {
      //Parse current option
      error = ipv6cpParseOption(context, option, length, NULL);

      //Any error to report?
      if(error == ERROR_INVALID_TYPE)
      {
         //Option not recognizable
         notRecognizable = TRUE;
         //Catch error
         error = NO_ERROR;
      }
      else if(error == ERROR_INVALID_VALUE)
      {
         //Option not acceptable for configuration
         notAcceptable = TRUE;
         //Catch error
         error = NO_ERROR;
      }
      else if(error)
      {
         //Malformed Configure-Request packet
         break;
      }

      //Remaining bytes to process
      length -= option->length;
      //Jump to the next option
      option = (PppOption *) ((uint8_t *) option + option->length);
   }

   //Valid Configure-Request packet received from the peer?
   if(!error)
   {
      //Check flags
      if(notRecognizable)
      {
         //If some configuration options received in the Configure-Request are not
         //recognizable or not acceptable for negotiation, then the implementation
         //must transmit a Configure-Reject
         pppRcvConfigureReqEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks,
            configureReqPacket, PPP_CODE_CONFIGURE_REJ);
      }
      else if(notAcceptable)
      {
         //If all configuration options are recognizable, but some values are not
         //acceptable, then the implementation must transmit a Configure-Nak
         pppRcvConfigureReqEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks,
            configureReqPacket, PPP_CODE_CONFIGURE_NAK);
      }
      else
      {
         //If every configuration option received in the Configure-Request is
         //recognizable and all values are acceptable, then the implementation
         //must transmit a Configure-Ack
         pppRcvConfigureReqEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks,
            configureReqPacket, PPP_CODE_CONFIGURE_ACK);
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Process Configure-Ack packet
 * @param[in] context PPP context
 * @param[in] configureAckPacket Packet received from the peer
 * @return Error code
 **/

error_t ipv6cpProcessConfigureAck(PppContext *context,
   const PppConfigurePacket *configureAckPacket)
{
   //Debug message
   TRACE_INFO("\r\nIPV6CP Receive-Configure-Ack event\r\n");

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(configureAckPacket->identifier != context->ipv6cpFsm.identifier)
      return ERROR_WRONG_IDENTIFIER;

   //A valid Configure-Ack packet has been received from the peer
   pppRcvConfigureAckEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Configure-Nak packet
 * @param[in] context PPP context
 * @param[in] configureNakPacket Packet received from the peer
 * @return Error code
 **/

error_t ipv6cpProcessConfigureNak(PppContext *context,
   const PppConfigurePacket *configureNakPacket)
{
   size_t length;
   PppOption *option;

   //Debug message
   TRACE_INFO("IPV6CP Receive-Configure-Nak event\r\n");

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(configureNakPacket->identifier != context->ipv6cpFsm.identifier)
      return ERROR_WRONG_IDENTIFIER;

   //Retrieve the length of the option list
   length = ntohs(configureNakPacket->length) - sizeof(PppConfigurePacket);
   //Point to the first option
   option = (PppOption *) configureNakPacket->options;

   //Parse configuration options
   while(length > 0)
   {
      //Check option length
      if(option->length < sizeof(PppOption))
         return ERROR_INVALID_LENGTH;
      if(option->length > length)
         return ERROR_INVALID_LENGTH;

      //Interface-Identifier option?
      if(option->type == IPV6CP_OPTION_INTERFACE_ID)
      {
         //Cast option
         Ipv6cpInterfaceIdOption *interfaceIdOption = (Ipv6cpInterfaceIdOption *) option;

         //Check option length
         if(interfaceIdOption->length != sizeof(Ipv6cpInterfaceIdOption))
            return ERROR_INVALID_LENGTH;

         //Save interface identifier
         context->localConfig.interfaceId = interfaceIdOption->interfaceId;
      }

      //Remaining bytes to process
      length -= option->length;
      //Jump to the next option
      option = (PppOption *) ((uint8_t *) option + option->length);
   }

   //A valid Configure-Nak or Configure-Reject packet has been received from the peer
   pppRcvConfigureNakEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Configure-Reject packet
 * @param[in] context PPP context
 * @param[in] configureRejPacket Packet received from the peer
 * @return Error code
 **/

error_t ipv6cpProcessConfigureReject(PppContext *context,
   const PppConfigurePacket *configureRejPacket)
{
   size_t length;
   PppOption *option;

   //Debug message
   TRACE_INFO("\r\nIPV6CP Receive-Configure-Reject event\r\n");

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(configureRejPacket->identifier != context->ipv6cpFsm.identifier)
      return ERROR_WRONG_IDENTIFIER;

   //Retrieve the length of the option list
   length = ntohs(configureRejPacket->length) - sizeof(PppConfigurePacket);
   //Point to the first option
   option = (PppOption *) configureRejPacket->options;

   //Parse configuration options
   while(length > 0)
   {
      //Check option length
      if(option->length < sizeof(PppOption))
         return ERROR_INVALID_LENGTH;
      if(option->length > length)
         return ERROR_INVALID_LENGTH;

      //Interface-Identifier option?
      if(option->type == IPV6CP_OPTION_INTERFACE_ID)
      {
         //The option is not recognized by the peer
         context->localConfig.interfaceIdRejected = TRUE;
      }

      //Remaining bytes to process
      length -= option->length;
      //Jump to the next option
      option = (PppOption *) ((uint8_t *) option + option->length);
   }

   //A valid Configure-Nak or Configure-Reject packet has been received from the peer
   pppRcvConfigureNakEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Terminate-Request packet
 * @param[in] context PPP context
 * @param[in] terminateReqPacket Packet received from the peer
 * @return Error code
 **/

error_t ipv6cpProcessTerminateReq(PppContext *context,
   const PppTerminatePacket *terminateReqPacket)
{
   //Debug message
   TRACE_INFO("\r\nIPV6CP Receive-Terminate-Request event\r\n");

   //The Terminate-Request indicates the desire of the peer to close the connection
   pppRcvTerminateReqEvent(context, &context->ipv6cpFsm,
      &ipv6cpCallbacks, terminateReqPacket);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Terminate-Ack packet
 * @param[in] context PPP context
 * @param[in] terminateAckPacket Packet received from the peer
 * @return Error code
 **/

error_t ipv6cpProcessTerminateAck(PppContext *context,
   const PppTerminatePacket *terminateAckPacket)
{
   //Debug message
   TRACE_INFO("\r\nIPV6CP Receive-Terminate-Ack event\r\n");

   //The Terminate-Ack packet is usually a response to a Terminate-Request
   //packet. This packet may also indicate that the peer is in Closed or
   //Stopped states, and serves to re-synchronize the link configuration
   pppRcvTerminateAckEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Code-Reject packet
 * @param[in] context PPP context
 * @param[in] codeRejPacket Packet received from the peer
 * @return Error code
 **/

error_t ipv6cpProcessCodeRej(PppContext *context,
   const PppCodeRejPacket *codeRejPacket)
{
   size_t length;
   PppPacket *packet;

   //Debug message
   TRACE_INFO("\r\nIPV6CP Receive-Code-Reject event\r\n");

   //Point to the rejected packet
   packet = (PppPacket *) codeRejPacket->rejectedPacket;
   //Retrieve the length of the rejected packet
   length = ntohs(codeRejPacket->length) - sizeof(PppCodeRejPacket);

   //Make sure the length of the rejected packet is valid
   if(length < sizeof(PppPacket))
      return ERROR_INVALID_LENGTH;

   //Check whether the rejected value is acceptable or catastrophic
   if(packet->code < PPP_CODE_CONFIGURE_REQ ||
      packet->code > PPP_CODE_CODE_REJ)
   {
      //The RXJ+ event arises when the rejected value is acceptable, such
      //as a Code-Reject of an extended code, or a Protocol-Reject of a
      //NCP. These are within the scope of normal operation
      pppRcvCodeRejEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks, TRUE);
   }
   else
   {
      //The RXJ- event arises when the rejected value is catastrophic, such
      //as a Code-Reject of Configure-Request! This event communicates an
      //unrecoverable error that terminates the connection
      pppRcvCodeRejEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks, FALSE);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process packet with unknown code
 * @param[in] context PPP context
 * @param[in] packet Un-interpretable packet received from the peer
 * @return Error code
 **/

error_t ipv6cpProcessUnknownCode(PppContext *context,
   const PppPacket *packet)
{
   //Debug message
   TRACE_INFO("\r\nIPV6CP Receive-Unknown-Code event\r\n");

   //This event occurs when an un-interpretable packet is received from
   //the peer. A Code-Reject packet is sent in response
   pppRcvUnknownCodeEvent(context, &context->ipv6cpFsm, &ipv6cpCallbacks, packet);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief This-Layer-Up callback function
 * @param[in] context PPP context
 **/

void ipv6cpThisLayerUp(PppContext *context)
{
   NetInterface *interface;
   Ipv6Addr ipAddr;

   //Debug message
   TRACE_INFO("IPV6CP This-Layer-Up callback\r\n");

   //Debug message
   TRACE_INFO("  Local Interface Id = %s\r\n",
      eui64AddrToString(&context->localConfig.interfaceId, NULL));
   TRACE_INFO("  Peer Interface Id = %s\r\n",
      eui64AddrToString(&context->peerConfig.interfaceId, NULL));

   //Point to the underlying interface
   interface = context->interface;

   //Generate an IPv6 address from the local interface identifier
   ipv6GenerateLinkLocalAddr(&context->localConfig.interfaceId, &ipAddr);

   //Update IPv6 configuration
   ipv6SetAddr(interface, 0, &ipAddr, IPV6_ADDR_STATE_PREFERRED,
      NDP_INFINITE_LIFETIME, NDP_INFINITE_LIFETIME, TRUE);

   //Generate an IPv6 address from the remote interface identifier
   ipv6GenerateLinkLocalAddr(&context->peerConfig.interfaceId, &ipAddr);

   //Update IPv6 configuration
   interface->ipv6Context.routerList[0].addr = ipAddr;
   interface->ipv6Context.routerList[0].permanent = TRUE;

   //Link is up
   interface->linkState = TRUE;

   //Disable interrupts
   interface->nicDriver->disableIrq(interface);
   //Process link state change event
   nicNotifyLinkChange(interface);
   //Re-enable interrupts
   interface->nicDriver->enableIrq(interface);
}


/**
 * @brief This-Layer-Down callback function
 * @param[in] context PPP context
 **/

void ipv6cpThisLayerDown(PppContext *context)
{
   NetInterface *interface;

   //Debug message
   TRACE_INFO("IPV6CP This-Layer-Down callback\r\n");

   //Point to the underlying interface
   interface = context->interface;

   //Link is up
   interface->linkState = FALSE;

   //Disable interrupts
   interface->nicDriver->disableIrq(interface);
   //Process link state change event
   nicNotifyLinkChange(interface);
   //Re-enable interrupts
   interface->nicDriver->enableIrq(interface);
}


/**
 * @brief This-Layer-Started callback function
 * @param[in] context PPP context
 **/

void ipv6cpThisLayerStarted(PppContext *context)
{
   //Debug message
   TRACE_INFO("IPV6CP This-Layer-Started callback\r\n");
}


/**
 * @brief This-Layer-Finished callback function
 * @param[in] context PPP context
 **/

void ipv6cpThisLayerFinished(PppContext *context)
{
   //Debug message
   TRACE_INFO("IPV6CP This-Layer-Finished callback\r\n");
}


/**
 * @brief Initialize-Restart-Count callback function
 * @param[in] context PPP context
 * @param[in] value Restart counter value
 **/

void ipv6cpInitRestartCount(PppContext *context, uint_t value)
{
   //Debug message
   TRACE_INFO("IPV6CP Initialize-Restart-Count callback\r\n");

   //Initialize restart counter
   context->ipv6cpFsm.restartCounter = value;
}


/**
 * @brief Zero-Restart-Count callback function
 * @param[in] context PPP context
 **/

void ipv6cpZeroRestartCount(PppContext *context)
{
   //Debug message
   TRACE_INFO("IPV6CP Zero-Restart-Count callback\r\n");

   //Zero restart counter
   context->ipv6cpFsm.restartCounter = 0;

   //The receiver of a Terminate-Request should wait for the peer to
   //disconnect, and must not disconnect until at least one Restart
   //time has passed after sending a Terminate-Ack
   context->ipv6cpFsm.timestamp = osGetSystemTime();
}


/**
 * @brief Send-Configure-Request callback function
 * @param[in] context PPP context
 * @return Error code
 **/

error_t ipv6cpSendConfigureReq(PppContext *context)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PppConfigurePacket *configureReqPacket;

   //Debug message
   TRACE_INFO("IPV6CP Send-Configure-Request callback\r\n");

   //Allocate a buffer memory to hold the Configure-Request packet
   buffer = pppAllocBuffer(PPP_MAX_CONF_REQ_SIZE, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Configure-Request packet
   configureReqPacket = netBufferAt(buffer, offset);

   //Format packet header
   configureReqPacket->code = PPP_CODE_CONFIGURE_REQ;
   configureReqPacket->identifier = ++context->ipv6cpFsm.identifier;
   configureReqPacket->length = sizeof(PppConfigurePacket);

   //Make sure the Interface-Identifier option has not been previously rejected
   if(!context->localConfig.interfaceIdRejected)
   {
      //Add option
      pppAddOption(configureReqPacket, IPV6CP_OPTION_INTERFACE_ID,
         &context->localConfig.interfaceId, sizeof(Eui64));
   }

   //Save packet length
   length = configureReqPacket->length;
   //Convert length field to network byte order
   configureReqPacket->length = htons(length);

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + length);

   //Debug message
   TRACE_INFO("Sending Configure-Request packet (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump packet contents for debugging purpose
   pppDumpPacket((PppPacket *) configureReqPacket, length, PPP_PROTOCOL_IPV6CP);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, PPP_PROTOCOL_IPV6CP);

   //The restart counter is decremented each time a Configure-Request is sent
   if(context->ipv6cpFsm.restartCounter > 0)
      context->ipv6cpFsm.restartCounter--;

   //Save the time at which the packet was sent
   context->ipv6cpFsm.timestamp = osGetSystemTime();

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send-Configure-Ack callback function
 * @param[in] context PPP context
 * @param[in] configureReqPacket Configure-Request packet received from the peer
 * @return Error code
 **/

error_t ipv6cpSendConfigureAck(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   //Debug message
   TRACE_INFO("IPV6CP Send-Configure-Ack callback\r\n");

   //Send Configure-Ack packet
   return pppSendConfigureAckNak(context, configureReqPacket,
      PPP_PROTOCOL_IPV6CP, PPP_CODE_CONFIGURE_ACK);
}


/**
 * @brief Send-Configure-Nak callback function
 * @param[in] context PPP context
 * @param[in] configureReqPacket Configure-Request packet received from the peer
 * @return Error code
 **/

error_t ipv6cpSendConfigureNak(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   //Debug message
   TRACE_INFO("IPV6CP Send-Configure-Nak callback\r\n");

   //Send Configure-Nak packet
   return pppSendConfigureAckNak(context, configureReqPacket,
      PPP_PROTOCOL_IPV6CP, PPP_CODE_CONFIGURE_NAK);
}


/**
 * @brief Send-Configure-Reject callback function
 * @param[in] context PPP context
 * @param[in] configureReqPacket Configure-Request packet received from the peer
 * @return Error code
 **/

error_t ipv6cpSendConfigureRej(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   //Debug message
   TRACE_INFO("IPV6CP Send-Configure-Reject callback\r\n");

   //Send Configure-Reject packet
   return pppSendConfigureAckNak(context, configureReqPacket,
      PPP_PROTOCOL_IPV6CP, PPP_CODE_CONFIGURE_REJ);
}


/**
 * @brief Send-Terminate-Request callback function
 * @param[in] context PPP context
 * @return Error code
 **/

error_t ipv6cpSendTerminateReq(PppContext *context)
{
   error_t error;

   //Debug message
   TRACE_INFO("IPV6CP Send-Terminate-Request callback\r\n");

   //On transmission, the Identifier field must be changed
   context->ipv6cpFsm.identifier++;

   //Send Terminate-Request packet
   error = pppSendTerminateReq(context, context->ipv6cpFsm.identifier, PPP_PROTOCOL_IPV6CP);

   //The restart counter is decremented each time a Terminate-Request is sent
   if(context->ipv6cpFsm.restartCounter > 0)
      context->ipv6cpFsm.restartCounter--;

   //Save the time at which the packet was sent
   context->ipv6cpFsm.timestamp = osGetSystemTime();

   //Return status code
   return error;
}


/**
 * @brief Send-Terminate-Ack callback function
 * @param[in] context PPP context
 * @param[in] terminateReqPacket Terminate-Request packet received from the peer
 * @return Error code
 **/

error_t ipv6cpSendTerminateAck(PppContext *context,
   const PppTerminatePacket *terminateReqPacket)
{
   uint8_t identifier;

   //Debug message
   TRACE_INFO("IPV6CP Send-Terminate-Ack callback\r\n");

   //Check whether this Terminate-Ack acknowledges the reception of a
   //Terminate-Request packet
   if(terminateReqPacket != NULL)
   {
      //The Identifier field of the Terminate-Request is copied into the
      //Identifier field of the Terminate-Ack packet
      identifier = terminateReqPacket->identifier;
   }
   else
   {
      //This Terminate-Ack packet serves to synchronize the automatons
      identifier = ++context->ipv6cpFsm.identifier;
   }

   //Send Terminate-Ack packet
   return pppSendTerminateAck(context, identifier, PPP_PROTOCOL_IPV6CP);
}


/**
 * @brief Send-Code-Reject callback function
 * @param[in] context PPP context
 * @param[in] packet Un-interpretable packet received from the peer
 * @return Error code
 **/

error_t ipv6cpSendCodeRej(PppContext *context, const PppPacket *packet)
{
   //Debug message
   TRACE_INFO("IPV6CP Send-Code-Reject callback\r\n");

   //The Identifier field must be changed for each Code-Reject sent
   context->ipv6cpFsm.identifier++;

   //Send Code-Reject packet
   return pppSendCodeRej(context, packet, context->ipv6cpFsm.identifier, PPP_PROTOCOL_IPV6CP);
}


/**
 * @brief Parse IPV6CP configuration option
 * @param[in] context PPP context
 * @param[in] option Option to be checked
 * @param[in] inPacketLen Remaining bytes to process in the incoming packet
 * @param[out] outPacket Pointer to the Configure-Ack, Nak or Reject packet
 * @return Error code
 **/

error_t ipv6cpParseOption(PppContext *context, PppOption *option,
   size_t inPacketLen, PppConfigurePacket *outPacket)
{
   error_t error;

   //Malformed IPV6CP packet?
   if(inPacketLen < sizeof(PppOption))
      return ERROR_INVALID_LENGTH;

   //Check option length
   if(option->length < sizeof(PppOption))
      return ERROR_INVALID_LENGTH;
   if(option->length > inPacketLen)
      return ERROR_INVALID_LENGTH;

   //Check option type
   switch(option->type)
   {
   case IPV6CP_OPTION_INTERFACE_ID:
      //Check Interface-Identifier option
      error = ipv6cpParseInterfaceIdOption(context, (Ipv6cpInterfaceIdOption *) option, outPacket);
      break;
   default:
      //If some configuration options received in the Configure-Request are not
      //recognizable or not acceptable for negotiation, then the implementation
      //must transmit a Configure-Reject
      if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_REJ)
      {
         //The options field of the Configure-Reject packet is filled
         //with the unrecognized options from the Configure-Request
         pppAddOption(outPacket, option->type, option->data,
            option->length - sizeof(PppOption));
      }

      //The option is not acceptable for negotiation
      error = ERROR_INVALID_TYPE;
      break;
   }

   //Return status code
   return error;
}


/**
 * @brief Parse Interface-Identifier option
 * @param[in] context PPP context
 * @param[in] option Option to be checked
 * @param[out] outPacket Pointer to the Configure-Nak or Configure-Reject packet
 * @return Error code
 **/

error_t ipv6cpParseInterfaceIdOption(PppContext *context,
   Ipv6cpInterfaceIdOption *option, PppConfigurePacket *outPacket)
{
   error_t error;

   //Check length field
   if(option->length == sizeof(Ipv6cpInterfaceIdOption))
   {
      //Check whether the option value is acceptable
      if(!eui64CompAddr(&option->interfaceId, &EUI64_UNSPECIFIED_ADDR))
      {
         //If every configuration option received in the Configure-Request is
         //recognizable and all values are acceptable, then the implementation
         //must transmit a Configure-Ack
         if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_ACK)
         {
            //Save interface identifier
            context->peerConfig.interfaceId = option->interfaceId;

            //The options field of the Configure-Ack packet contains the
            //configuration options that the sender is acknowledging
            pppAddOption(outPacket, IPV6CP_OPTION_INTERFACE_ID,
               &option->interfaceId, option->length - sizeof(PppOption));
         }

         //The value is acceptable
         error = NO_ERROR;
      }
      else
      {
         //If all configuration options are recognizable, but some values are not
         //acceptable, then the implementation must transmit a Configure-Nak
         if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_NAK)
         {
            //The option must be modified to a value acceptable to the
            //Configure-Nak sender
            pppAddOption(outPacket, IPV6CP_OPTION_INTERFACE_ID,
               &context->peerConfig.interfaceId, sizeof(Eui64));
         }

         //The value is not acceptable
         error = ERROR_INVALID_VALUE;
      }
   }
   else
   {
      //Invalid length field
      error = ERROR_INVALID_LENGTH;
   }

   //Return status code
   return error;
}

#endif
