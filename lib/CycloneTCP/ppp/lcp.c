/**
 * @file lcp.c
 * @brief LCP (PPP Link Control Protocol)
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
#include "ppp/ppp_fsm.h"
#include "ppp/ppp_misc.h"
#include "ppp/ppp_debug.h"
#include "ppp/lcp.h"
#include "ppp/ipcp.h"
#include "ppp/ipv6cp.h"
#include "ppp/pap.h"
#include "ppp/chap.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (PPP_SUPPORT == ENABLED)


/**
 * @brief LCP FSM callbacks
 **/

const PppCallbacks lcpCallbacks =
{
   lcpThisLayerUp,
   lcpThisLayerDown,
   lcpThisLayerStarted,
   lcpThisLayerFinished,
   lcpInitRestartCount,
   lcpZeroRestartCount,
   lcpSendConfigureReq,
   lcpSendConfigureAck,
   lcpSendConfigureNak,
   lcpSendConfigureRej,
   lcpSendTerminateReq,
   lcpSendTerminateAck,
   lcpSendCodeRej,
   lcpSendEchoRep
};


/**
 * @brief LCP Open event
 * @param[in] context PPP context
 * @return Error code
 **/

error_t lcpOpen(PppContext *context)
{
   //Debug message
   TRACE_INFO("\r\nLCP Open event\r\n");

   //Advance to the Establish phase
   context->pppPhase = PPP_PHASE_ESTABLISH;

   //The link is administratively available for traffic
   pppOpenEvent(context, &context->lcpFsm, &lcpCallbacks);
   //The lower layer is ready to carry packets
   pppUpEvent(context, &context->lcpFsm, &lcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief LCP Close event
 * @param[in] context PPP context
 * @return Error code
 **/

error_t lcpClose(PppContext *context)
{
   //Debug message
   TRACE_INFO("\r\nLCP Close event\r\n");

   //The link is no longer available for traffic
   pppCloseEvent(context, &context->lcpFsm, &lcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief LCP timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * manage retransmissions
 *
 * @param[in] context PPP context
 **/

void lcpTick(PppContext *context)
{
   //Check whether the restart timer is running
   if(context->lcpFsm.state >= PPP_STATE_4_CLOSING &&
      context->lcpFsm.state <= PPP_STATE_8_ACK_SENT)
   {
      //Get current time
      systime_t time = osGetSystemTime();

      //Check restart timer
      if((time - context->lcpFsm.timestamp) >= PPP_RESTART_TIMER)
      {
         //Debug message
         TRACE_INFO("\r\nLCP Timeout event\r\n");

         //The restart timer is used to retransmit Configure-Request
         //and Terminate-Request packets
         pppTimeoutEvent(context, &context->lcpFsm, &lcpCallbacks);
      }
   }
}


/**
 * @brief Process an incoming LCP packet
 * @param[in] context PPP context
 * @param[in] packet LCP packet received from the peer
 * @param[in] length Length of the packet, in bytes
 **/

void lcpProcessPacket(PppContext *context, const PppPacket *packet, size_t length)
{
   //Ensure the length of the incoming LCP packet is valid
   if(length < sizeof(PppPacket))
      return;

   //Check the length field
   if(ntohs(packet->length) > length)
      return;
   if(ntohs(packet->length) < sizeof(PppPacket))
      return;

   //Save the length of the LCP packet
   length = ntohs(packet->length);

   //Debug message
   TRACE_INFO("LCP packet received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump LCP packet contents for debugging purpose
   pppDumpPacket(packet, length, PPP_PROTOCOL_LCP);

   //Check LCP code field
   switch(packet->code)
   {
   //Configure-Request packet?
   case PPP_CODE_CONFIGURE_REQ:
      //Process Configure-Request packet
      lcpProcessConfigureReq(context, (PppConfigurePacket *) packet);
      break;
   //Configure-Ack packet?
   case PPP_CODE_CONFIGURE_ACK:
      //Process Configure-Ack packet
      lcpProcessConfigureAck(context, (PppConfigurePacket *) packet);
      break;
   //Configure-Nak packet?
   case PPP_CODE_CONFIGURE_NAK:
      //Process Configure-Nak packet
      lcpProcessConfigureNak(context, (PppConfigurePacket *) packet);
      break;
   //Configure-Reject packet?
   case PPP_CODE_CONFIGURE_REJ:
      //Process Configure-Reject packet
      lcpProcessConfigureReject(context, (PppConfigurePacket *) packet);
      break;
   //Terminate-Request packet?
   case PPP_CODE_TERMINATE_REQ:
      //Process Terminate-Request packet
      lcpProcessTerminateReq(context, (PppTerminatePacket *) packet);
      break;
   //Terminate-Ack packet?
   case PPP_CODE_TERMINATE_ACK:
      //Process Terminate-Ack packet
      lcpProcessTerminateAck(context, (PppTerminatePacket *) packet);
      break;
   //Code-Reject packet?
   case PPP_CODE_CODE_REJ:
      //Process Code-Reject packet
      lcpProcessCodeRej(context, (PppCodeRejPacket *) packet);
      break;
   //Protocol-Reject packet?
   case PPP_CODE_PROTOCOL_REJ:
      //Process Protocol-Reject packet
      lcpProcessProtocolRej(context, (PppProtocolRejPacket *) packet);
      break;
   //Echo-Request packet?
   case PPP_CODE_ECHO_REQ:
      //Process Echo-Request packet
      lcpProcessEchoReq(context, (PppEchoPacket *) packet);
      break;
   //Echo-Reply packet?
   case PPP_CODE_ECHO_REP:
      //Process Echo-Reply packet
      lcpProcessEchoRep(context, (PppEchoPacket *) packet);
      break;
   //Discard-Request packet?
   case PPP_CODE_DISCARD_REQ:
      //Process Discard-Request packet
      lcpProcessDiscardReq(context, (PppDiscardReqPacket *) packet);
      break;
   //Unknown code field
   default:
      //The packet is un-interpretable
      lcpProcessUnknownCode(context, packet);
      break;
   }
}


/**
 * @brief Process Configure-Request packet
 * @param[in] context PPP context
 * @param[in] configureReqPacket Packet received from the peer
 * @return Error code
 **/

error_t lcpProcessConfigureReq(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   error_t error;
   size_t length;
   bool_t notRecognizable;
   bool_t notAcceptable;
   PppOption *option;

   //Debug message
   TRACE_INFO("\r\nLCP Receive-Configure-Request event\r\n");

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
      error = lcpParseOption(context, option, length, NULL);

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
         pppRcvConfigureReqEvent(context, &context->lcpFsm, &lcpCallbacks,
            configureReqPacket, PPP_CODE_CONFIGURE_REJ);
      }
      else if(notAcceptable)
      {
         //If all configuration options are recognizable, but some values are not
         //acceptable, then the implementation must transmit a Configure-Nak
         pppRcvConfigureReqEvent(context, &context->lcpFsm, &lcpCallbacks,
            configureReqPacket, PPP_CODE_CONFIGURE_NAK);
      }
      else
      {
         //If every configuration option received in the Configure-Request is
         //recognizable and all values are acceptable, then the implementation
         //must transmit a Configure-Ack
         pppRcvConfigureReqEvent(context, &context->lcpFsm, &lcpCallbacks,
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

error_t lcpProcessConfigureAck(PppContext *context,
   const PppConfigurePacket *configureAckPacket)
{
   //Debug message
   TRACE_INFO("\r\nLCP Receive-Configure-Ack event\r\n");

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(configureAckPacket->identifier != context->lcpFsm.identifier)
      return ERROR_WRONG_IDENTIFIER;

   //A valid Configure-Ack packet has been received from the peer
   pppRcvConfigureAckEvent(context, &context->lcpFsm, &lcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Configure-Nak packet
 * @param[in] context PPP context
 * @param[in] configureNakPacket Packet received from the peer
 * @return Error code
 **/

error_t lcpProcessConfigureNak(PppContext *context,
   const PppConfigurePacket *configureNakPacket)
{
   size_t length;
   PppOption *option;

   //Debug message
   TRACE_INFO("LCP Receive-Configure-Nak event\r\n");

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(configureNakPacket->identifier != context->lcpFsm.identifier)
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

      //Maximum-Receive-Unit option?
      if(option->type == LCP_OPTION_MRU)
      {
         //Cast option
         LcpMruOption *mruOption = (LcpMruOption *) option;

         //Check option length
         if(mruOption->length != sizeof(LcpMruOption))
            return ERROR_INVALID_LENGTH;

         //Save value
         context->localConfig.mru = ntohs(mruOption->mru);
         //Make sure the MRU is acceptable
         context->localConfig.mru = MAX(context->localConfig.mru, PPP_MIN_MRU);
         context->localConfig.mru = MIN(context->localConfig.mru, PPP_MAX_MRU);
      }
      else if(option->type == LCP_OPTION_ACCM)
      {
         //Cast option
         LcpAccmOption *accmOption = (LcpAccmOption *) option;

         //Check option length
         if(accmOption->length != sizeof(LcpAccmOption))
            return ERROR_INVALID_LENGTH;

         //Save value
         context->localConfig.accm = ntohl(accmOption->accm);
      }
      //Authentication-Protocol option?
      else if(option->type == LCP_OPTION_AUTH_PROTOCOL)
      {
         //Cast option
         LcpAuthProtocolOption *authProtocolOption = (LcpAuthProtocolOption *) option;

         //Check option length
         if(authProtocolOption->length < sizeof(LcpAuthProtocolOption))
            return ERROR_INVALID_LENGTH;

         //Check the value provided by the peer
         if(ntohs(authProtocolOption->protocol) == PPP_PROTOCOL_PAP)
         {
#if (PAP_SUPPORT == ENABLED)
            //Manage authentication policy
            if(context->settings.authProtocol & PPP_AUTH_PROTOCOL_PAP)
            {
               //Select PAP authentication protocol
               context->localConfig.authProtocol = PPP_PROTOCOL_PAP;
            }
#endif
         }
         else if(ntohs(authProtocolOption->protocol) == PPP_PROTOCOL_CHAP)
         {
#if (CHAP_SUPPORT == ENABLED)
            //Make sure that the length of the option is correct
            if(authProtocolOption->length > sizeof(LcpAuthProtocolOption))
            {
               //Check the algorithm identifier
               if(authProtocolOption->data[0] == CHAP_ALGO_ID_CHAP_MD5)
               {
                  //Manage authentication policy
                  if(context->settings.authProtocol & PPP_AUTH_PROTOCOL_CHAP_MD5)
                  {
                     //Select CHAP with MD5 authentication protocol
                     context->localConfig.authProtocol = PPP_PROTOCOL_CHAP;
                     context->localConfig.authAlgo = CHAP_ALGO_ID_CHAP_MD5;
                  }
               }
            }
#endif
         }
      }

      //Remaining bytes to process
      length -= option->length;
      //Jump to the next option
      option = (PppOption *) ((uint8_t *) option + option->length);
   }

   //A valid Configure-Nak or Configure-Reject packet has been received from the peer
   pppRcvConfigureNakEvent(context, &context->lcpFsm, &lcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Configure-Reject packet
 * @param[in] context PPP context
 * @param[in] configureRejPacket Packet received from the peer
 * @return Error code
 **/

error_t lcpProcessConfigureReject(PppContext *context,
   const PppConfigurePacket *configureRejPacket)
{
   size_t length;
   PppOption *option;

   //Debug message
   TRACE_INFO("\r\nLCP Receive-Configure-Reject event\r\n");

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(configureRejPacket->identifier != context->lcpFsm.identifier)
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

      //Maximum-Receive-Unit option?
      if(option->type == LCP_OPTION_MRU)
      {
         //The option is not recognized by the peer
         context->localConfig.mruRejected = TRUE;
         //Restore default value
         context->localConfig.mru = PPP_DEFAULT_MRU;
      }
      //Async-Control-Character-Map option?
      else if(option->type == LCP_OPTION_ACCM)
      {
         //The option is not recognized by the peer
         context->localConfig.accmRejected = TRUE;
         //Restore default value
         context->localConfig.accm = PPP_DEFAULT_ACCM;
      }
      //Authentication-Protocol option?
      else if(option->type == LCP_OPTION_AUTH_PROTOCOL)
      {
         //This is an unrecoverable error that terminates the connection
         pppRcvCodeRejEvent(context, &context->lcpFsm, &lcpCallbacks, FALSE);
         //Exit immediately
         return ERROR_FAILURE;
      }
      //Magic-Number option?
      else if(option->type == LCP_OPTION_MAGIC_NUMBER)
      {
         //The option is not recognized by the peer
         context->localConfig.magicNumberRejected = TRUE;
         //Restore default value
         context->localConfig.magicNumber = PPP_DEFAULT_MAGIC_NUMBER;
      }
      //Protocol-Field-Compression option?
      else if(option->type == LCP_OPTION_PFC)
      {
         //The option is not recognized by the peer
         context->localConfig.pfcRejected = TRUE;
         //Restore default value
         context->localConfig.pfc = FALSE;
      }
      //Address-and-Control-Field-Compression option?
      else if(option->type == LCP_OPTION_ACFC)
      {
         //The option is not recognized by the peer
         context->localConfig.acfcRejected = TRUE;
         //Restore default value
         context->localConfig.acfc = FALSE;
      }

      //Remaining bytes to process
      length -= option->length;
      //Jump to the next option
      option = (PppOption *) ((uint8_t *) option + option->length);
   }

   //A valid Configure-Nak or Configure-Reject packet has been received from the peer
   pppRcvConfigureNakEvent(context, &context->lcpFsm, &lcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Terminate-Request packet
 * @param[in] context PPP context
 * @param[in] terminateReqPacket Packet received from the peer
 * @return Error code
 **/

error_t lcpProcessTerminateReq(PppContext *context,
   const PppTerminatePacket *terminateReqPacket)
{
   //Debug message
   TRACE_INFO("\r\nLCP Receive-Terminate-Request event\r\n");

   //The Terminate-Request indicates the desire of the peer to close the connection
   pppRcvTerminateReqEvent(context, &context->lcpFsm,
      &lcpCallbacks, terminateReqPacket);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Terminate-Ack packet
 * @param[in] context PPP context
 * @param[in] terminateAckPacket Packet received from the peer
 * @return Error code
 **/

error_t lcpProcessTerminateAck(PppContext *context,
   const PppTerminatePacket *terminateAckPacket)
{
   //Debug message
   TRACE_INFO("\r\nLCP Receive-Terminate-Ack event\r\n");

   //The Terminate-Ack packet is usually a response to a Terminate-Request
   //packet. This packet may also indicate that the peer is in Closed or
   //Stopped states, and serves to re-synchronize the link configuration
   pppRcvTerminateAckEvent(context, &context->lcpFsm, &lcpCallbacks);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Code-Reject packet
 * @param[in] context PPP context
 * @param[in] codeRejPacket Packet received from the peer
 * @return Error code
 **/

error_t lcpProcessCodeRej(PppContext *context,
   const PppCodeRejPacket *codeRejPacket)
{
   size_t length;
   PppPacket *packet;

   //Debug message
   TRACE_INFO("\r\nLCP Receive-Code-Reject event\r\n");

   //Point to the rejected packet
   packet = (PppPacket *) codeRejPacket->rejectedPacket;
   //Retrieve the length of the rejected packet
   length = ntohs(codeRejPacket->length) - sizeof(PppCodeRejPacket);

   //Make sure the length of the rejected packet is valid
   if(length < sizeof(PppPacket))
      return ERROR_INVALID_LENGTH;

   //Check whether the rejected value is acceptable or catastrophic
   if(packet->code < PPP_CODE_CONFIGURE_REQ ||
      packet->code > PPP_CODE_DISCARD_REQ)
   {
      //The RXJ+ event arises when the rejected value is acceptable, such
      //as a Code-Reject of an extended code, or a Protocol-Reject of a
      //NCP. These are within the scope of normal operation
      pppRcvCodeRejEvent(context, &context->lcpFsm, &lcpCallbacks, TRUE);
   }
   else
   {
      //The RXJ- event arises when the rejected value is catastrophic, such
      //as a Code-Reject of Configure-Request! This event communicates an
      //unrecoverable error that terminates the connection
      pppRcvCodeRejEvent(context, &context->lcpFsm, &lcpCallbacks, FALSE);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Protocol-Reject packet
 * @param[in] context PPP context
 * @param[in] protocolRejPacket Packet received from the peer
 * @return Error code
 **/

error_t lcpProcessProtocolRej(PppContext *context,
   const PppProtocolRejPacket *protocolRejPacket)
{
   size_t length;
   uint16_t protocol;

   //Debug message
   TRACE_INFO("\r\nLCP Receive-Protocol-Reject event\r\n");

   //Retrieve the length of the packet
   length = ntohs(protocolRejPacket->length);

   //Make sure the length of the Protocol-Reject packet is valid
   if(length < sizeof(PppProtocolRejPacket))
      return ERROR_INVALID_LENGTH;

   //Convert the Rejected-Protocol field to host byte order
   protocol = ntohs(protocolRejPacket->rejectedProtocol);

   //Check Rejected-Protocol field value
   switch(protocol)
   {
   //LCP protocol?
   case PPP_PROTOCOL_LCP:
      //The rejected value is catastrophic. This event communicates
      //an unrecoverable error that terminates the connection
      pppRcvCodeRejEvent(context, &context->lcpFsm, &lcpCallbacks, FALSE);
      break;

   //IPv4 or IPCP protocol?
   case PPP_PROTOCOL_IP:
   case PPP_PROTOCOL_IPCP:
      //The implementation must stop sending the offending packet type
      context->ipRejected = TRUE;
      //This is within the scope of normal operation...
      pppRcvCodeRejEvent(context, &context->lcpFsm, &lcpCallbacks, TRUE);
      break;

   //IPv6 or IPV6CP protocol?
   case PPP_PROTOCOL_IPV6:
   case PPP_PROTOCOL_IPV6CP:
      //The implementation must stop sending the offending packet type
      context->ipv6Rejected = TRUE;
      //This is within the scope of normal operation...
      pppRcvCodeRejEvent(context, &context->lcpFsm, &lcpCallbacks, TRUE);
      break;

   //Unknown protocol?
   default:
      //Just for sanity's sake...
      break;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Echo-Request packet
 * @param[in] context PPP context
 * @param[in] echoReqPacket Packet received from the peer
 * @return Error code
 **/

error_t lcpProcessEchoReq(PppContext *context,
   const PppEchoPacket *echoReqPacket)
{
   //Debug message
   TRACE_INFO("\r\nLCP Receive-Echo-Request event\r\n");

   //An Echo-Reply packet is transmitted to acknowledge the
   //reception of the Echo-Request packet
   pppRcvEchoReqEvent(context, &context->lcpFsm,
      &lcpCallbacks, echoReqPacket);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Echo-Reply packet
 * @param[in] context PPP context
 * @param[in] echoRepPacket Packet received from the peer
 * @return Error code
 **/

error_t lcpProcessEchoRep(PppContext *context,
   const PppEchoPacket *echoRepPacket)
{
   //Debug message
   TRACE_INFO("\r\nLCP Receive-Echo-Reply event\r\n");

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Discard-Request packet
 * @param[in] context PPP context
 * @param[in] discardReqPacket Packet received from the peer
 * @return Error code
 **/

error_t lcpProcessDiscardReq(PppContext *context,
   const PppDiscardReqPacket *discardReqPacket)
{
   //Debug message
   TRACE_INFO("\r\nLCP Receive-Discard-Request event\r\n");

   //The receiver must silently discard any Discard-Request that it receives
   return NO_ERROR;
}


/**
 * @brief Process packet with unknown code
 * @param[in] context PPP context
 * @param[in] packet Un-interpretable packet received from the peer
 * @return Error code
 **/

error_t lcpProcessUnknownCode(PppContext *context,
   const PppPacket *packet)
{
   //Debug message
   TRACE_INFO("\r\nLCP Receive-Unknown-Code event\r\n");

   //This event occurs when an un-interpretable packet is received from
   //the peer. A Code-Reject packet is sent in response
   pppRcvUnknownCodeEvent(context, &context->lcpFsm, &lcpCallbacks, packet);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process PPP frame with unknown protocol
 * @param[in] context PPP context
 * @param[in] protocol Rejected protocol
 * @param[in] information Rejected information
 * @param[in] length Length of the rejected information
 * @return Error code
 **/

error_t lcpProcessUnknownProtocol(PppContext *context,
   uint16_t protocol, const uint8_t *information, size_t length)
{
   //Debug message
   TRACE_INFO("\r\nLCP Receive-Unknown-Protocol event\r\n");

   //The peer is attempting to use a protocol which is unsupported
   if(context->lcpFsm.state == PPP_STATE_9_OPENED)
   {
      //The Identifier field must be changed for each Protocol-Reject sent
      context->lcpFsm.identifier++;

      //If the LCP automaton is in the Opened state, then this must be
      //reported back to the peer by transmitting a Protocol-Reject
      pppSendProtocolRej(context, context->lcpFsm.identifier,
         protocol, information, length);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief This-Layer-Up callback function
 * @param[in] context PPP context
 **/

void lcpThisLayerUp(PppContext *context)
{
   //Debug message
   TRACE_INFO("LCP This-Layer-Up callback\r\n");

   //Check whether the other end of the PPP link is being authenticated
   if(context->localConfig.authProtocol != 0)
      context->localAuthDone = FALSE;
   else
      context->localAuthDone = TRUE;

   //Check whether the other end of the PPP link is the authenticator
   if(context->peerConfig.authProtocol != 0)
      context->peerAuthDone = FALSE;
   else
      context->peerAuthDone = TRUE;

#if (PAP_SUPPORT == ENABLED)
   //PAP authentication required?
   if(context->localConfig.authProtocol == PPP_PROTOCOL_PAP ||
      context->peerConfig.authProtocol == PPP_PROTOCOL_PAP)
   {
      //Advance to the Authentication phase
      context->pppPhase = PPP_PHASE_AUTHENTICATE;
      //Start PAP authentication process
      papStartAuth(context);
   }
#endif
#if (CHAP_SUPPORT == ENABLED)
   //CHAP authentication required?
   if(context->localConfig.authProtocol == PPP_PROTOCOL_CHAP ||
      context->peerConfig.authProtocol == PPP_PROTOCOL_CHAP)
   {
      //Advance to the Authentication phase
      context->pppPhase = PPP_PHASE_AUTHENTICATE;
      //Start CHAP authentication process
      chapStartAuth(context);
   }
#endif

   //Check whether PPP authentication is complete
   if(context->localAuthDone && context->peerAuthDone)
   {
      //Advance to the Network phase
      context->pppPhase = PPP_PHASE_NETWORK;

#if (IPV4_SUPPORT == ENABLED)
      //IPCP Open event
      ipcpOpen(context);
#endif
#if (IPV6_SUPPORT == ENABLED)
      //IPV6CP Open event
      ipv6cpOpen(context);
#endif
   }
}


/**
 * @brief This-Layer-Down callback function
 * @param[in] context PPP context
 **/

void lcpThisLayerDown(PppContext *context)
{
   //Debug message
   TRACE_INFO("LCP This-Layer-Down callback\r\n");

   //Advance to the Terminate phase
   context->pppPhase = PPP_PHASE_TERMINATE;

#if (IPV4_SUPPORT == ENABLED)
   //IPCP Close event
   ipcpClose(context);
#endif
#if (IPV6_SUPPORT == ENABLED)
   //IPV6CP Close event
   ipv6cpClose(context);
#endif

#if (PAP_SUPPORT == ENABLED)
   //Abort PAP authentication process
   papAbortAuth(context);
#endif

#if (CHAP_SUPPORT == ENABLED)
   //Abort CHAP authentication process
   chapAbortAuth(context);
#endif
}


/**
 * @brief This-Layer-Started callback function
 * @param[in] context PPP context
 **/

void lcpThisLayerStarted(PppContext *context)
{
   //Debug message
   TRACE_INFO("LCP This-Layer-Started callback\r\n");
}


/**
 * @brief This-Layer-Finished callback function
 * @param[in] context PPP context
 **/

void lcpThisLayerFinished(PppContext *context)
{
   //Debug message
   TRACE_INFO("LCP This-Layer-Finished callback\r\n");

   //The link is no longer available for traffic
   pppCloseEvent(context, &context->lcpFsm, &lcpCallbacks);
   //The lower layer is no longer ready to carry packets
   pppDownEvent(context, &context->lcpFsm, &lcpCallbacks);

   //Advance to the Link Dead phase
   context->pppPhase = PPP_PHASE_DEAD;
}


/**
 * @brief Initialize-Restart-Count callback function
 * @param[in] context PPP context
 * @param[in] value Restart counter value
 **/

void lcpInitRestartCount(PppContext *context, uint_t value)
{
   //Debug message
   TRACE_INFO("LCP Initialize-Restart-Count callback\r\n");

   //Initialize restart counter
   context->lcpFsm.restartCounter = value;
}


/**
 * @brief Zero-Restart-Count callback function
 * @param[in] context PPP context
 **/

void lcpZeroRestartCount(PppContext *context)
{
   //Debug message
   TRACE_INFO("LCP Zero-Restart-Count callback\r\n");

   //Zero restart counter
   context->lcpFsm.restartCounter = 0;

   //The receiver of a Terminate-Request should wait for the peer to
   //disconnect, and must not disconnect until at least one Restart
   //time has passed after sending a Terminate-Ack
   context->lcpFsm.timestamp = osGetSystemTime();
}


/**
 * @brief Send-Configure-Request callback function
 * @param[in] context PPP context
 * @return Error code
 **/

error_t lcpSendConfigureReq(PppContext *context)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PppConfigurePacket *configureReqPacket;

   //Debug message
   TRACE_INFO("LCP Send-Configure-Request callback\r\n");

   //Allocate a buffer memory to hold the Configure-Request packet
   buffer = pppAllocBuffer(PPP_MAX_CONF_REQ_SIZE, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Configure-Request packet
   configureReqPacket = netBufferAt(buffer, offset);

   //Format packet header
   configureReqPacket->code = PPP_CODE_CONFIGURE_REQ;
   configureReqPacket->identifier = ++context->lcpFsm.identifier;
   configureReqPacket->length = sizeof(PppConfigurePacket);

   //Make sure the Maximum-Receive-Unit option has not been
   //previously rejected
   if(!context->localConfig.mruRejected)
   {
      //Convert MRU to network byte order
      uint16_t value = htons(context->localConfig.mru);
      //Add option
      pppAddOption(configureReqPacket, LCP_OPTION_MRU, &value, sizeof(uint16_t));
   }

   //Make sure the Async-Control-Character-Map option has not been
   //previously rejected
   if(!context->localConfig.accmRejected)
   {
      //Convert ACCM to network byte order
      uint32_t value = htonl(context->localConfig.accm);
      //Add option
      pppAddOption(configureReqPacket, LCP_OPTION_ACCM, &value, sizeof(uint32_t));
   }

   //Make sure the Authentication-Protocol option has not been
   //previously rejected
   if(!context->localConfig.authProtocolRejected)
   {
      uint8_t value[3];

      //PAP authentication protocol?
      if(context->localConfig.authProtocol == PPP_PROTOCOL_PAP)
      {
         //Format Authentication-Protocol option
         value[0] = MSB(PPP_PROTOCOL_PAP);
         value[1] = LSB(PPP_PROTOCOL_PAP);

         //Add option
         pppAddOption(configureReqPacket, LCP_OPTION_AUTH_PROTOCOL, &value, 2);
      }
      //CHAP authentication protocol?
      else if(context->localConfig.authProtocol == PPP_PROTOCOL_CHAP)
      {
         //Format Authentication-Protocol option
         value[0] = MSB(PPP_PROTOCOL_CHAP);
         value[1] = LSB(PPP_PROTOCOL_CHAP);
         value[2] = context->localConfig.authAlgo;

         //Add option
         pppAddOption(configureReqPacket, LCP_OPTION_AUTH_PROTOCOL, &value, 3);
      }
   }

   //Make sure the Protocol-Field-Compression option has not been
   //previously rejected
   if(!context->localConfig.pfcRejected)
   {
      //Check whether compression of the Protocol field is supported
      if(context->localConfig.pfc)
      {
         //Add option
         pppAddOption(configureReqPacket, LCP_OPTION_PFC, NULL, 0);
      }
   }

   //Make sure the Address-and-Control-Field-Compression option has not been
   //previously rejected
   if(!context->localConfig.acfcRejected)
   {
      //Check whether compression of the Address and Control fields is supported
      if(context->localConfig.acfc)
      {
         //Add option
         pppAddOption(configureReqPacket, LCP_OPTION_ACFC, NULL, 0);
      }
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
   pppDumpPacket((PppPacket *) configureReqPacket, length, PPP_PROTOCOL_LCP);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, PPP_PROTOCOL_LCP);

   //The restart counter is decremented each time a Configure-Request is sent
   if(context->lcpFsm.restartCounter > 0)
      context->lcpFsm.restartCounter--;

   //Save the time at which the packet was sent
   context->lcpFsm.timestamp = osGetSystemTime();

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

error_t lcpSendConfigureAck(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   //Debug message
   TRACE_INFO("LCP Send-Configure-Ack callback\r\n");

   //Send Configure-Ack packet
   return pppSendConfigureAckNak(context, configureReqPacket,
      PPP_PROTOCOL_LCP, PPP_CODE_CONFIGURE_ACK);
}


/**
 * @brief Send-Configure-Nak callback function
 * @param[in] context PPP context
 * @param[in] configureReqPacket Configure-Request packet received from the peer
 * @return Error code
 **/

error_t lcpSendConfigureNak(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   //Debug message
   TRACE_INFO("LCP Send-Configure-Nak callback\r\n");

   //Send Configure-Nak packet
   return pppSendConfigureAckNak(context, configureReqPacket,
      PPP_PROTOCOL_LCP, PPP_CODE_CONFIGURE_NAK);
}


/**
 * @brief Send-Configure-Reject callback function
 * @param[in] context PPP context
 * @param[in] configureReqPacket Configure-Request packet received from the peer
 * @return Error code
 **/

error_t lcpSendConfigureRej(PppContext *context,
   const PppConfigurePacket *configureReqPacket)
{
   //Debug message
   TRACE_INFO("LCP Send-Configure-Reject callback\r\n");

   //Send Configure-Reject packet
   return pppSendConfigureAckNak(context, configureReqPacket,
      PPP_PROTOCOL_LCP, PPP_CODE_CONFIGURE_REJ);
}


/**
 * @brief Send-Terminate-Request callback function
 * @param[in] context PPP context
 * @return Error code
 **/

error_t lcpSendTerminateReq(PppContext *context)
{
   error_t error;

   //Debug message
   TRACE_INFO("LCP Send-Terminate-Request callback\r\n");

   //On transmission, the Identifier field must be changed
   context->lcpFsm.identifier++;

   //Send Terminate-Request packet
   error = pppSendTerminateReq(context, context->lcpFsm.identifier, PPP_PROTOCOL_LCP);

   //The restart counter is decremented each time a Terminate-Request is sent
   if(context->lcpFsm.restartCounter > 0)
      context->lcpFsm.restartCounter--;

   //Save the time at which the packet was sent
   context->lcpFsm.timestamp = osGetSystemTime();

   //Return status code
   return error;
}


/**
 * @brief Send-Terminate-Ack callback function
 * @param[in] context PPP context
 * @param[in] terminateReqPacket Terminate-Request packet received from the peer
 * @return Error code
 **/

error_t lcpSendTerminateAck(PppContext *context,
   const PppTerminatePacket *terminateReqPacket)
{
   uint8_t identifier;

   //Debug message
   TRACE_INFO("LCP Send-Terminate-Ack callback\r\n");

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
      identifier = ++context->lcpFsm.identifier;
   }

   //Send Terminate-Ack packet
   return pppSendTerminateAck(context, identifier, PPP_PROTOCOL_LCP);
}


/**
 * @brief Send-Code-Reject callback function
 * @param[in] context PPP context
 * @param[in] packet Un-interpretable packet received from the peer
 * @return Error code
 **/

error_t lcpSendCodeRej(PppContext *context, const PppPacket *packet)
{
   //Debug message
   TRACE_INFO("LCP Send-Code-Reject callback\r\n");

   //The Identifier field must be changed for each Code-Reject sent
   context->lcpFsm.identifier++;

   //Send Code-Reject packet
   return pppSendCodeRej(context, packet, context->lcpFsm.identifier, PPP_PROTOCOL_LCP);
}


/**
 * @brief Send-Echo-Reply callback function
 * @param[in] context PPP context
 * @param[in] echoReqPacket Echo-Request packet received from the peer
 * @return Error code
 **/

error_t lcpSendEchoRep(PppContext *context, const PppEchoPacket *echoReqPacket)
{
   //Debug message
   TRACE_INFO("LCP Send-Echo-Reply callback\r\n");

   //Send Echo-Reply packet
   return pppSendEchoRep(context, echoReqPacket, PPP_PROTOCOL_LCP);
}


/**
 * @brief Parse LCP configuration option
 * @param[in] context PPP context
 * @param[in] option Option to be checked
 * @param[in] inPacketLen Remaining bytes to process in the incoming packet
 * @param[out] outPacket Pointer to the Configure-Ack, Nak or Reject packet
 * @return Error code
 **/

error_t lcpParseOption(PppContext *context, PppOption *option,
   size_t inPacketLen, PppConfigurePacket *outPacket)
{
   error_t error;

   //Malformed LCP packet?
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
   case LCP_OPTION_MRU:
      //Check Maximum-Receive-Unit option
      error = lcpParseMruOption(context, (LcpMruOption *) option, outPacket);
      break;
   case LCP_OPTION_ACCM:
      //Check Async-Control-Character-Map option
      error = lcpParseAccmOption(context, (LcpAccmOption *) option, outPacket);
      break;
   case LCP_OPTION_AUTH_PROTOCOL:
      //Check Authentication-Protocol option
      error = lcpParseAuthProtocolOption(context, (LcpAuthProtocolOption *) option, outPacket);
      break;
   case LCP_OPTION_MAGIC_NUMBER:
      //Check Magic-Number option
      error = lcpParseMagicNumberOption(context, (LcpMagicNumberOption *) option, outPacket);
      break;
   case LCP_OPTION_PFC:
      //Check Protocol-Field-Compression option
      error = lcpParsePfcOption(context, (LcpPfcOption *) option, outPacket);
      break;
   case LCP_OPTION_ACFC:
      //Check Address-and-Control-Field-Compression option
      error = lcpParseAcfcOption(context, (LcpAcfcOption *) option, outPacket);
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
 * @brief Parse Maximum-Receive-Unit option
 * @param[in] context PPP context
 * @param[in] option Option to be checked
 * @param[out] outPacket Pointer to the Configure-Nak or Configure-Reject packet
 * @return Error code
 **/

error_t lcpParseMruOption(PppContext *context,
   LcpMruOption *option, PppConfigurePacket *outPacket)
{
   error_t error;
   uint16_t value;

   //Check length field
   if(option->length == sizeof(LcpMruOption))
   {
      //Check whether the option value is acceptable
      if(ntohs(option->mru) >= PPP_MIN_MRU)
      {
         //If every configuration option received in the Configure-Request is
         //recognizable and all values are acceptable, then the implementation
         //must transmit a Configure-Ack
         if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_ACK)
         {
            //Save Maximum-Receive-Unit option
            context->peerConfig.mru = ntohl(option->mru);

            //The options field of the Configure-Ack packet contains the
            //configuration options that the sender is acknowledging
            pppAddOption(outPacket, LCP_OPTION_MRU, (void *) &option->mru,
               option->length - sizeof(PppOption));
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
            //Use default value
            value = htons(PPP_DEFAULT_MRU);

            //The option must be modified to a value acceptable to the
            //Configure-Nak sender
            pppAddOption(outPacket, LCP_OPTION_MRU, &value, sizeof(uint16_t));
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


/**
 * @brief Parse Async-Control-Character-Map option
 * @param[in] context PPP context
 * @param[in] option Option to be checked
 * @param[out] outPacket Pointer to the Configure-Nak or Configure-Reject packet
 * @return Error code
 **/

error_t lcpParseAccmOption(PppContext *context,
   LcpAccmOption *option, PppConfigurePacket *outPacket)
{
   error_t error;

   //Check length field
   if(option->length == sizeof(LcpAccmOption))
   {
      //If every configuration option received in the Configure-Request is
      //recognizable and all values are acceptable, then the implementation
      //must transmit a Configure-Ack
      if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_ACK)
      {
         //Save Async-Control-Character-Map option
         context->peerConfig.accm = ntohl(option->accm);

         //The options field of the Configure-Ack packet contains the
         //configuration options that the sender is acknowledging
         pppAddOption(outPacket, LCP_OPTION_ACCM, (void *) &option->accm,
            option->length - sizeof(PppOption));
      }

      //The value is acceptable
      error = NO_ERROR;
   }
   else
   {
      //Invalid length field
      error = ERROR_INVALID_LENGTH;
   }

   //Return status code
   return error;
}


/**
 * @brief Parse Authentication-Protocol option
 * @param[in] context PPP context
 * @param[in] option Option to be checked
 * @param[out] outPacket Pointer to the Configure-Nak or Configure-Reject packet
 * @return Error code
 **/

error_t lcpParseAuthProtocolOption(PppContext *context,
   LcpAuthProtocolOption *option, PppConfigurePacket *outPacket)
{
   error_t error;
   uint8_t value[3];

   //Assume an error condition...
   error = ERROR_INVALID_LENGTH;

   //Check the length of the option
   if(option->length >= sizeof(LcpAuthProtocolOption))
   {
      //The Authentication-Protocol option for PAP must be exactly 4 bytes
      if(ntohs(option->protocol) == PPP_PROTOCOL_PAP)
      {
         if(option->length == 4)
            error = NO_ERROR;
      }
      //The Authentication-Protocol option for CHAP must be exactly 5 bytes
      else if(ntohs(option->protocol) == PPP_PROTOCOL_CHAP)
      {
         if(option->length == 5)
            error = NO_ERROR;
      }
   }

   //Make sure the length field is valid
   if(!error)
   {
      //PAP authentication protocol?
      if(context->settings.authProtocol & PPP_AUTH_PROTOCOL_PAP &&
         ntohs(option->protocol) == PPP_PROTOCOL_PAP)
      {
         //If every configuration option received in the Configure-Request is
         //recognizable and all values are acceptable, then the implementation
         //must transmit a Configure-Ack
         if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_ACK)
         {
            //Save the authentication protocol to be used
            context->peerConfig.authProtocol = PPP_PROTOCOL_PAP;

            //The options field of the Configure-Ack packet contains the
            //configuration options that the sender is acknowledging
            pppAddOption(outPacket, option->type, (void *) &option->protocol,
               option->length - sizeof(PppOption));
         }

         //The value is acceptable
         error = NO_ERROR;
      }
      //CHAP with MD5 authentication protocol?
      else if(context->settings.authProtocol & PPP_AUTH_PROTOCOL_CHAP_MD5 &&
         ntohs(option->protocol) == PPP_PROTOCOL_CHAP &&
         option->data[0] == CHAP_ALGO_ID_CHAP_MD5)
      {
         //If every configuration option received in the Configure-Request is
         //recognizable and all values are acceptable, then the implementation
         //must transmit a Configure-Ack
         if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_ACK)
         {
            //Save the authentication protocol to be used
            context->peerConfig.authProtocol = PPP_PROTOCOL_CHAP;
            context->peerConfig.authAlgo = CHAP_ALGO_ID_CHAP_MD5;

            //The options field of the Configure-Ack packet contains the
            //configuration options that the sender is acknowledging
            pppAddOption(outPacket, option->type, (void *) &option->protocol,
               option->length - sizeof(PppOption));
         }

         //The value is acceptable
         error = NO_ERROR;
      }
      else
      {
         //PAP authentication protocol allowed?
         if(context->settings.authProtocol & PPP_AUTH_PROTOCOL_PAP)
         {
            //If all configuration options are recognizable, but some values are not
            //acceptable, then the implementation must transmit a Configure-Nak
            if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_NAK)
            {
               //Format Authentication-Protocol option
               value[0] = MSB(PPP_PROTOCOL_PAP);
               value[1] = LSB(PPP_PROTOCOL_PAP);

               //The option must be modified to a value acceptable to the
               //Configure-Nak sender
               pppAddOption(outPacket, LCP_OPTION_AUTH_PROTOCOL, value, 2);
            }

            //The value is not acceptable
            error = ERROR_INVALID_VALUE;
         }
         //CHAP with MD5 authentication protocol allowed?
         else if(context->settings.authProtocol & PPP_AUTH_PROTOCOL_CHAP_MD5)
         {
            //If all configuration options are recognizable, but some values are not
            //acceptable, then the implementation must transmit a Configure-Nak
            if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_NAK)
            {
               //Format Authentication-Protocol option
               value[0] = MSB(PPP_PROTOCOL_CHAP);
               value[1] = LSB(PPP_PROTOCOL_CHAP);
               value[2] = CHAP_ALGO_ID_CHAP_MD5;

               //The option must be modified to a value acceptable to the
               //Configure-Nak sender
               pppAddOption(outPacket, LCP_OPTION_AUTH_PROTOCOL, value, 3);
            }

            //The value is not acceptable
            error = ERROR_INVALID_VALUE;
         }
         else
         {
            //If some configuration options received in the Configure-Request are not
            //recognizable or not acceptable for negotiation, then the implementation
            //must transmit a Configure-Reject
            if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_REJ)
            {
               //The options field of the Configure-Reject packet is filled
               //with the unrecognized options from the Configure-Request
               pppAddOption(outPacket, option->type, (void *) &option->protocol,
                  option->length - sizeof(PppOption));
            }

            //The option is not acceptable for negotiation
            error = ERROR_INVALID_TYPE;
         }
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Parse Magic-Number option
 * @param[in] context PPP context
 * @param[in] option Option to be checked
 * @param[out] outPacket Pointer to the Configure-Nak or Configure-Reject packet
 * @return Error code
 **/

error_t lcpParseMagicNumberOption(PppContext *context,
   LcpMagicNumberOption *option, PppConfigurePacket *outPacket)
{
   error_t error;

   //Check length field
   if(option->length == sizeof(LcpMagicNumberOption))
   {
      //If every configuration option received in the Configure-Request is
      //recognizable and all values are acceptable, then the implementation
      //must transmit a Configure-Ack
      if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_ACK)
      {
         //Save Magic-Number option
         context->peerConfig.magicNumber = ntohl(option->magicNumber);

         //The options field of the Configure-Ack packet contains the
         //configuration options that the sender is acknowledging
         pppAddOption(outPacket, LCP_OPTION_MAGIC_NUMBER, (void *) &option->magicNumber,
            option->length - sizeof(PppOption));
      }

      //The value is acceptable
      error = NO_ERROR;
   }
   else
   {
      //Invalid length field
      error = ERROR_INVALID_LENGTH;
   }

   //Return status code
   return error;
}


/**
 * @brief Parse Protocol-Field-Compression option
 * @param[in] context PPP context
 * @param[in] option Option to be checked
 * @param[out] outPacket Pointer to the Configure-Nak or Configure-Reject packet
 * @return Error code
 **/

error_t lcpParsePfcOption(PppContext *context,
   LcpPfcOption *option, PppConfigurePacket *outPacket)
{
   error_t error;

   //Check length field
   if(option->length == sizeof(LcpPfcOption))
   {
      //If every configuration option received in the Configure-Request is
      //recognizable and all values are acceptable, then the implementation
      //must transmit a Configure-Ack
      if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_ACK)
      {
         //Save Protocol-Field-Compression option
         context->peerConfig.pfc = TRUE;

         //The options field of the Configure-Ack packet contains the
         //configuration options that the sender is acknowledging
         pppAddOption(outPacket, LCP_OPTION_PFC, NULL, 0);
      }

      //The value is acceptable
      error = NO_ERROR;
   }
   else
   {
      //Invalid length field
      error = ERROR_INVALID_LENGTH;
   }

   //Return status code
   return error;
}


/**
 * @brief Parse Address-and-Control-Field-Compression option
 * @param[in] context PPP context
 * @param[in] option Option to be checked
 * @param[out] outPacket Pointer to the Configure-Nak or Configure-Reject packet
 * @return Error code
 **/

error_t lcpParseAcfcOption(PppContext *context,
   LcpAcfcOption *option, PppConfigurePacket *outPacket)
{
   error_t error;

   //Check length field
   if(option->length == sizeof(LcpAcfcOption))
   {
      //If every configuration option received in the Configure-Request is
      //recognizable and all values are acceptable, then the implementation
      //must transmit a Configure-Ack
      if(outPacket != NULL && outPacket->code == PPP_CODE_CONFIGURE_ACK)
      {
         //Save Address-and-Control-Field-Compression option
         context->peerConfig.acfc = TRUE;

         //The options field of the Configure-Ack packet contains the
         //configuration options that the sender is acknowledging
         pppAddOption(outPacket, LCP_OPTION_ACFC, NULL, 0);
      }

      //The value is acceptable
      error = NO_ERROR;
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
