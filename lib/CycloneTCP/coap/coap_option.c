/**
 * @file coap_option.c
 * @brief CoAP option formatting and parsing
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
//#include "coap/coap_server.h"
#include "coap/coap_option.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (COAP_CLIENT_SUPPORT == ENABLED || COAP_SERVER_SUPPORT == ENABLED)


/**
 * @brief List of supported CoAP options
 **/

const CoapOptionParameters coapOptionList[] =
{
   {COAP_OPT_IF_MATCH,       TRUE,  FALSE, FALSE, TRUE,  "If-Match",       COAP_OPT_FORMAT_OPAQUE, 0, 8},
   {COAP_OPT_URI_HOST,       TRUE,  TRUE,  FALSE, FALSE, "Uri-Host",       COAP_OPT_FORMAT_STRING, 1, 255},
   {COAP_OPT_ETAG,           FALSE, FALSE, FALSE, TRUE,  "ETag",           COAP_OPT_FORMAT_OPAQUE, 1, 8},
   {COAP_OPT_IF_NONE_MATCH,  TRUE,  FALSE, FALSE, FALSE, "If-None-Match",  COAP_OPT_FORMAT_EMPTY,  0, 0},
   {COAP_OPT_OBSERVE,        FALSE, FALSE, FALSE, FALSE, "Observe",        COAP_OPT_FORMAT_UINT,   0, 3},
   {COAP_OPT_URI_PORT,       TRUE,  TRUE,  FALSE, FALSE, "Uri-Port",       COAP_OPT_FORMAT_UINT,   0, 2},
   {COAP_OPT_LOCATION_PATH,  FALSE, FALSE, FALSE, TRUE,  "Location-Path",  COAP_OPT_FORMAT_STRING, 0, 255},
   {COAP_OPT_URI_PATH,       TRUE,  TRUE,  FALSE, TRUE,  "Uri-Path",       COAP_OPT_FORMAT_STRING, 0, 255},
   {COAP_OPT_CONTENT_FORMAT, FALSE, FALSE, FALSE, FALSE, "Content-Format", COAP_OPT_FORMAT_UINT,   0, 2},
   {COAP_OPT_MAX_AGE,        FALSE, TRUE,  FALSE, FALSE, "Max-Age",        COAP_OPT_FORMAT_UINT,   0, 4},
   {COAP_OPT_URI_QUERY,      TRUE,  TRUE,  FALSE, TRUE,  "Uri-Query",      COAP_OPT_FORMAT_STRING, 0, 255},
   {COAP_OPT_ACCEPT,         TRUE,  FALSE, FALSE, FALSE, "Accept",         COAP_OPT_FORMAT_UINT,   0, 2},
   {COAP_OPT_LOCATION_QUERY, FALSE, FALSE, FALSE, TRUE,  "Location-Query", COAP_OPT_FORMAT_STRING, 0, 255},
   {COAP_OPT_BLOCK2,         TRUE,  TRUE,  FALSE, FALSE, "Block2",         COAP_OPT_FORMAT_UINT,   0, 3},
   {COAP_OPT_BLOCK1,         TRUE,  TRUE,  FALSE, FALSE, "Block1",         COAP_OPT_FORMAT_UINT,   0, 3},
   {COAP_OPT_SIZE2,          FALSE, FALSE, TRUE,  FALSE, "Size2",          COAP_OPT_FORMAT_UINT,   0, 4},
   {COAP_OPT_PROXY_URI,      TRUE,  TRUE,  FALSE, FALSE, "Proxy-Uri",      COAP_OPT_FORMAT_STRING, 1, 1034},
   {COAP_OPT_PROXY_SCHEME,   TRUE,  TRUE,  FALSE, FALSE, "Proxy-Scheme",   COAP_OPT_FORMAT_STRING, 1, 255},
   {COAP_OPT_SIZE1,          FALSE, FALSE, TRUE,  FALSE, "Size1",          COAP_OPT_FORMAT_UINT,   0, 4}
};


/**
 * @brief Parse the list of CoAP options
 * @param[in] p Input stream where to read the CoAP options
 * @param[in] length Number of bytes available in the input stream
 * @param[out] consumed Total number of bytes that have been consumed
 * @return Error code
 **/

error_t coapParseOptions(const uint8_t *p, size_t length, size_t *consumed)
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
 * @brief Parse CoAP option
 * @param[in] p Input stream where to read the CoAP option
 * @param[in] length Number of bytes available in the input stream
 * @param[in] prevOptionNum Option number of the previous instance
 * @param[out] option CoAP option content
 * @param[out] consumed Total number of bytes that have been consumed
 * @return Error code
 **/

error_t coapParseOption(const uint8_t *p, size_t length,
   uint16_t prevOptionNum, CoapOption *option, size_t *consumed)
{
   const CoapOptionHeader *header;

   //Malformed CoAP option?
   if(length < sizeof(CoapOptionHeader))
      return ERROR_BUFFER_UNDERFLOW;

   //Point to the CoAP option header
   header = (CoapOptionHeader *) p;

   //Point to the next field
   p += sizeof(CoapOptionHeader);
   length -= sizeof(CoapOptionHeader);

   //Check the value of the Option Delta field
   if(header->delta == COAP_OPT_DELTA_RESERVED)
   {
      //If the field is set to 15 but the entire byte is not the payload
      //marker, this must be processed as a message format error
      return ERROR_INVALID_MESSAGE;
   }
   else if(header->delta == COAP_OPT_DELTA_16_BITS)
   {
      //Malformed CoAP option?
      if(length < sizeof(uint16_t))
         return ERROR_BUFFER_UNDERFLOW;

      //A 16-bit unsigned integer in network byte order follows the initial
      //byte and indicates the Option Delta minus 269
      option->delta = LOAD16BE(p) + COAP_OPT_DELTA_MINUS_16_BITS;

      //Point to the next field
      p += sizeof(uint16_t);
      length -= sizeof(uint16_t);
   }
   else if(header->delta == COAP_OPT_DELTA_8_BITS)
   {
      //Malformed CoAP option?
      if(length < sizeof(uint8_t))
         return ERROR_BUFFER_UNDERFLOW;

      //An 8-bit unsigned integer follows the initial byte and indicates
      //the Option Delta minus 13
      option->delta = *p + COAP_OPT_DELTA_MINUS_8_BITS;

      //Point to the next field
      p += sizeof(uint8_t);
      length -= sizeof(uint8_t);
   }
   else
   {
      //A value between 0 and 12 indicates the Option Delta
      option->delta = header->delta;
   }

   //Check the value of the Option Length field
   if(header->length == COAP_OPT_LEN_RESERVED)
   {
      //If the field is set to this value, it must be processed as a
      //message format error
      return ERROR_INVALID_MESSAGE;
   }
   else if(header->length == COAP_OPT_LEN_16_BITS)
   {
      //Malformed CoAP option?
      if(length < sizeof(uint16_t))
         return ERROR_BUFFER_UNDERFLOW;

      //A 16-bit unsigned integer in network byte order precedes the
      //Option Value and indicates the Option Length minus 269
      option->length = LOAD16BE(p) + COAP_OPT_LEN_MINUS_16_BITS;

      //Point to the next field
      p += sizeof(uint16_t);
      length -= sizeof(uint16_t);
   }
   else if(header->length == COAP_OPT_LEN_8_BITS)
   {
      //Malformed CoAP option?
      if(length < sizeof(uint8_t))
         return ERROR_BUFFER_UNDERFLOW;

      //An 8-bit unsigned integer precedes the Option Value and indicates
      //the Option Length minus 13
      option->length = *p + COAP_OPT_LEN_MINUS_8_BITS;

      //Point to the next field
      p += sizeof(uint8_t);
      length -= sizeof(uint8_t);
   }
   else
   {
      //A value between 0 and 12 indicates the length of the Option Value,
      //in bytes
      option->length = header->length;
   }

   //Malformed CoAP option?
   if(length < option->length)
      return ERROR_BUFFER_UNDERFLOW;

   //Save option value
   option->value = p;
   //Advance data pointer
   p += option->length;

   //the Option Number for each instance is calculated as the sum of its delta
   //and the Option Number of the preceding instance in the message
   option->number = option->delta + prevOptionNum;

   //Total number of bytes that have been consumed
   *consumed = p - (uint8_t *) header;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Format CoAP option
 * @param[in] p Buffer where to format the CoAP option (optional parameter)
 * @param[in] prevOptionNum Option number of the previous instance
 * @param[in] option CoAP option content
 * @param[out] written Total number of bytes that have been written
 * @return Error code
 **/

error_t coapFormatOption(uint8_t *p, uint16_t prevOptionNum,
   CoapOption *option, size_t *written)
{
   size_t n;
   CoapOptionHeader *header;

   //The Option Delta is the difference between the Option Number of this
   //option and that of the previous option (or zero for the first option)
   option->delta = option->number - prevOptionNum;

   //Point to the buffer where to write the CoAP option
   header = (CoapOptionHeader *) p;
   //Length of the CoAP option
   n = sizeof(CoapOptionHeader);

   //Encode the Option Delta field
   if(option->delta >= COAP_OPT_DELTA_MINUS_16_BITS)
   {
      //The first parameter is optional
      if(p != NULL)
      {
         //Fix the initial byte of the CoAP option
         header->delta = COAP_OPT_DELTA_16_BITS;

         //A 16-bit unsigned integer in network byte order follows the
         //initial byte and indicates the Option Delta minus 269
         STORE16BE(option->delta - COAP_OPT_DELTA_MINUS_16_BITS, p + n);
      }

      //Adjust the length of the CoAP option
      n += sizeof(uint16_t);
   }
   else if(option->delta >= COAP_OPT_DELTA_MINUS_8_BITS)
   {
      //The first parameter is optional
      if(p != NULL)
      {
         //Fix the initial byte of the CoAP option
         header->delta = COAP_OPT_DELTA_8_BITS;

         //An 8-bit unsigned integer follows the initial byte and
         //indicates the Option Delta minus 13
         p[n] = (uint8_t) (option->delta - COAP_OPT_DELTA_MINUS_8_BITS);
      }

      //Adjust the length of the CoAP option
      n += sizeof(uint8_t);
   }
   else
   {
      //The first parameter is optional
      if(p != NULL)
      {
         //The Option Delta is directly encoded in the initial byte
         header->delta = (uint8_t) option->delta;
      }
   }

   //Encode the Option Length field
   if(option->length >= COAP_OPT_LEN_MINUS_16_BITS)
   {
      //The first parameter is optional
      if(p != NULL)
      {
         //Fix the initial byte of the CoAP option
         header->length = COAP_OPT_LEN_16_BITS;

         //A 16-bit unsigned integer in network byte order precedes the
         //Option Value and indicates the Option Length minus 269
         STORE16BE(option->length - COAP_OPT_LEN_MINUS_16_BITS, p + n);
      }

      //Adjust the length of the CoAP option
      n += sizeof(uint16_t);
   }
   else if(option->length >= COAP_OPT_LEN_MINUS_8_BITS)
   {
      //The first parameter is optional
      if(p != NULL)
      {
         //Fix the initial byte of the CoAP option
         header->length = COAP_OPT_LEN_8_BITS;

         //An 8-bit unsigned integer precedes the Option Value and
         //indicates the Option Length minus 13
         p[n] = (uint8_t) (option->length - COAP_OPT_LEN_MINUS_8_BITS);
      }

      //Adjust the length of the CoAP option
      n += sizeof(uint8_t);
   }
   else
   {
      //The first parameter is optional
      if(p != NULL)
      {
         //The Option Length is directly encoded in the initial byte
         header->length = (uint8_t) option->length;
      }
   }

   //The first parameter is optional
   if(p != NULL)
   {
      //The Option Value is a sequence of exactly Option Length bytes
      osMemmove(p + n, option->value, option->length);
   }

   //Total number of bytes that have been written
   *written = n + option->length;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Add an option to the specified CoAP message
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[in] optionValue Pointer to the first byte of the option value
 * @param[in] optionLen Length of the option, in bytes
 * @return Error code
 **/

error_t coapSetOption(CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex, const uint8_t *optionValue, size_t optionLen)
{
   error_t error;
   bool_t replace;
   size_t n;
   size_t m;
   size_t length;
   uint_t index;
   uint16_t prevOptionNum;
   uint8_t *p;
   CoapOption option;

   //Initialize variables
   replace = FALSE;
   index = 0;

   //Point to the first byte of the CoAP message
   p = message->buffer;
   //Retrieve the length of the message
   length = message->length;

   //Parse message header
   error = coapParseMessageHeader(p, length, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first option of the message
   p += n;
   //Number of bytes left to process
   length -= n;

   //For the first option in a message, a preceding option instance with
   //Option Number zero is assumed
   prevOptionNum = 0;

   //Loop through CoAP options
   while(length > 0)
   {
      //Payload marker found?
      if(*p == COAP_PAYLOAD_MARKER)
         break;

      //Parse current option
      error = coapParseOption(p, length, prevOptionNum, &option, &n);
      //Any error to report?
      if(error)
         return error;

      //Options are inserted in ascending order
      if(option.number > optionNum)
         break;

      //Matching option number?
      if(option.number == optionNum)
      {
         //Matching occurrence found?
         if(index++ == optionIndex)
         {
            //The current option will be replaced
            replace = TRUE;
            break;
         }
      }

      //Keep track of the current option number
      prevOptionNum = option.number;

      //Jump to the next option
      p += n;
      length -= n;
   }

   //Check whether the current option should be replaced
   if(replace)
   {
      //Remove the current occurrence of the option
      osMemmove(p, p + n, length - n);
      //Number of bytes left to process
      length -= n;
      //Adjust the length of the CoAP message
      message->length -= n;
   }

   //Each option instance in a message specifies the Option Number of the
   //defined CoAP option, the length of the Option Value, and the Option
   //Value itself
   option.number = optionNum;
   option.length = optionLen;
   option.value = optionValue;

   //The first pass calculates the required length
   error = coapFormatOption(NULL, prevOptionNum, &option, &n);
   //Any error to report?
   if(error)
      return error;

   //Make sure the output buffer is large enough to hold the new option
   if((message->length + n) > COAP_MAX_MSG_SIZE)
      return ERROR_BUFFER_OVERFLOW;

   //Make room for the new option
   osMemmove(p + n, p, length);

   //The second pass formats the CoAP option
   error = coapFormatOption(p, prevOptionNum, &option, &n);
   //Any error to report?
   if(error)
      return error;

   //Advance data pointer
   p += n;
   //Adjust the length of the CoAP message
   message->length += n;

   //Check whether another CoAP option is following
   if(length > 0 && *p != COAP_PAYLOAD_MARKER && !replace)
   {
      //Parse the following option
      error = coapParseOption(p, length, prevOptionNum, &option, &n);
      //Any error to report?
      if(error)
         return error;

      //Fix the Option Delta field
      error = coapFormatOption(p, optionNum, &option, &m);
      //Any error to report?
      if(error)
         return error;

      //Test if the length of the option has changed
      if(m < n)
      {
         //Move the rest of the CoAP message
         osMemmove(p + m, p + n, length - n);
         //Fix the length of the message
         message->length -= n - m;
      }
   }

   //Return status code
   return NO_ERROR;
}


/**
 * @brief Add a uint option to the specified CoAP message
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[in] optionValue Option value (unsigned integer)
 * @return Error code
 **/

error_t coapSetUintOption(CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex, uint32_t optionValue)
{
   size_t i;
   uint8_t buffer[4];

   //A sender should represent the integer with as few bytes as possible
   for(i = 4; optionValue != 0; i--)
   {
      buffer[i - 1] = optionValue & 0xFF;
      optionValue >>= 8;
   }

   //Add the specified option to the CoAP message
   return coapSetOption(message, optionNum, optionIndex, buffer + i, 4 - i);
}


/**
 * @brief Get the value of the specified option
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[out] optionValue Pointer to the first byte of the option value
 * @param[out] optionLen Length of the option, in bytes
 * @return Error code
 **/

error_t coapGetOption(const CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex, const uint8_t **optionValue, size_t *optionLen)
{
   error_t error;
   size_t n;
   size_t length;
   uint_t index;
   const uint8_t *p;
   CoapOption option;

   //Initialize index
   index = 0;

   //Point to the first byte of the CoAP message
   p = message->buffer;
   //Retrieve the length of the message
   length = message->length;

   //Parse message header
   error = coapParseMessageHeader(p, length, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first option of the message
   p += n;
   //Number of bytes left to process
   length -= n;

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
         return error;

      //Matching option number?
      if(option.number == optionNum)
      {
         //Matching occurrence found?
         if(index++ == optionIndex)
         {
            //Return option value
            *optionValue = option.value;
            *optionLen = option.length;

            //We are done
            return NO_ERROR;
         }
      }

      //Jump to the next option
      p += n;
      length -= n;
   }

   //The specified option number does not exist
   return ERROR_NOT_FOUND;
}


/**
 * @brief Get the value of the specified uint option
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number to search for
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @param[out] optionValue Option value (unsigned integer)
 * @return Error code
 **/

error_t coapGetUintOption(const CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex, uint32_t *optionValue)
{
   error_t error;
   size_t i;
   size_t n;
   const uint8_t *p;

   //Search the CoAP message for the specified option number
   error = coapGetOption(message, optionNum, optionIndex, &p, &n);
   //Any error to report ?
   if(error)
      return error;

   //Initialize integer value
   *optionValue = 0;

   //Convert the integer from network byte order
   for(i = 0; i < n; i++)
   {
      *optionValue <<= 8;
      *optionValue += p[i];
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Remove an option from the specified CoAP message
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number
 * @param[in] optionIndex Occurrence index (for repeatable options only)
 * @return Error code
 **/

error_t coapDeleteOption(CoapMessage *message, uint16_t optionNum,
   uint_t optionIndex)
{
   error_t error;
   bool_t found;
   size_t n;
   size_t m;
   size_t length;
   uint_t index;
   uint16_t prevOptionNum;
   uint8_t *p;
   CoapOption option;

   //Initialize variables
   found = FALSE;
   index = 0;

   //Point to the first byte of the CoAP message
   p = message->buffer;
   //Retrieve the length of the message
   length = message->length;

   //Parse message header
   error = coapParseMessageHeader(p, length, &n);
   //Any error to report?
   if(error)
      return error;

   //Point to the first option of the message
   p += n;
   //Number of bytes left to process
   length -= n;

   //For the first option in a message, a preceding option instance with
   //Option Number zero is assumed
   prevOptionNum = 0;

   //Loop through CoAP options
   while(length > 0)
   {
      //Payload marker found?
      if(*p == COAP_PAYLOAD_MARKER)
         break;

      //Parse current option
      error = coapParseOption(p, length, prevOptionNum, &option, &n);
      //Any error to report?
      if(error)
         return error;

      //Options are inserted in ascending order
      if(option.number > optionNum)
         break;

      //Matching option number?
      if(option.number == optionNum)
      {
         //Matching occurrence found?
         if(index++ == optionIndex)
         {
            //The current option will be removed
            found = TRUE;
            break;
         }
      }

      //Keep track of the current option number
      prevOptionNum = option.number;

      //Jump to the next option
      p += n;
      length -= n;
   }

   //Check whether the option has been found
   if(found)
   {
      //Remove the current occurrence of the option
      osMemmove(p, p + n, length - n);
      //Number of bytes left to process
      length -= n;
      //Adjust the length of the CoAP message
      message->length -= n;

      //Check whether another CoAP option is following
      if(length > 0 && *p != COAP_PAYLOAD_MARKER)
      {
         //Parse the following option
         error = coapParseOption(p, length, optionNum, &option, &n);
         //Any error to report?
         if(error)
            return error;

         //The first pass calculates the required length
         error = coapFormatOption(NULL, prevOptionNum, &option, &m);
         //Any error to report?
         if(error)
            return error;

         //Test if the length of the option has changed
         if(m > n)
         {
            //Move the rest of the CoAP message
            osMemmove(p + m - option.length, p + n - option.length,
               length + option.length - n);

            //Fix the value of the option
            option.value += m - n;
            //Fix the length of the message
            message->length += m - n;
         }

         //The second pass fixes the Option Delta field
         error = coapFormatOption(p, prevOptionNum, &option, &m);
         //Any error to report?
         if(error)
            return error;
      }
   }

   //Return status code
   return NO_ERROR;
}


/**
 * @brief Encode a path or query component into multiple repeatable options
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number
 * @param[in] optionValue Path or a query component to be encoded
 * @param[in] separator Delimiting character
 * @return Error code
 **/

error_t coapSplitRepeatableOption(CoapMessage *message, uint16_t optionNum,
   const char_t *optionValue, char_t separator)
{
   error_t error;
   size_t i;
   size_t j;
   uint_t index;

   //Initialize status code
   error = NO_ERROR;

   //Initialize variables
   i = 0;
   j = 0;
   index = 0;

   //Split the path or query component into multiple repeatable options
   do
   {
      //Delimiting character found?
      if(optionValue[i] == separator || optionValue[i] == '\0')
      {
         //Discard empty segments
         if((i - j) > 0)
         {
            //Each option specifies one segment of the component
            error = coapSetOption(message, optionNum, index++,
               (uint8_t *) optionValue + j, i - j);
            //Any error to report?
            if(error)
               break;
         }

         //Move to the next segment
         j = i + 1;
      }

      //Loop until the NULL character is reached
   } while(optionValue[i++] != '\0');

   //Return status code
   return error;
}


/**
 * @brief Decode a path or query component from multiple repeatable options
 * @param[in] message Pointer to the CoAP message
 * @param[in] optionNum Option number
 * @param[out] optionValue Buffer where to copy the path or query component
 * @param[in] maxLen Maximum number of characters the buffer can hold
 * @param[in] separator Delimiting character
 * @return Error code
 **/

error_t coapJoinRepeatableOption(const CoapMessage *message,
   uint16_t optionNum, char_t *optionValue, size_t maxLen, char_t separator)
{
   error_t error;
   size_t i;
   size_t n;
   uint_t index;
   const uint8_t *p;

   //Initialize status code
   error = NO_ERROR;

   //Initialize variables
   i = 0;
   index = 0;

   //Build path or query component
   while(!error)
   {
      //Each option specifies one segment of the component
      error = coapGetOption(message, optionNum, index++, &p, &n);

      //Check status code
      if(!error)
      {
         //Check separator
         if(separator != '&' || i != 0)
         {
            //Make sure the output buffer is large enough
            if(i < maxLen)
            {
               //Append a delimiting character
               optionValue[i++] = separator;
            }
         }

         //Make sure the output buffer is large enough
         if((i + n) <= maxLen)
         {
            //Copy option's value
            osMemcpy(optionValue + i, p, n);
            //Update the length of the absolute path
            i += n;
         }
         else
         {
            //Report an error
            error = ERROR_BUFFER_OVERFLOW;
         }
      }
   }

   //Check status code
   if(error == ERROR_NOT_FOUND)
   {
      //Catch exception
      error = NO_ERROR;
   }

   //Properly terminate the string with a NULL character
   optionValue[i] = '\0';

   //Return status code
   return error;
}


/**
 * @brief Retrieve parameters for a given option number
 * @param[in] optionNum Option number
 * @return Option parameters
 **/

const CoapOptionParameters *coapGetOptionParameters(uint16_t optionNum)
{
   uint_t i;
   const CoapOptionParameters *optionParams;

   //Initialize variable
   optionParams = NULL;

   //Loop through the list of supported options
   for(i = 0; i < arraysize(coapOptionList); i++)
   {
      //Check option number against the expected value
      if(coapOptionList[i].number == optionNum)
      {
         optionParams = &coapOptionList[i];
         break;
      }
   }

   //Return option parameters
   return optionParams;
}

#endif
