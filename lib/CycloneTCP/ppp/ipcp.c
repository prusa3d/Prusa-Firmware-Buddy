/**
 * @file ipcp.c
 * @brief IPCP (PPP Internet Protocol Control Protocol)
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
#include "ipv4/ipv4.h"
#include "ppp/ppp_fsm.h"
#include "ppp/ppp_misc.h"
#include "ppp/ppp_debug.h"
#include "ppp/ipcp.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (PPP_SUPPORT == ENABLED && IPV4_SUPPORT == ENABLED)


/**
 * @brief IPCP FSM callbacks
 **/

const PppCallbacks ipcpCallbacks =
{
   ipcpThisLayerUp,
   ipcpThisLayerDown,
   ipcpThisLayerStarted,
   ipcpThisLayerFinished,
   ipcpInitRestartCount,
   ipcpZeroRestartCount,
   ipcpSendConfigureReq,
   ipcpSendConfigureAck,
   ipcpSendConfigureNak,
   ipcpSendConfigureRej,
   ipcpSendTerminateReq,
   ipcpSendTerminateAck,
   ipcpSendCodeRej,
   NULL
};


/**
 * @brief IPCP Open event
 * @param[in] context PPP context
 * @return Error code
 **/

error_t ipcpOpen(PppContext *context)
{
   //Debug message
   TRACE_INFO("\r\nIPCP Open event\r\n");

   //The link is administratively available for traffic
   pppOpenEvent(context, &context->ipcpFsm, &ipcpCallbacks);
   //The lower layer is ready to carry packets
   pppUpEvent(context, &context->ipcpFsm, &ipcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief IPCP Close event
 * @param[in] context PPP context
 * @return Error code
 **/

error_t ipcpClose(PppContext *context)
{
   //Debug message
   TRACE_INFO("\r\nIPCP Close event\r\n");

   //The lower layer is no longer ready to carry packets
   pppDownEvent(context, &context->ipcpFsm, &ipcpCallbacks);
   //The link is no longer available for traffic
   pppCloseEvent(context, &context->ipcpFsm, &ipcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief IPCP timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * manage retransmissions
 *
 * @param[in] context PPP context
 **/

void ipcpTick(PppContext *context)
{
   //Check whether the restart timer is running
   if(context->ipcpFsm.state >= PPP_STATE_4_CLOSING &&
      context->ipcpFsm.state <= PPP_STATE_8_ACK_SENT)
   {
      //Get current time
      systime_t time = osGetSystemTime();

      //Check restart timer
      if((time - context->ipcpFsm.timestamp) >= PPP_RESTART_TIMER)
      {
         //Debug message
         TRACE_INFO("\r\nIPCP Timeout event\r\n");

         //The restart timer is used to retransmit Configure-Request
         //and Terminate-Request packets
         pppTimeoutEvent(context, &context->ipcpFsm, &ipcpCallbacks);
      }
   }
}


/**
 * @brief Process an incoming IPCP packet
 * @param[in] context PPP context
 * @param[in]  packet IPCP packet received from the peer
 * @param[in] length Length of the packet, in bytes
 **/

void ipcpProcessPacket(PppContext *context, const PppPacket *packet, size_t length)
{
   //Ensure the length of the incoming IPCP packet is valid
   if(length < sizeof(PppPacket))
      return;

   //Check the length field
   if(ntohs(packet->length) > length)
      return;
   if(ntohs(packet->length) < sizeof(PppPacket))
      return;

   //Save the length of the IPCP packet
   length = ntohs(packet->length);

   //Debug message
   TRACE_INFO("IPCP packet received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump IPCP packet contents for debugging purpose
   pppDumpPacket(packet, length, PPP_PROTOCOL_IPCP);

   //Check IPCP code field
   switch(packet->code)
   {
   //Configure-Request packet?
   case PPP_CODE_CONFIGURE_REQ:
      //Process Configure-Request packet
      ipcpProcessConfigureReq(context, (PppConfigurePacket *) packet);
      break;
   //Configure-Ack packet?
   case PPP_CODE_CONFIGURE_ACK:
      //Process Configure-Ack packet
      ipcpProcessConfigureAck(context, (PppConfigurePacket *) packet);
      break;
   //Configure-Nak packet?
   case PPP_CODE_CONFIGURE_NAK:
      //Process Configure-Nak packet
      ipcpProcessConfigureNak(context, (PppConfigurePacket *) packet);
      break;
   //Configure-Reject packet?
   case PPP_CODE_CONFIGURE_REJ:
      //Process Configure-Reject packet
      ipcpProcessConfigureReject(context, (PppConfigurePacket *) packet);
      break;
   //Terminate-Request packet?
   case PPP_CODE_TERMINATE_REQ:
      //Process Terminate-Request packet
      ipcpProcessTerminateReq(context, (PppTerminatePacket *) packet);
      break;
   //Terminate-Ack packet?
   case PPP_CODE_TERMINATE_ACK:
      //Process Terminate-Ack packet
      ipcpProcessTerminateAck(context, (PppTerminatePacket *) packet);
      break;
   //Code-Reject packet?
   case PPP_CODE_CODE_REJ:
      //Process Code-Reject packet
      ipcpProcessCodeRej(context, (PppCodeRejPacket *) packet);
      break;
   //Unknown code field
   default:
      //The packet is un-interpretable
      ipcpProcessUnknownCode(context, packet);
      break;
   }
}


/**
 * @brief Process Configure-Request packet
 * @param[in] context PPP context
 * @param[in] configureReqPacket Packet received from the peer
 * @return Error code
 **/

error_t ipcpProcessConfigureReq(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   error_t error;
   size_t length;
   bool_t notRecognizable;
   bool_t notAcceptable;
   PppOption *option;

   //Debug message
   TRACE_INFO("\r\nIPCP Receive-Configure-Request event\r\n");

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
      error = ipcpParseOption(context, option, length, NULL);

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
         pppRcvConfigureReqEvent(context, &context->ipcpFsm, &ipcpCallbacks,
            configureReqPacket, PPP_CODE_CONFIGURE_REJ);
      }
      else if(notAcceptable)
      {
         //If all configuration options are recognizable, but some values are not
         //acceptable, then the implementation must transmit a Configure-Nak
         pppRcvConfigureReqEvent(context, &context->ipcpFsm, &ipcpCallbacks,
            configureReqPacket, PPP_CODE_CONFIGURE_NAK);
      }
      else
      {
         //If every configuration option received in the Configure-Request is
         //recognizable and all values are acceptable, then the implementation
         //must transmit a Configure-Ack
         pppRcvConfigureReqEvent(context, &context->ipcpFsm, &ipcpCallbacks,
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

error_t ipcpProcessConfigureAck(PppContext *context,
   const PppConfigurePacket *configureAckPacket)
{
   //Debug message
   TRACE_INFO("\r\nIPCP Receive-Configure-Ack event\r\n");

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(configureAckPacket->identifier != context->ipcpFsm.identifier)
      return ERROR_WRONG_IDENTIFIER;

   //A valid Configure-Ack packet has been received from the peer
   pppRcvConfigureAckEvent(context, &context->ipcpFsm, &ipcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Configure-Nak packet
 * @param[in] context PPP context
 * @param[in] configureNakPacket Packet received from the peer
 * @return Error code
 **/

error_t ipcpProcessConfigureNak(PppContext *context,
   const PppConfigurePacket *configureNakPacket)
{
   size_t length;
   PppOption *option;

   //Debug message
   TRACE_INFO("IPCP Receive-Configure-Nak event\r\n");

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(configureNakPacket->identifier != context->ipcpFsm.identifier)
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

      //IP-Address option?
      if(option->type == IPCP_OPTION_IP_ADDRESS)
      {
         //Cast option
         IpcpIpAddressOption *ipAddressOption = (IpcpIpAddressOption *) option;

         //Check option length
         if(ipAddressOption->length != sizeof(IpcpIpAddressOption))
            return ERROR_INVALID_LENGTH;

         //Save IP address
         context->localConfig.ipAddr = ipAddressOption->ipAddr;
      }
      //Primary-DNS-Server-Address option?
      else if(option->type == IPCP_OPTION_PRIMARY_DNS)
      {
         //Cast option
         IpcpPrimaryDnsOption *primaryDns = (IpcpPrimaryDnsOption *) option;

         //Check option length
         if(primaryDns->length != sizeof(IpcpPrimaryDnsOption))
            return ERROR_INVALID_LENGTH;

         //Save primary DNS server address
         context->localConfig.primaryDns = primaryDns->ipAddr;
      }
      //Secondary-DNS-Server-Address option?
      else if(option->type == IPCP_OPTION_SECONDARY_DNS)
      {
         //Cast option
         IpcpSecondaryDnsOption *secondaryDns = (IpcpSecondaryDnsOption *) option;

         //Check option length
         if(secondaryDns->length != sizeof(IpcpSecondaryDnsOption))
            return ERROR_INVALID_LENGTH;

         //Save secondary DNS server address
         context->localConfig.secondaryDns = secondaryDns->ipAddr;
      }

      //Remaining bytes to process
      length -= option->length;
      //Jump to the next option
      option = (PppOption *) ((uint8_t *) option + option->length);
   }

   //A valid Configure-Nak or Configure-Reject packet has been received from the peer
   pppRcvConfigureNakEvent(context, &context->ipcpFsm, &ipcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Configure-Reject packet
 * @param[in] context PPP context
 * @param[in] configureRejPacket Packet received from the peer
 * @return Error code
 **/

error_t ipcpProcessConfigureReject(PppContext *context,
   const PppConfigurePacket *configureRejPacket)
{
   size_t length;
   PppOption *option;

   //Debug message
   TRACE_INFO("\r\nIPCP Receive-Configure-Reject event\r\n");

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(configureRejPacket->identifier != context->ipcpFsm.identifier)
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

      //IP-Address option?
      if(option->type == IPCP_OPTION_IP_ADDRESS)
      {
         //The option is not recognized by the peer
         context->localConfig.ipAddrRejected = TRUE;
      }
      //Primary-DNS-Server-Address option?
      else if(option->type == IPCP_OPTION_PRIMARY_DNS)
      {
         //The option is not recognized by the peer
         context->localConfig.primaryDnsRejected = TRUE;
      }
      //Secondary-DNS-Server-Address option?
      else if(option->type == IPCP_OPTION_SECONDARY_DNS)
      {
         //The option is not recognized by the peer
         context->localConfig.secondaryDnsRejected = TRUE;
      }

      //Remaining bytes to process
      length -= option->length;
      //Jump to the next option
      option = (PppOption *) ((uint8_t *) option + option->length);
   }

   //A valid Configure-Nak or Configure-Reject packet has been received from the peer
   pppRcvConfigureNakEvent(context, &context->ipcpFsm, &ipcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Terminate-Request packet
 * @param[in] context PPP context
 * @param[in] terminateReqPacket Packet received from the peer
 * @return Error code
 **/

error_t ipcpProcessTerminateReq(PppContext *context,
   const PppTerminatePacket *terminateReqPacket)
{
   //Debug message
   TRACE_INFO("\r\nIPCP Receive-Terminate-Request event\r\n");

   //The Terminate-Request indicates the desire of the peer to close the connection
   pppRcvTerminateReqEvent(context, &context->ipcpFsm,
      &ipcpCallbacks, terminateReqPacket);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Terminate-Ack packet
 * @param[in] context PPP context
 * @param[in] terminateAckPacket Packet received from the peer
 * @return Error code
 **/

error_t ipcpProcessTerminateAck(PppContext *context,
   const PppTerminatePacket *terminateAckPacket)
{
   //Debug message
   TRACE_INFO("\r\nIPCP Receive-Terminate-Ack event\r\n");

   //The Terminate-Ack packet is usually a response to a Terminate-Request
   //packet. This packet may also indicate that the peer is in Closed or
   //Stopped states, and serves to re-synchronize the link configuration
   pppRcvTerminateAckEvent(context, &context->ipcpFsm, &ipcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Code-Reject packet
 * @param[in] context PPP context
 * @param[in] codeRejPacket Packet received from the peer
 * @return Error code
 **/

error_t ipcpProcessCodeRej(PppContext *context,
   const PppCodeRejPacket *codeRejPacket)
{
   size_t length;
   PppPacket *packet;

   //Debug message
   TRACE_INFO("\r\nIPCP Receive-Code-Reject event\r\n");

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
      pppRcvCodeRejEvent(context, &context->ipcpFsm, &ipcpCallbacks, TRUE);
   }
   else
   {
      //The RXJ- event arises when the rejected value is catastrophic, such
      //as a Code-Reject of Configure-Request! This event communicates an
      //unrecoverable error that terminates the connection
      pppRcvCodeRejEvent(context, &context->ipcpFsm, &ipcpCallbacks, FALSE);
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

error_t ipcpProcessUnknownCode(PppContext *context,
   const PppPacket *packet)
{
   //Debug message
   TRACE_INFO("\r\nIPCP Receive-Unknown-Code event\r\n");

   //This event occurs when an un-interpretable packet is received from
   //the peer. A Code-Reject packet is sent in response
   pppRcvUnknownCodeEvent(context, &context->ipcpFsm, &ipcpCallbacks, packet);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief This-Layer-Up callback function
 * @param[in] context PPP context
 **/

void ipcpThisLayerUp(PppContext *context)
{
   NetInterface *interface;

   //Debug message
   TRACE_INFO("IPCP This-Layer-Up callback\r\n");

   //Debug message
   TRACE_INFO("  Local IP Addr = %s\r\n", ipv4AddrToString(context->localConfig.ipAddr, NULL));
   TRACE_INFO("  Peer IP Addr = %s\r\n", ipv4AddrToString(context->peerConfig.ipAddr, NULL));
   TRACE_INFO("  Primary DNS = %s\r\n", ipv4AddrToString(context->localConfig.primaryDns, NULL));
   TRACE_INFO("  Secondary DNS = %s\r\n", ipv4AddrToString(context->localConfig.secondaryDns, NULL));

   //Point to the underlying interface
   interface = context->interface;

   //Update IPv4 configuration
   interface->ipv4Context.addrList[0].addr = context->localConfig.ipAddr;
   interface->ipv4Context.addrList[0].state = IPV4_ADDR_STATE_VALID;
   interface->ipv4Context.addrList[0].defaultGateway = context->peerConfig.ipAddr;

   //Update the list of DNS servers
   interface->ipv4Context.dnsServerList[0] = context->localConfig.primaryDns;
#if (IPV4_DNS_SERVER_LIST_SIZE >= 2)
   interface->ipv4Context.dnsServerList[1] = context->localConfig.secondaryDns;
#endif

   //All the outgoing traffic will be routed to the other end of the link
   interface->ipv4Context.addrList[0].subnetMask = IPCP_DEFAULT_SUBNET_MASK;

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

void ipcpThisLayerDown(PppContext *context)
{
   NetInterface *interface;

   //Debug message
   TRACE_INFO("IPCP This-Layer-Down callback\r\n");

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

void ipcpThisLayerStarted(PppContext *context)
{
   //Debug message
   TRACE_INFO("IPCP This-Layer-Started callback\r\n");
}


/**
 * @brief This-Layer-Finished callback function
 * @param[in] context PPP context
 **/

void ipcpThisLayerFinished(PppContext *context)
{
   //Debug message
   TRACE_INFO("IPCP This-Layer-Finished callback\r\n");
}


/**
 * @brief Initialize-Restart-Count callback function
 * @param[in] context PPP context
 * @param[in] value Restart counter value
 **/

void ipcpInitRestartCount(PppContext *context, uint_t value)
{
   //Debug message
   TRACE_INFO("IPCP Initialize-Restart-Count callback\r\n");

   //Initialize restart counter
   context->ipcpFsm.restartCounter = value;
}


/**
 * @brief Zero-Restart-Count callback function
 * @param[in] context PPP context
 **/

void ipcpZeroRestartCount(PppContext *context)
{
   //Debug message
   TRACE_INFO("IPCP Zero-Restart-Count callback\r\n");

   //Zero restart counter
   context->ipcpFsm.restartCounter = 0;

   //The receiver of a Terminate-Request should wait for the peer to
   //disconnect, and must not disconnect until at least one Restart
   //time has passed after sending a Terminate-Ack
   context->ipcpFsm.timestamp = osGetSystemTime();
}


/**
 * @brief Send-Configure-Request callback function
 * @param[in] context PPP context
 * @return Error code
 **/

error_t ipcpSendConfigureReq(PppContext *context)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PppConfigurePacket *configureReqPacket;

   //Debug message
   TRACE_INFO("IPCP Send-Configure-Request callback\r\n");

   //Allocate a buffer memory to hold the Configure-Request packet
   buffer = pppAllocBuffer(PPP_MAX_CONF_REQ_SIZE, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Configure-Request packet
   configureReqPacket = netBufferAt(buffer, offset);

   //Format packet header
   configureReqPacket->code = PPP_CODE_CONFIGURE_REQ;
   configureReqPacket->identifier = ++context->ipcpFsm.identifier;
   configureReqPacket->length = sizeof(PppConfigurePacket);

   //Make sure the IP-Address option has not been previously rejected
   if(!context->localConfig.ipAddrRejected)
   {
      //Add option
      pppAddOption(configureReqPacket, IPCP_OPTION_IP_ADDRESS,
         &context->localConfig.ipAddr, sizeof(Ipv4Addr));
   }

   //Make sure the Primary-DNS-Server-Address option has not been
   //previously rejected
   if(!context->localConfig.primaryDnsRejected)
   {
      //Add option
      pppAddOption(configureReqPacket, IPCP_OPTION_PRIMARY_DNS,
         &context->localConfig.primaryDns, sizeof(Ipv4Addr));
   }

   //Make sure the Secondary-DNS-Server-Address option has not been
   //previously rejected
   if(!context->localConfig.secondaryDnsRejected)
   {
      //Add option
      pppAddOption(configureReqPacket, IPCP_OPTION_SECONDARY_DNS,
         &context->localConfig.secondaryDns, sizeof(Ipv4Addr));
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
   pppDumpPacket((PppPacket *) configureReqPacket, length, PPP_PROTOCOL_IPCP);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, PPP_PROTOCOL_IPCP);

   //The restart counter is decremented each time a Configure-Request is sent
   if(context->ipcpFsm.restartCounter > 0)
      context->ipcpFsm.restartCounter--;

   //Save the time at which the packet was sent
   context->ipcpFsm.timestamp = osGetSystemTime();

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

error_t ipcpSendConfigureAck(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   //Debug message
   TRACE_INFO("IPCP Send-Configure-Ack callback\r\n");

   //Send Configure-Ack packet
   return pppSendConfigureAckNak(context, configureReqPacket,
      PPP_PROTOCOL_IPCP, PPP_CODE_CONFIGURE_ACK);
}


/**
 * @brief Send-Configure-Nak callback function
 * @param[in] context PPP context
 * @param[in] configureReqPacket Configure-Request packet received from the peer
 * @return Error code
 **/

error_t ipcpSendConfigureNak(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   //Debug message
   TRACE_INFO("IPCP Send-Configure-Nak callback\r\n");

   //Send Configure-Nak packet
   return pppSendConfigureAckNak(context, configureReqPacket,
      PPP_PROTOCOL_IPCP, PPP_CODE_CONFIGURE_NAK);
}


/**
 * @brief Send-Configure-Reject callback function
 * @param[in] context PPP context
 * @param[in] configureReqPacket Configure-Request packet received from the peer
 * @return Error code
 **/

error_t ipcpSendConfigureRej(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   //Debug message
   TRACE_INFO("IPCP Send-Configure-Reject callback\r\n");

   //Send Configure-Reject packet
   return pppSendConfigureAckNak(context, configureReqPacket,
      PPP_PROTOCOL_IPCP, PPP_CODE_CONFIGURE_REJ);
}


/**
 * @brief Send-Terminate-Request callback function
 * @param[in] context PPP context
 * @return Error code
 **/

error_t ipcpSendTerminateReq(PppContext *context)
{
   error_t error;

   //Debug message
   TRACE_INFO("IPCP Send-Terminate-Request callback\r\n");

   //On transmission, the Identifier field must be changed
   context->ipcpFsm.identifier++;

   //Send Terminate-Request packet
   error = pppSendTerminateReq(context, context->ipcpFsm.identifier, PPP_PROTOCOL_IPCP);

   //The restart counter is decremented each time a Terminate-Request is sent
   if(context->ipcpFsm.restartCounter > 0)
      context->ipcpFsm.restartCounter--;

   //Save the time at which the packet was sent
   context->ipcpFsm.timestamp = osGetSystemTime();

   //Return status code
   return error;
}


/**
 * @brief Send-Terminate-Ack callback function
 * @param[in] context PPP context
 * @param[in] terminateReqPacket Terminate-Request packet received from the peer
 * @return Error code
 **/

error_t ipcpSendTerminateAck(PppContext *context,
   const PppTerminatePacket *terminateReqPacket)
{
   uint8_t identifier;

   //Debug message
   TRACE_INFO("IPCP Send-Terminate-Ack callback\r\n");

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
      identifier = ++context->ipcpFsm.identifier;
   }

   //Send Terminate-Ack packet
   return pppSendTerminateAck(context, identifier, PPP_PROTOCOL_IPCP);
}


/**
 * @brief Send-Code-Reject callback function
 * @param[in] context PPP context
 * @param[in] packet Un-interpretable packet received from the peer
 * @return Error code
 **/

error_t ipcpSendCodeRej(PppContext *context, const PppPacket *packet)
{
   //Debug message
   TRACE_INFO("IPCP Send-Code-Reject callback\r\n");

   //The Identifier field must be changed for each Code-Reject sent
   context->ipcpFsm.identifier++;

   //Send Code-Reject packet
   return pppSendCodeRej(context, packet, context->ipcpFsm.identifier, PPP_PROTOCOL_IPCP);
}


/**
 * @brief Parse IPCP configuration option
 * @param[in] context PPP context
 * @param[in] option Option to be checked
 * @param[in] inPacketLen Remaining bytes to process in the incoming packet
 * @param[out] outPacket Pointer to the Configure-Ack, Nak or Reject packet
 * @return Error code
 **/

error_t ipcpParseOption(PppContext *context, PppOption *option,
   size_t inPacketLen, PppConfigurePacket *outPacket)
{
   error_t error;

   //Malformed IPCP packet?
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
   case IPCP_OPTION_IP_ADDRESS:
      //Check IP-Address option
      error = ipcpParseIpAddressOption(context, (IpcpIpAddressOption *) option, outPacket);
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
 * @brief Parse IP-Address option
 * @param[in] context PPP context
 * @param[in] option Option to be checked
 * @param[out] outPacket Pointer to the Configure-Nak or Configure-Reject packet
 * @return Error code
 **/

error_t ipcpParseIpAddressOption(PppContext *context,
   IpcpIpAddressOption *option, PppConfigurePacket *outPacket)
{
   error_t error;

   //Check length field
   if(option->length == sizeof(IpcpIpAddressOption))
   {
      //Check whether the option value is acceptable
      if(option->ipAddr != IPV4_UNSPECIFIED_ADDR)
      {
         //If every configuration option received in the Configure-Request is
         //recognizable and all values are acceptable, then the implementation
         //must transmit a Configure-Ack
         if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_ACK)
         {
            //Save IP address
            context->peerConfig.ipAddr = option->ipAddr;

            //The options field of the Configure-Ack packet contains the
            //configuration options that the sender is acknowledging
            pppAddOption(outPacket, IPCP_OPTION_IP_ADDRESS,
               (void *) &option->ipAddr, option->length - sizeof(PppOption));
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
            pppAddOption(outPacket, IPCP_OPTION_IP_ADDRESS,
               &context->peerConfig.ipAddr, sizeof(Ipv4Addr));
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
