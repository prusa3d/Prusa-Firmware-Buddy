/**
 * @file pap.c
 * @brief PAP (Password Authentication Protocol)
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
#include "ppp/ppp_debug.h"
#include "ppp/lcp.h"
#include "ppp/ipcp.h"
#include "ppp/ipv6cp.h"
#include "ppp/pap.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (PPP_SUPPORT == ENABLED && PAP_SUPPORT == ENABLED)


/**
 * @brief Start PAP authentication
 * @param[in] context PPP context
 * @return Error code
 **/

error_t papStartAuth(PppContext *context)
{
   //Debug message
   TRACE_INFO("\r\nStarting PAP authentication...\r\n");

   //Check whether the other end of the PPP link is being authenticated
   if(context->localConfig.authProtocol == PPP_PROTOCOL_PAP)
   {
      //Switch to the Started state
      context->papFsm.localState = PAP_STATE_1_STARTED;
   }

   //Check whether the other end of the PPP link is the authenticator
   if(context->peerConfig.authProtocol == PPP_PROTOCOL_PAP)
   {
      //Initialize restart counter
      context->papFsm.restartCounter = PAP_MAX_REQUESTS;
      //Send Authenticate-Request packet
      papSendAuthReq(context);
      //Switch to the Req-Sent state
      context->papFsm.peerState = PAP_STATE_2_REQ_SENT;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Abort PAP authentication
 * @param[in] context PPP context
 * @return Error code
 **/

error_t papAbortAuth(PppContext *context)
{
   //Debug message
   TRACE_INFO("\r\nAborting PAP authentication...\r\n");

   //Abort PAP authentication process
   context->papFsm.localState = PAP_STATE_0_INITIAL;
   context->papFsm.peerState = PAP_STATE_0_INITIAL;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief PAP timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * manage retransmissions
 *
 * @param[in] context PPP context
 **/

void papTick(PppContext *context)
{
   //Check whether the restart timer is running
   if(context->papFsm.peerState == PAP_STATE_2_REQ_SENT)
   {
      //Get current time
      systime_t time = osGetSystemTime();

      //Check restart timer
      if((time - context->papFsm.timestamp) >= PAP_RESTART_TIMER)
      {
         //Debug message
         TRACE_INFO("\r\nPAP Timeout event\r\n");

         //Check whether the restart counter is greater than zero
         if(context->papFsm.restartCounter > 0)
         {
            //Retransmit the Authenticate-Request packet
            papSendAuthReq(context);
         }
         else
         {
            //Abort PAP authentication
            context->papFsm.peerState = PAP_STATE_0_INITIAL;
            //Authentication failed
            lcpClose(context);
         }
      }
   }
}


/**
 * @brief Process an incoming PAP packet
 * @param[in] context PPP context
 * @param[in] packet PAP packet received from the peer
 * @param[in] length Length of the packet, in bytes
 **/

void papProcessPacket(PppContext *context,
   const PppPacket *packet, size_t length)
{
   //Ensure the length of the incoming PAP packet is valid
   if(length < sizeof(PppPacket))
      return;

   //Check the length field
   if(ntohs(packet->length) > length)
      return;
   if(ntohs(packet->length) < sizeof(PppPacket))
      return;

   //Save the length of the PAP packet
   length = ntohs(packet->length);

   //Debug message
   TRACE_INFO("PAP packet received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump PAP packet contents for debugging purpose
   pppDumpPacket(packet, length, PPP_PROTOCOL_PAP);

   //Because the Authenticate-Ack might be lost, the authenticator must
   //allow repeated Authenticate-Request packets after completing the
   //Authentication phase
   if(context->pppPhase != PPP_PHASE_AUTHENTICATE &&
      context->pppPhase != PPP_PHASE_NETWORK)
   {
      //Any packets received during any other phase must be silently discarded
      return;
   }

   //Check PAP code field
   switch(packet->code)
   {
   //Authenticate-Request packet?
   case PAP_CODE_AUTH_REQ:
      //Process Authenticate-Request packet
      papProcessAuthReq(context, (PapAuthReqPacket *) packet, length);
      break;
   //Authenticate-Ack packet?
   case PAP_CODE_AUTH_ACK:
      //Process Authenticate-Ack packet
      papProcessAuthAck(context, (PapAuthAckPacket *) packet, length);
      break;
   //Authenticate-Nak packet?
   case PAP_CODE_AUTH_NAK:
      //Process Authenticate-Nak packet
      papProcessAuthNak(context, (PapAuthNakPacket *) packet, length);
      break;
   //Unknown code field
   default:
      //Silently drop the incoming packet
      break;
   }
}


/**
 * @brief Process Authenticate-Request packet
 * @param[in] context PPP context
 * @param[in] authReqPacket Packet received from the peer
 * @param[in] length Length of the packet, in bytes
 * @return Error code
 **/

error_t papProcessAuthReq(PppContext *context,
   const PapAuthReqPacket *authReqPacket, size_t length)
{
   bool_t status;
   size_t usernameLen;
   const uint8_t *p;

   //Debug message
   TRACE_INFO("\r\nPAP Authenticate-Request packet received\r\n");

   //Make sure the Authenticate-Request packet is acceptable
   if(context->localConfig.authProtocol != PPP_PROTOCOL_PAP)
      return ERROR_FAILURE;

   //Check the length of the packet
   if(length < sizeof(PapAuthReqPacket))
      return ERROR_INVALID_LENGTH;

   //Retrieve the length of the Peer-ID field
   usernameLen = authReqPacket->peerIdLength;

   //Malformed Authenticate-Request packet?
   if(length < (sizeof(PapAuthReqPacket) + 1 + usernameLen))
      return ERROR_INVALID_LENGTH;

   //Limit the length of the string
   usernameLen = MIN(usernameLen, PPP_MAX_USERNAME_LEN);
   //Copy the name of the peer to be identified
   osMemcpy(context->peerName, authReqPacket->peerId, usernameLen);
   //Properly terminate the string with a NULL character
   context->peerName[usernameLen] = '\0';

   //Point to the Passwd-Length field
   p = authReqPacket->peerId + usernameLen;

   //Save the length of Password field
   context->papFsm.passwordLen = p[0];
   //Point to the Password field
   context->papFsm.password = p + 1;

   //Malformed Authenticate-Request packet?
   if(length < (sizeof(PapAuthReqPacket) + 1 + usernameLen + context->papFsm.passwordLen))
      return ERROR_INVALID_LENGTH;

   //Invoke user-defined callback, if any
   if(context->settings.authCallback != NULL)
   {
      //Perfom username and password verification
      status = context->settings.authCallback(context->interface,
         context->peerName);
   }
   else
   {
      //Unable to perform authentication...
      status = FALSE;
   }

   //Successful authentication?
   if(status)
   {
      //If the Peer-ID/Password pair received in the Authenticate-Request
      //is both recognizable and acceptable, then the authenticator must
      //transmit an Authenticate-Ack packet
      papSendAuthAck(context, authReqPacket->identifier);

      //Switch to the Ack-Sent state
      context->papFsm.localState = PAP_STATE_4_ACK_SENT;
      //The user has been successfully authenticated
      context->localAuthDone = TRUE;

      //Check whether PPP authentication is complete
      if(context->localAuthDone && context->peerAuthDone)
      {
         //Check current PPP phase
         if(context->pppPhase == PPP_PHASE_AUTHENTICATE)
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
   }
   else
   {
      //If the Peer-ID/Password pair received in the Authenticate-Request
      //is not recognizable or acceptable, then the authenticator must
      //transmit an Authenticate-Nak packet
      papSendAuthNak(context, authReqPacket->identifier);

      //Switch to the Nak-Sent state
      context->papFsm.localState = PAP_STATE_6_NAK_SENT;
      //The authenticator should take action to terminate the link
      lcpClose(context);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Authenticate-Ack packet
 * @param[in] context PPP context
 * @param[in] authAckPacket Packet received from the peer
 * @param[in] length Length of the packet, in bytes
 * @return Error code
 **/

error_t papProcessAuthAck(PppContext *context,
   const PapAuthAckPacket *authAckPacket, size_t length)
{
   //Debug message
   TRACE_INFO("\r\nPAP Authenticate-Ack packet received\r\n");

   //Make sure the Authenticate-Ack packet is acceptable
   if(context->peerConfig.authProtocol != PPP_PROTOCOL_PAP)
      return ERROR_FAILURE;

   //Check the length of the packet
   if(length < sizeof(PapAuthAckPacket))
      return ERROR_INVALID_LENGTH;

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(authAckPacket->identifier != context->papFsm.identifier)
      return ERROR_WRONG_IDENTIFIER;

   //Switch to the Ack-Rcvd state
   context->papFsm.peerState = PAP_STATE_5_ACK_RCVD;
   //The user name has been accepted by the authenticator
   context->peerAuthDone = TRUE;

   //Check whether PPP authentication is complete
   if(context->localAuthDone && context->peerAuthDone)
   {
      //Check current PPP phase
      if(context->pppPhase == PPP_PHASE_AUTHENTICATE)
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

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Authenticate-Nak packet
 * @param[in] context PPP context
 * @param[in] authNakPacket Packet received from the peer
 * @param[in] length Length of the packet, in bytes
 * @return Error code
 **/

error_t papProcessAuthNak(PppContext *context,
   const PapAuthNakPacket *authNakPacket, size_t length)
{
   //Debug message
   TRACE_INFO("\r\nPAP Authenticate-Nak packet received\r\n");

   //Make sure the Authenticate-Nak packet is acceptable
   if(context->peerConfig.authProtocol != PPP_PROTOCOL_PAP)
      return ERROR_FAILURE;

   //Check the length of the packet
   if(length < sizeof(PapAuthNakPacket))
      return ERROR_INVALID_LENGTH;

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(authNakPacket->identifier != context->papFsm.identifier)
      return ERROR_WRONG_IDENTIFIER;

   //Switch to the Nak-Rcvd state
   context->papFsm.peerState = PAP_STATE_7_NAK_RCVD;
   //Authentication failed
   lcpClose(context);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Send Authenticate-Request packet
 * @param[in] context PPP context
 * @return Error code
 **/

error_t papSendAuthReq(PppContext *context)
{
   error_t error;
   size_t usernameLen;
   size_t passwordLen;
   size_t length;
   size_t offset;
   uint8_t *p;
   NetBuffer *buffer;
   PapAuthReqPacket *authReqPacket;

   //Get the length of the user name
   usernameLen = osStrlen(context->username);
   //Get the length of the password
   passwordLen = osStrlen(context->password);

   //Calculate the length of the Authenticate-Request packet
   length = sizeof(PapAuthReqPacket) + 1 + usernameLen + passwordLen;

   //Allocate a buffer memory to hold the packet
   buffer = pppAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Authenticate-Request packet
   authReqPacket = netBufferAt(buffer, offset);

   //Format packet header
   authReqPacket->code = PAP_CODE_AUTH_REQ;
   authReqPacket->identifier = ++context->papFsm.identifier;
   authReqPacket->length = htons(length);

   //The Peer-ID-Length field indicates the length of Peer-ID field
   authReqPacket->peerIdLength = usernameLen;
   //Append Peer-ID
   osMemcpy(authReqPacket->peerId, context->username, usernameLen);

   //Point to the Passwd-Length field
   p = authReqPacket->peerId + usernameLen;
   //The Passwd-Length field indicates the length of Password field
   p[0] = passwordLen;

   //Append Password
   osMemcpy(p + 1, context->password, passwordLen);

   //Adjust the length of the multi-part buffer
   netBufferSetLength(buffer, offset + length);

   //Debug message
   TRACE_INFO("Sending PAP Authenticate-Request packet (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump packet contents for debugging purpose
   pppDumpPacket((PppPacket *) authReqPacket, length, PPP_PROTOCOL_PAP);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, PPP_PROTOCOL_PAP);

   //The restart counter is decremented each time a Authenticate-Request is sent
   if(context->papFsm.restartCounter > 0)
      context->papFsm.restartCounter--;

   //Save the time at which the packet was sent
   context->papFsm.timestamp = osGetSystemTime();

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send Authenticate-Ack packet
 * @param[in] context PPP context
 * @param[in] identifier Identifier field
 * @return Error code
 **/

error_t papSendAuthAck(PppContext *context, uint8_t identifier)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PapAuthAckPacket *authAckPacket;

   //Retrieve the length of the Authenticate-Ack packet
   length = sizeof(PapAuthAckPacket);

   //Allocate a buffer memory to hold the Authenticate-Ack packet
   buffer = pppAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Authenticate-Ack packet
   authAckPacket = netBufferAt(buffer, offset);

   //Format packet header
   authAckPacket->code = PAP_CODE_AUTH_ACK;
   authAckPacket->identifier = identifier;
   authAckPacket->length = htons(length);

   //The Message field is zero or more octets, and its contents are
   //implementation dependent
   authAckPacket->msgLength = 0;

   //Debug message
   TRACE_INFO("Sending PAP Authenticate-Ack packet (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump packet contents for debugging purpose
   pppDumpPacket((PppPacket *) authAckPacket, length, PPP_PROTOCOL_PAP);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, PPP_PROTOCOL_PAP);

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send Authenticate-Nak packet
 * @param[in] context PPP context
 * @param[in] identifier Identifier field
 * @return Error code
 **/

error_t papSendAuthNak(PppContext *context, uint8_t identifier)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PapAuthNakPacket *authNakPacket;

   //Retrieve the length of the Authenticate-Nak packet
   length = sizeof(PapAuthNakPacket);

   //Allocate a buffer memory to hold the Authenticate-Nak packet
   buffer = pppAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Authenticate-Nak packet
   authNakPacket = netBufferAt(buffer, offset);

   //Format packet header
   authNakPacket->code = PAP_CODE_AUTH_NAK;
   authNakPacket->identifier = identifier;
   authNakPacket->length = htons(length);

   //The Message field is zero or more octets, and its contents are
   //implementation dependent
   authNakPacket->msgLength = 0;

   //Debug message
   TRACE_INFO("Sending PAP Authenticate-Nak packet (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump packet contents for debugging purpose
   pppDumpPacket((PppPacket *) authNakPacket, length, PPP_PROTOCOL_PAP);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, PPP_PROTOCOL_PAP);

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Password verification
 * @param[in] context PPP context
 * @param[in] password NULL-terminated string containing the password to be checked
 * @return TRUE if the password is valid, else FALSE
 **/

bool_t papCheckPassword(PppContext *context, const char_t *password)
{
   size_t n;
   bool_t status;

   //This flag tells whether the password is valid
   status = FALSE;

   //Retrieve the length of the password
   n = osStrlen(password);

   //Compare the length of the password against the expected value
   if(n == context->papFsm.passwordLen)
   {
      //Check whether the password is valid
      if(!osMemcmp(password, context->papFsm.password, n))
         status = TRUE;
   }

   //Return TRUE is the password is valid, else FALSE
   return status;
}

#endif
