/**
 * @file chap.c
 * @brief CHAP (Challenge Handshake Authentication Protocol)
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
#include "ppp/chap.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (PPP_SUPPORT == ENABLED && CHAP_SUPPORT == ENABLED)

//Additional dependencies
#include "core/crypto.h"
#include "hash/md5.h"


/**
 * @brief Start CHAP authentication
 * @param[in] context PPP context
 * @return Error code
 **/

error_t chapStartAuth(PppContext *context)
{
   //Debug message
   TRACE_INFO("\r\nStarting CHAP authentication...\r\n");

   //Check whether the other end of the PPP link is being authenticated
   if(context->localConfig.authProtocol == PPP_PROTOCOL_CHAP)
   {
      //Initialize restart counter
      context->chapFsm.restartCounter = CHAP_MAX_CHALLENGES;
      //Send a Challenge packet
      chapSendChallenge(context);
      //Switch to the Challenge-Sent state
      context->chapFsm.localState = CHAP_STATE_2_CHALLENGE_SENT;
   }

   //Check whether the other end of the PPP link is the authenticator
   if(context->peerConfig.authProtocol == PPP_PROTOCOL_CHAP)
   {
      //Switch to the Started state
      context->chapFsm.peerState = CHAP_STATE_1_STARTED;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Abort CHAP authentication
 * @param[in] context PPP context
 * @return Error code
 **/

error_t chapAbortAuth(PppContext *context)
{
   //Debug message
   TRACE_INFO("\r\nAborting CHAP authentication...\r\n");

   //Abort CHAP authentication process
   context->chapFsm.localState = CHAP_STATE_0_INITIAL;
   context->chapFsm.peerState = CHAP_STATE_0_INITIAL;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief CHAP timer handler
 * @param[in] context PPP context
 **/

void chapTick(PppContext *context)
{
   //Check whether the restart timer is running
   if(context->chapFsm.localState == CHAP_STATE_2_CHALLENGE_SENT)
   {
      //Get current time
      systime_t time = osGetSystemTime();

      //Check restart timer
      if((time - context->chapFsm.timestamp) >= CHAP_RESTART_TIMER)
      {
         //Debug message
         TRACE_INFO("\r\nCHAP Timeout event\r\n");

         //Check whether the restart counter is greater than zero
         if(context->chapFsm.restartCounter > 0)
         {
            //Retransmit the Challenge packet
            chapSendChallenge(context);
         }
         else
         {
            //Abort CHAP authentication
            context->chapFsm.localState = CHAP_STATE_0_INITIAL;
            //Authentication failed
            lcpClose(context);
         }
      }
   }
}


/**
 * @brief Process an incoming CHAP packet
 * @param[in] context PPP context
 * @param[in] packet CHAP packet received from the peer
 * @param[in] length Length of the packet, in bytes
 **/

void chapProcessPacket(PppContext *context,
   const PppPacket *packet, size_t length)
{
   //Ensure the length of the incoming CHAP packet is valid
   if(length < sizeof(PppPacket))
      return;

   //Check the length field
   if(ntohs(packet->length) > length)
      return;
   if(ntohs(packet->length) < sizeof(PppPacket))
      return;

   //Save the length of the CHAP packet
   length = ntohs(packet->length);

   //Debug message
   TRACE_INFO("CHAP packet received (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump CHAP packet contents for debugging purpose
   pppDumpPacket(packet, length, PPP_PROTOCOL_CHAP);

   //CHAP is done at initial link establishment, and could also be
   //requested after link establishment
   if(context->pppPhase != PPP_PHASE_AUTHENTICATE &&
      context->pppPhase != PPP_PHASE_NETWORK)
   {
      //Any packets received during any other phase must be silently discarded
      return;
   }

   //Check CHAP code field
   switch(packet->code)
   {
   //Challenge packet?
   case CHAP_CODE_CHALLENGE:
      //Process Challenge packet
      chapProcessChallenge(context, (ChapChallengePacket *) packet, length);
      break;
   //Response packet?
   case CHAP_CODE_RESPONSE:
      //Process Response packet
      chapProcessResponse(context, (ChapResponsePacket *) packet, length);
      break;
   //Success packet?
   case CHAP_CODE_SUCCESS:
      //Process Success packet
      chapProcessSuccess(context, (ChapSuccessPacket *) packet, length);
      break;
   //Failure packet?
   case CHAP_CODE_FAILURE:
      //Process Failure packet
      chapProcessFailure(context, (ChapFailurePacket *) packet, length);
      break;
   //Unknown code field
   default:
      //Silently drop the incoming packet
      break;
   }
}


/**
 * @brief Process Challenge packet
 * @param[in] context PPP context
 * @param[in] challengePacket Packet received from the peer
 * @param[in] length Length of the packet, in bytes
 * @return Error code
 **/

error_t chapProcessChallenge(PppContext *context,
   const ChapChallengePacket *challengePacket, size_t length)
{
   size_t n;
   Md5Context md5Context;

   //Debug message
   TRACE_INFO("\r\nCHAP Challenge packet received\r\n");

   //Make sure the Challenge packet is acceptable
   if(context->peerConfig.authProtocol != PPP_PROTOCOL_CHAP)
      return ERROR_FAILURE;

   //Check the length of the packet
   if(length < sizeof(ChapChallengePacket))
      return ERROR_INVALID_LENGTH;

   //Malformed Challenge packet?
   if(length < (sizeof(ChapChallengePacket) + challengePacket->valueSize))
      return ERROR_INVALID_LENGTH;

   //Save the Identifier field
   context->chapFsm.peerIdentifier = challengePacket->identifier;

   //Retrieve the length of the password
   n = osStrlen(context->password);

   //The response value is the one-way hash calculated over a stream
   //of octets consisting of the identifier, followed by the secret,
   //followed by the challenge value
   md5Init(&md5Context);
   md5Update(&md5Context, &challengePacket->identifier, sizeof(uint8_t));
   md5Update(&md5Context, context->password, n);
   md5Update(&md5Context, challengePacket->value, challengePacket->valueSize);
   md5Final(&md5Context, NULL);

   //Whenever a Challenge packet is received, the peer must send a Response packet
   chapSendResponse(context, md5Context.digest);

   //Switch to the Response-Sent state
   context->chapFsm.peerState = CHAP_STATE_4_RESPONSE_SENT;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Response packet
 * @param[in] context PPP context
 * @param[in] responsePacket Packet received from the peer
 * @param[in] length Length of the packet, in bytes
 * @return Error code
 **/

error_t chapProcessResponse(PppContext *context,
   const ChapResponsePacket *responsePacket, size_t length)
{
   bool_t status;
   const uint8_t *p;

   //Debug message
   TRACE_INFO("\r\nCHAP Response packet received\r\n");

   //Make sure the Response packet is acceptable
   if(context->localConfig.authProtocol != PPP_PROTOCOL_CHAP)
      return ERROR_FAILURE;

   //Check the length of the packet
   if(length < sizeof(ChapResponsePacket))
      return ERROR_INVALID_LENGTH;

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(responsePacket->identifier != context->chapFsm.localIdentifier)
      return ERROR_WRONG_IDENTIFIER;

   //Malformed Response packet?
   if(length < (sizeof(ChapResponsePacket) + responsePacket->valueSize))
      return ERROR_INVALID_LENGTH;

   //The length of the response value depends upon the hash algorithm used
   if(responsePacket->valueSize != MD5_DIGEST_SIZE)
      return ERROR_INVALID_LENGTH;

   //Retrieve the response value
   context->chapFsm.response = responsePacket->value;

   //Point to the Name field
   p = responsePacket->value + responsePacket->valueSize;
   //Retrieve the length of the Name field
   length -= sizeof(ChapResponsePacket) + responsePacket->valueSize;

   //Limit the length of the string
   length = MIN(length, PPP_MAX_USERNAME_LEN);
   //Copy the name of the peer to be identified
   osMemcpy(context->peerName, p, length);
   //Properly terminate the string with a NULL character
   context->peerName[length] = '\0';

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

   //Whenever a Response packet is received, the authenticator compares the
   //Response Value with its own calculation of the expected value. Based on
   //this comparison, the authenticator must send a Success or Failure packet
   if(status)
   {
      //Send a Success packet
      chapSendSuccess(context);

      //Switch to the Success-Sent state
      context->chapFsm.localState = CHAP_STATE_6_SUCCESS_SENT;
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
      //Send a Failure packet
      chapSendFailure(context);

      //Switch to the Failure-Sent state
      context->chapFsm.localState = CHAP_STATE_8_FAILURE_SENT;
      //The authenticator should take action to terminate the link
      lcpClose(context);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Process Success packet
 * @param[in] context PPP context
 * @param[in] successPacket Packet received from the peer
 * @param[in] length Length of the packet, in bytes
 * @return Error code
 **/

error_t chapProcessSuccess(PppContext *context,
   const ChapSuccessPacket *successPacket, size_t length)
{
   //Debug message
   TRACE_INFO("\r\nCHAP Success packet received\r\n");

   //Make sure the Success packet is acceptable
   if(context->peerConfig.authProtocol != PPP_PROTOCOL_CHAP)
      return ERROR_FAILURE;

   //Check the length of the packet
   if(length < sizeof(ChapSuccessPacket))
      return ERROR_INVALID_LENGTH;

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(successPacket->identifier != context->chapFsm.peerIdentifier)
      return ERROR_WRONG_IDENTIFIER;

   //Switch to the Success-Rcvd state
   context->chapFsm.peerState = CHAP_STATE_7_SUCCESS_RCVD;
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
 * @brief Process Failure packet
 * @param[in] context PPP context
 * @param[in] failurePacket Packet received from the peer
 * @param[in] length Length of the packet, in bytes
 * @return Error code
 **/

error_t chapProcessFailure(PppContext *context,
   const ChapFailurePacket *failurePacket, size_t length)
{
   //Debug message
   TRACE_INFO("\r\nCHAP Failure packet received\r\n");

   //Make sure the Failure packet is acceptable
   if(context->peerConfig.authProtocol != PPP_PROTOCOL_CHAP)
      return ERROR_FAILURE;

   //Check the length of the packet
   if(length < sizeof(ChapFailurePacket))
      return ERROR_INVALID_LENGTH;

   //When a packet is received with an invalid Identifier field, the
   //packet is silently discarded without affecting the automaton
   if(failurePacket->identifier != context->chapFsm.peerIdentifier)
      return ERROR_WRONG_IDENTIFIER;

   //Switch to the Failure-Rcvd state
   context->chapFsm.peerState = CHAP_STATE_9_FAILURE_RCVD;
   //Authentication failed
   lcpClose(context);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Send Challenge packet
 * @param[in] context PPP context
 * @return Error code
 **/

error_t chapSendChallenge(PppContext *context)
{
   error_t error;
   size_t n;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   ChapChallengePacket *challengePacket;

   //Retrieve the length of the username
   n = osStrlen(context->username);
   //Calculate the length of the Challenge packet
   length = sizeof(ChapChallengePacket) + MD5_DIGEST_SIZE + n;

   //Allocate a buffer memory to hold the Challenge packet
   buffer = pppAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Challenge packet
   challengePacket = netBufferAt(buffer, offset);

   //Format packet header
   challengePacket->code = CHAP_CODE_CHALLENGE;
   challengePacket->identifier = ++context->chapFsm.localIdentifier;
   challengePacket->length = htons(length);
   challengePacket->valueSize = MD5_DIGEST_SIZE;

   //Make sure that the callback function has been registered
   if(context->settings.randCallback != NULL)
   {
      //Generate a random challenge value
      error = context->settings.randCallback(
         context->chapFsm.challenge, MD5_DIGEST_SIZE);
   }
   else
   {
      //Report an error
      error = ERROR_FAILURE;
   }

   //Check status code
   if(!error)
   {
      //Copy the challenge value
      osMemcpy(challengePacket->value, context->chapFsm.challenge, MD5_DIGEST_SIZE);

      //The Name field is one or more octets representing the
      //identification of the system transmitting the packet
      osMemcpy(challengePacket->value + MD5_DIGEST_SIZE, context->username, n);

      //Debug message
      TRACE_INFO("Sending CHAP Challenge packet (%" PRIuSIZE " bytes)...\r\n", length);
      //Dump packet contents for debugging purpose
      pppDumpPacket((PppPacket *) challengePacket, length, PPP_PROTOCOL_CHAP);

      //Send PPP frame
      error = pppSendFrame(context->interface, buffer, offset, PPP_PROTOCOL_CHAP);

      //The restart counter is decremented each time a Challenge packet is sent
      if(context->chapFsm.restartCounter > 0)
         context->chapFsm.restartCounter--;

      //Save the time at which the packet was sent
      context->chapFsm.timestamp = osGetSystemTime();
   }

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send Response packet
 * @param[in] context PPP context
 * @param[in] value Response value
 * @return Error code
 **/

error_t chapSendResponse(PppContext *context, const uint8_t *value)
{
   error_t error;
   size_t n;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   ChapResponsePacket *responsePacket;

   //Retrieve the length of the username
   n = osStrlen(context->username);
   //Calculate the length of the Response packet
   length = sizeof(ChapResponsePacket) + MD5_DIGEST_SIZE + n;

   //Allocate a buffer memory to hold the Response packet
   buffer = pppAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Response packet
   responsePacket = netBufferAt(buffer, offset);

   //Format packet header
   responsePacket->code = CHAP_CODE_RESPONSE;
   responsePacket->identifier = context->chapFsm.peerIdentifier;
   responsePacket->length = htons(length);
   responsePacket->valueSize = MD5_DIGEST_SIZE;

   //Copy the Response value
   osMemcpy(responsePacket->value, value, MD5_DIGEST_SIZE);

   //The Name field is one or more octets representing the
   //identification of the system transmitting the packet
   osMemcpy(responsePacket->value + MD5_DIGEST_SIZE, context->username, n);

   //Debug message
   TRACE_INFO("Sending CHAP Response packet (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump packet contents for debugging purpose
   pppDumpPacket((PppPacket *) responsePacket, length, PPP_PROTOCOL_CHAP);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, PPP_PROTOCOL_CHAP);

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send Success packet
 * @param[in] context PPP context
 * @return Error code
 **/

error_t chapSendSuccess(PppContext *context)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PppPacket *successPacket;

   //Retrieve the length of the Success packet
   length = sizeof(PppPacket);

   //Allocate a buffer memory to hold the Success packet
   buffer = pppAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Success packet
   successPacket = netBufferAt(buffer, offset);

   //Format packet header
   successPacket->code = CHAP_CODE_SUCCESS;
   successPacket->identifier = context->chapFsm.localIdentifier;
   successPacket->length = htons(length);

   //Debug message
   TRACE_INFO("Sending CHAP Success packet (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump packet contents for debugging purpose
   pppDumpPacket((PppPacket *) successPacket, length, PPP_PROTOCOL_CHAP);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, PPP_PROTOCOL_CHAP);

   //Free previously allocated memory block
   netBufferFree(buffer);
   //Return status code
   return error;
}


/**
 * @brief Send Failure packet
 * @param[in] context PPP context
 * @return Error code
 **/

error_t chapSendFailure(PppContext *context)
{
   error_t error;
   size_t length;
   size_t offset;
   NetBuffer *buffer;
   PppPacket *failurePacket;

   //Retrieve the length of the Failure packet
   length = sizeof(PppPacket);

   //Allocate a buffer memory to hold the Failure packet
   buffer = pppAllocBuffer(length, &offset);
   //Failed to allocate memory?
   if(buffer == NULL)
      return ERROR_OUT_OF_MEMORY;

   //Point to the Failure packet
   failurePacket = netBufferAt(buffer, offset);

   //Format packet header
   failurePacket->code = CHAP_CODE_FAILURE;
   failurePacket->identifier = context->chapFsm.localIdentifier;
   failurePacket->length = htons(length);

   //Debug message
   TRACE_INFO("Sending CHAP Failure packet (%" PRIuSIZE " bytes)...\r\n", length);
   //Dump packet contents for debugging purpose
   pppDumpPacket((PppPacket *) failurePacket, length, PPP_PROTOCOL_CHAP);

   //Send PPP frame
   error = pppSendFrame(context->interface, buffer, offset, PPP_PROTOCOL_CHAP);

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

bool_t chapCheckPassword(PppContext *context, const char_t *password)
{
   size_t n;
   Md5Context md5Context;

   //Retrieve the length of the password
   n = osStrlen(password);

   //The response value is the one-way hash calculated over a stream
   //of octets consisting of the identifier, followed by the secret,
   //followed by the challenge value
   md5Init(&md5Context);
   md5Update(&md5Context, &context->chapFsm.localIdentifier, sizeof(uint8_t));
   md5Update(&md5Context, password, n);
   md5Update(&md5Context, context->chapFsm.challenge, MD5_DIGEST_SIZE);
   md5Final(&md5Context, NULL);

   //Check the resulting digest value
   if(!osMemcmp(md5Context.digest, context->chapFsm.response, MD5_DIGEST_SIZE))
      return TRUE;
   else
      return FALSE;
}

#endif
