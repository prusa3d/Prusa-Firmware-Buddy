/**
 * @file coap_client_observe.c
 * @brief CoAP observe
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
#define TRACE_LEVEL COAP_TRACE_LEVEL

//Dependencies
#include <stdlib.h>
#include "core/net.h"
#include "coap/coap_client.h"
#include "coap/coap_client_observe.h"
#include "coap/coap_client_misc.h"
#include "debug.h"
#include "error.h"

//Check TCP/IP stack configuration
#if (COAP_CLIENT_SUPPORT == ENABLED && COAP_CLIENT_OBSERVE_SUPPORT == ENABLED)


/**
 * @brief Process notification response
 * @param[in] request CoAP request handle
 * @param[in] response Pointer to the response message
 * @return Error code
 **/

error_t coapClientProcessNotification(CoapClientRequest *request,
   const CoapMessage *response)
{
   error_t error;
   uint32_t value;
   systime_t time;

   //The only difference between a notification and a normal response
   //is the presence of the Observe option
   error = coapGetUintOption(response, COAP_OPT_OBSERVE, 0, &value);

   //Observe option included in the response?
   if(!error)
   {
      //Get current time
      time = osGetSystemTime();

      //Check the order of arrival for incoming notification
      if(coapClientCheckSequenceNumber(request, value, time))
      {
         //Save the value of the Observe option
         request->observeSeqNum = value;
         //Save the time at which the notification was received
         request->retransmitStartTime = time;

         //Search the CoAP response for a Max-Age option
         error = coapGetUintOption(response, COAP_OPT_MAX_AGE, 0, &value);

         //Max-Age option not included in the response?
         if(error)
         {
            //A default value of 60 seconds is assumed in the absence of the
            //option in a response
            value = COAP_DEFAULT_MAX_AGE;
         }

         //A notification is considered fresh while its age is not greater
         //than the value indicated by the Max-Age option
         request->retransmitTimeout = value * 1000;

         //Additionally, the client should at least wait for a random amount
         //of time between 5 and 15 seconds after Max-Age expired to reduce
         //collisions with other clients (refer to RFC 7641, section 3.3.1)
         request->retransmitTimeout += netGetRandRange(COAP_CLIENT_RAND_DELAY_MIN,
            COAP_CLIENT_RAND_DELAY_MAX);

         //The user is notified of changes to the resource state
         error = coapClientChangeRequestState(request,
            COAP_REQ_STATE_OBSERVE);
      }
      else
      {
         //Drop the incoming notification...
         TRACE_INFO("Out-of-order notification received!\r\n");
      }
   }
   else
   {
      //The client is notified of changes to the resource state
      coapClientChangeRequestState(request, COAP_REQ_STATE_OBSERVE);

      //The observation has been canceled
      error = coapClientChangeRequestState(request, COAP_REQ_STATE_CANCELED);
   }

   //Return status code
   return error;
}


/**
 * @brief Check the order of arrival for incoming notification
 * @param[in] request CoAP request handle
 * @param[in] v2 Value of the Observe option in the incoming notification
 * @param[in] t2 Client-local timestamp for the incoming notification
 * @return TRUE if the incoming notification was sent more recently than the
 *   freshest notification so far. Otherwise FALSE is returned
 **/

bool_t coapClientCheckSequenceNumber(CoapClientRequest *request,
   uint32_t v2, systime_t t2)
{
   uint32_t v1;
   systime_t t1;
   bool_t valid;

   //Check current state
   if(request->state != COAP_REQ_STATE_OBSERVE)
   {
      //The first notification has been received
      valid = TRUE;
   }
   else
   {
      //Value of the Observe option in the freshest notification so far
      v1 = request->observeSeqNum;
      //Client-local timestamp for the freshest notification so far
      t1 = request->retransmitStartTime;

      //An incoming notification was sent more recently than the freshest
      //notification so far when one of the following conditions is met
      if(v1 < v2 && (v2 - v1) < 0x00800000)
      {
         //V1 is less than V2 in 24-bit serial number arithmetic
         valid = TRUE;
      }
      else if(v1 > v2 && (v1 - v2) > 0x00800000)
      {
         //V1 is less than V2 in 24-bit serial number arithmetic
         valid = TRUE;
      }
      else if(timeCompare(t2, t1 + 128000) > 0)
      {
         //After 128 seconds have elapsed without any notification, a client
         //does not need to check the sequence numbers to assume that an
         //incoming notification was sent more recently than the freshest
         //notification it has received so far
         valid = TRUE;
      }
      else
      {
         //An out-of-order sequence number has been detected
         valid = FALSE;
      }
   }

   //Return TRUE if the incoming notification was sent more recently than the
   //freshest notification so far
   return valid;
}

#endif
