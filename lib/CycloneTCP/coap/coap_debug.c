/**
 * @file coap_debug.c
 * @brief Data logging functions for debugging purpose (CoAP)
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
#include "core/net.h"
#include "coap/coap_client.h"
#include "coap/coap_server.h"
#include "coap/coap_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (COAP_CLIENT_SUPPORT == ENABLED || COAP_SERVER_SUPPORT == ENABLED)

//CoAP message types
const CoapParamName coapTypeList[] =
{
   {COAP_TYPE_CON, "CON"},
   {COAP_TYPE_NON, "NON"},
   {COAP_TYPE_ACK, "ACK"},
   {COAP_TYPE_RST, "RST"}
};

//CoAP method and response codes
const CoapParamName coapCodeList[] =
{
   {COAP_CODE_EMPTY,                      "Empty"},
   {COAP_CODE_GET,                        "GET"},
   {COAP_CODE_POST,                       "POST"},
   {COAP_CODE_PUT,                        "PUT"},
   {COAP_CODE_DELETE,                     "DELETE"},
   {COAP_CODE_FETCH,                      "FETCH"},
   {COAP_CODE_PATCH,                      "PATCH"},
   {COAP_CODE_IPATCH,                     "iPATCH"},
   {COAP_CODE_CREATED,                    "Created"},
   {COAP_CODE_DELETED,                    "Deleted"},
   {COAP_CODE_VALID,                      "Valid"},
   {COAP_CODE_CHANGED,                    "Changed"},
   {COAP_CODE_CONTENT,                    "Content"},
   {COAP_CODE_CONTINUE,                   "Continue"},
   {COAP_CODE_BAD_REQUEST,                "Bad Request"},
   {COAP_CODE_UNAUTHOZED,                 "Unauthorized"},
   {COAP_CODE_BAD_OPTION,                 "Bad Option"},
   {COAP_CODE_FORBIDDEN,                  "Forbidden"},
   {COAP_CODE_NOT_FOUND,                  "Not Found"},
   {COAP_CODE_METHOD_NOT_ALLOWED,         "Method Not Allowed"},
   {COAP_CODE_NOT_ACCEPTABLE,             "Not Acceptable"},
   {COAP_CODE_REQUEST_ENTITY_INCOMPLETE,  "Request Entity Incomplete"},
   {COAP_CODE_CONFLICT,                   "Conflict"},
   {COAP_CODE_PRECONDITION_FAILED,        "Precondition Failed"},
   {COAP_CODE_REQUEST_ENTITY_TO_LARGE,    "Request Entity Too Large"},
   {COAP_CODE_UNSUPPORTED_CONTENT_FORMAT, "Unsupported Content-Format"},
   {COAP_CODE_UNPROCESSABLE_ENTITY,       "Unprocessable Entity"},
   {COAP_CODE_INTERNAL_SERVER,            "Internal Server Error"},
   {COAP_CODE_NOT_IMPLEMENTED,            "Not Implemented"},
   {COAP_CODE_BAD_GATEWAY,                "Bad Gateway"},
   {COAP_CODE_SERVICE_UNAVAILABLE,        "Service Unavailable"},
   {COAP_CODE_GATEWAY_TIMEOUT,            "Gateway Timeout"},
   {COAP_CODE_PROXYING_NOT_SUPPORTED,     "Proxying Not Supported"},
   {COAP_CODE_CSM,                        "CSM"},
   {COAP_CODE_PING,                       "Ping"},
   {COAP_CODE_PONG,                       "Pong"},
   {COAP_CODE_RELEASE,                    "Release"},
   {COAP_CODE_ABORT,                      "Abort"}
};

//Observe option values
const CoapParamName coapObserveList[] =
{
   {COAP_OBSERVE_REGISTER,   "Register"},
   {COAP_OBSERVE_DEREGISTER, "Deregister"}
};

//Content-Format option values
const CoapParamName coapContentFormatList[] =
{
   {COAP_CONTENT_FORMAT_TEXT_PLAIN,       "text/plain"},
   {COAP_CONTENT_FORMAT_APP_LINK_FORMAT,  "application/link-format"},
   {COAP_CONTENT_FORMAT_APP_XML,          "application/xml"},
   {COAP_CONTENT_FORMAT_APP_OCTET_STREAM, "application/octet-stream"},
   {COAP_CONTENT_FORMAT_APP_EXI,          "application/exi"},
   {COAP_CONTENT_FORMAT_APP_JSON,         "application/json"}
};


/**
 * @brief Dump CoAP message for debugging purpose
 * @param[in] message Pointer to the CoAP message
 * @param[in] length Length of the message, in bytes
 * @return Error code
 **/

error_t coapDumpMessage(const void *message, size_t length)
{
#if (COAP_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   error_t error;
   size_t n;
   const uint8_t *p;

   //Point to the first byte of the CoAP message
   p = message;

   //Dump message header
   error = coapDumpMessageHeader(p, length, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first option of the message
   p += n;
   //Number of bytes left to process
   length -= n;

   //Parse the list of options
   error = coapDumpOptions(p, length, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the payload
   p += n;
   //Number of bytes left to process
   length -= n;

   //The payload is optional
   if(length > 0)
   {
      //The payload is prefixed by a fixed, one-byte payload marker
      p++;
      //Retrieve the length of the payload
      length--;

      //Dump payload
      TRACE_DEBUG("  Payload (%" PRIuSIZE " bytes)\r\n", length);
      TRACE_DEBUG_ARRAY("    ", p, length);
      //TRACE_DEBUG("    %s\r\n", p);
   }
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump CoAP message header
 * @param[in] p Input stream where to read the CoAP message header
 * @param[in] length Number of bytes available in the input stream
 * @param[out] consumed Total number of bytes that have been consumed
 * @return Error code
 **/

error_t coapDumpMessageHeader(const uint8_t *p, size_t length, size_t *consumed)
{
   CoapMessageHeader *header;

   //Malformed CoAP message?
   if(length < sizeof(CoapMessageHeader))
      return ERROR_INVALID_MESSAGE;

   //Point to the CoAP message header
   header = (CoapMessageHeader *) p;

   //Dump CoAP version number
   TRACE_DEBUG("  Version = %" PRIu8 "\r\n", header->version);

   //Dump message type
   TRACE_DEBUG("  Type = %" PRIu8 " (%s)\r\n", header->type,
      coapGetParamName(header->type, coapTypeList, arraysize(coapTypeList)));

   //Dump token length
   TRACE_DEBUG("  Token Length = %" PRIu8 "\r\n", header->tokenLen);

   //Dump method or response code
   TRACE_DEBUG("  Code = %" PRIu8 ".%02" PRIu8 " (%s)\r\n",
      header->code / 32, header->code & 31,
      coapGetParamName(header->code, coapCodeList, arraysize(coapCodeList)));

   //Dump message identifier
   TRACE_DEBUG("  Message ID = 0x%04" PRIX16 "\r\n", ntohs(header->mid));

   //Malformed CoAP message?
   if(length < (sizeof(CoapMessageHeader) + header->tokenLen))
      return ERROR_INVALID_LENGTH;

   //Dump token
   TRACE_DEBUG_ARRAY("  Token = ", header->token, header->tokenLen);

   //Total number of bytes that have been consumed
   *consumed = sizeof(CoapMessageHeader) + header->tokenLen;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Dump the list of CoAP options
 * @param[in] p Input stream where to read the CoAP options
 * @param[in] length Number of bytes available in the input stream
 * @param[out] consumed Total number of bytes that have been consumed
 * @return Error code
 **/

error_t coapDumpOptions(const uint8_t *p, size_t length, size_t *consumed)
{
   error_t error;
   size_t n;
   CoapOption option;

   //Initialize status code
   error = NO_ERROR;

   //Total number of bytes that have been consumed
   *consumed = 0;

   //For the first option in a message, a preceding option instance with
   //Option Number zero is assumed
   option.number = 0;

   //Loop through CoAP options
   while(length > 0)
   {
      //Payload marker found?
      if(*p == COAP_PAYLOAD_MARKER)
         break;

      //Parse current option
      error = coapParseOption(p, length, option.number, &option, &n);
      //Any error to report?
      if(error)
         break;

      //Dump current option
      error = coapDumpOption(&option);
      //Any error to report?
      if(error)
         break;

      //Total number of bytes that have been consumed
      *consumed += n;

      //Jump to the next option
      p += n;
      length -= n;
   }

   //Return status code
   return error;
}


/**
 * @brief Dump CoAP option
 * @param[out] option CoAP option content
 * @return Error code
 **/

error_t coapDumpOption(const CoapOption *option)
{
#if (COAP_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)
   size_t i;
   uint32_t value;
   const char_t *name;
   const CoapOptionParameters *optionParams;

   //Retrieve option parameters
   optionParams = coapGetOptionParameters(option->number);

   //Check whether the specified option is supported
   if(optionParams != NULL)
   {
      //Dump option number
      TRACE_DEBUG("  Option %" PRIu16 " (%s)\r\n",
         option->number, optionParams->name);

      //Check option type
      if(optionParams->format == COAP_OPT_FORMAT_UINT)
      {
         //Initialize integer value
         value = 0;

         //Convert the integer from network byte order
         for(i = 0; i < option->length; i++)
         {
            value = (value << 8) + option->value[i];
         }

         //Check option number
         if(option->number == COAP_OPT_OBSERVE)
         {
            //Convert the Observe option to string representation
            name = coapGetParamName(value, coapObserveList,
               arraysize(coapObserveList));

            //Dump the value of the option
            if(osStrcmp(name, "Unknown"))
            {
               TRACE_DEBUG("    %" PRIu32 " (%s)\r\n", value, name);
            }
            else
            {
               TRACE_DEBUG("    %" PRIu32 "\r\n", value);
            }
         }
         else if(option->number == COAP_OPT_CONTENT_FORMAT)
         {
            //Convert the Content-Format option to string representation
            name = coapGetParamName(value, coapContentFormatList,
               arraysize(coapContentFormatList));

            //Dump the value of the option
            TRACE_DEBUG("    %" PRIu32 " (%s)\r\n", value, name);
         }
         else if(option->number == COAP_OPT_BLOCK1 ||
            option->number == COAP_OPT_BLOCK2)
         {
            //Dump the value of the Block option
            TRACE_DEBUG("    %" PRIu32 "/%" PRIu32 "/%" PRIu32 "\r\n",
               COAP_GET_BLOCK_NUM(value), COAP_GET_BLOCK_M(value),
               16 << COAP_GET_BLOCK_SZX(value));
         }
         else
         {
            //Dump the value of the option
            TRACE_DEBUG("    %" PRIu32 "\r\n", value);
         }
      }
      else if(optionParams->format == COAP_OPT_FORMAT_STRING)
      {
         //Append prefix
         TRACE_DEBUG("    \"");

         //Dump string value
         for(i = 0; i < option->length; i++)
            TRACE_DEBUG("%c", option->value[i]);

         //Add a line feed
         TRACE_DEBUG("\"\r\n");
      }
      else
      {
         //Dump option value
         TRACE_DEBUG_ARRAY("    ", option->value, option->length);
      }
   }
   else
   {
      //Dump option number
      TRACE_DEBUG("  Option %" PRIu16 " (%" PRIu16 " bytes)\r\n",
         option->number, option->length);

      //Dump option value
      TRACE_DEBUG_ARRAY("    ", option->value, option->length);
   }
#endif

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Convert a parameter to string representation
 * @param[in] value Parameter value
 * @param[in] paramList List of acceptable parameters
 * @param[in] paramListLen Number of entries in the list
 * @return NULL-terminated string describing the parameter
 **/

const char_t *coapGetParamName(uint_t value,
   const CoapParamName *paramList, size_t paramListLen)
{
   uint_t i;

   //Default name for unknown values
   static const char_t defaultName[] = "Unknown";

   //Loop through the list of acceptable parameters
   for(i = 0; i < paramListLen; i++)
   {
      if(paramList[i].value == value)
         return paramList[i].name;
   }

   //Unknown value
   return defaultName;
}

#endif
