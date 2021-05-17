/**
 * @file coap_client_block.c
 * @brief CoAP block-wise transfer
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
#include "coap/coap_client_block.h"
#include "debug.h"
#include "error.h"

//Check TCP/IP stack configuration
#if (COAP_CLIENT_SUPPORT == ENABLED && COAP_CLIENT_BLOCK_SUPPORT == ENABLED)


/**
 * @brief Set preferred block for transmission path
 * @param[in] request CoAP request handle
 * @param[in] blockSize Preferred block size, in bytes
 * @return Error code
 **/

error_t coapClientSetTxBlockSize(CoapClientRequest *request, uint_t blockSize)
{
   //Make sure the CoAP request handle is valid
   if(request == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the CoAP client context
   osAcquireMutex(&request->context->mutex);

   //Set TX block size
   if(blockSize == 16)
   {
      request->txBlockSzx = COAP_BLOCK_SIZE_16;
   }
   else if(blockSize == 32)
   {
      request->txBlockSzx = COAP_BLOCK_SIZE_32;
   }
   else if(blockSize == 64)
   {
      request->txBlockSzx = COAP_BLOCK_SIZE_64;
   }
   else if(blockSize == 128)
   {
      request->txBlockSzx = COAP_BLOCK_SIZE_128;
   }
   else if(blockSize == 256)
   {
      request->txBlockSzx = COAP_BLOCK_SIZE_256;
   }
   else if(blockSize == 512)
   {
      request->txBlockSzx = COAP_BLOCK_SIZE_512;
   }
   else
   {
      request->txBlockSzx = COAP_BLOCK_SIZE_1024;
   }

   //Ensure the block size is acceptable
   if(request->txBlockSzx > coapClientGetMaxBlockSize())
   {
      request->txBlockSzx = coapClientGetMaxBlockSize();
   }

   //Release exclusive access to the CoAP client context
   osReleaseMutex(&request->context->mutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set preferred block for reception path
 * @param[in] request CoAP request handle
 * @param[in] blockSize Preferred block size, in bytes
 * @return Error code
 **/

error_t coapClientSetRxBlockSize(CoapClientRequest *request, uint_t blockSize)
{
   //Make sure the CoAP request handle is valid
   if(request == NULL)
      return ERROR_INVALID_PARAMETER;

   //Acquire exclusive access to the CoAP client context
   osAcquireMutex(&request->context->mutex);

   //Set RX block size
   if(blockSize == 16)
   {
      request->rxBlockSzx = COAP_BLOCK_SIZE_16;
   }
   else if(blockSize == 32)
   {
      request->rxBlockSzx = COAP_BLOCK_SIZE_32;
   }
   else if(blockSize == 64)
   {
      request->rxBlockSzx = COAP_BLOCK_SIZE_64;
   }
   else if(blockSize == 128)
   {
      request->rxBlockSzx = COAP_BLOCK_SIZE_128;
   }
   else if(blockSize == 256)
   {
      request->rxBlockSzx = COAP_BLOCK_SIZE_256;
   }
   else if(blockSize == 512)
   {
      request->rxBlockSzx = COAP_BLOCK_SIZE_512;
   }
   else
   {
      request->rxBlockSzx = COAP_BLOCK_SIZE_1024;
   }

   //Ensure the block size is acceptable
   if(request->rxBlockSzx > coapClientGetMaxBlockSize())
   {
      request->rxBlockSzx = coapClientGetMaxBlockSize();
   }

   //Release exclusive access to the CoAP client context
   osReleaseMutex(&request->context->mutex);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Write resource body using block-wise mode
 * @param[in] request CoAP request handle
 * @param[in] data Pointer to a buffer containing the data to be transmitted
 * @param[in] length Number of bytes to be transmitted
 * @param[out] written Actual number of bytes written (optional parameter)
 * @param[in] last Flag indicating whether this message fragment is the last
 * @return Error code
 **/

error_t coapClientWriteBody(CoapClientRequest *request,
   const void *data, size_t length, size_t *written, bool_t last)
{
   error_t error;
   size_t n;
   uint32_t value;
   uint32_t blockPos;
   uint32_t blockSzx;
   size_t payloadLen;
   const uint8_t *payload;
   CoapMessage *requestMsg;
   CoapMessage *responseMsg;
   CoapCode responseCode;

   //Initialize status code
   error = NO_ERROR;

   //Total number of bytes that have been written
   if(written != NULL)
      *written = 0;

   //Block-wise transfers are realized as combinations of exchanges, each
   //of which is performed according to the CoAP base protocol
   while(length > 0 || last)
   {
      //Point to the request message
      requestMsg = coapClientGetRequestMessage(request);

      //Block1 option is used in descriptive usage in a request
      error = coapGetUintOption(requestMsg, COAP_OPT_BLOCK1, 0, &value);

      //Block1 option found?
      if(!error)
      {
         //Retrieve current block parameters
         blockPos = COAP_GET_BLOCK_POS(value);
         blockSzx = COAP_GET_BLOCK_SZX(value);
      }
      else
      {
         //Initialize block parameters
         blockPos = 0;
         blockSzx = request->txBlockSzx;
      }

      //Retrieve message payload length
      error = coapClientGetPayload(requestMsg, &payload, &payloadLen);
      //Any error to report?
      if(error)
         break;

      //Write as much data as possible
      if(length > 0 && payloadLen < COAP_GET_BLOCK_SIZE(blockSzx))
      {
         //Limit the number of data to copy at a time
         n = MIN(length, COAP_GET_BLOCK_SIZE(blockSzx) - payloadLen);

         //Write payload data
         error = coapClientWritePayload(requestMsg, data, n);
         //Any error to report?
         if(error)
            break;

         //Advance data pointer
         data = (uint8_t *) data + n;
         length -= n;

         //Total number of bytes that have been written
         if(written != NULL)
            *written += n;
      }
      else
      {
         //Block-wise transfer?
         if(blockPos > 0 || length > 0 || !last)
         {
            //The NUM field in the option value describes what block number
            //is contained in the payload of this message
            COAP_SET_BLOCK_NUM(value, blockPos >> (blockSzx + 4));

            //The M bit indicates whether further blocks need to be transferred
            //to complete the transfer of the body
            if(length == 0 && last)
            {
               COAP_SET_BLOCK_M(value, 0);
            }
            else
            {
               COAP_SET_BLOCK_M(value, 1);
            }

            //Set block size
            COAP_SET_BLOCK_SZX(value, blockSzx);

            //Block1 option is used in descriptive usage in a request
            error = coapClientSetUintOption(requestMsg, COAP_OPT_BLOCK1, 0, value);
            //Any error to report?
            if(error)
               break;
         }

         //Last block?
         if(length == 0 && last)
         {
            //Any preferred block size defined?
            if(request->rxBlockSzx < COAP_BLOCK_SIZE_RESERVED)
            {
               //Set preferred block size
               COAP_SET_BLOCK_NUM(value, 0);
               COAP_SET_BLOCK_M(value, 0);
               COAP_SET_BLOCK_SZX(value, request->rxBlockSzx);

               //Perform early negotiation
               error = coapClientSetUintOption(requestMsg, COAP_OPT_BLOCK2, 0, value);
               //Any error to report?
               if(error)
                  break;
            }
         }

         //Send request
         error = coapClientSendRequest(request, NULL, NULL);
         //Any error to report?
         if(error)
            break;

         //Point to the response message
         responseMsg = coapClientGetResponseMessage(request);

         //Retrieve response code
         error = coapClientGetResponseCode(responseMsg, &responseCode);
         //Any error to report?
         if(error)
            break;

         //Check response code
         if(COAP_GET_CODE_CLASS(responseCode) != COAP_CODE_CLASS_SUCCESS)
         {
            error = ERROR_INVALID_STATUS;
            break;
         }

         //Block-wise transfer?
         if(blockPos > 0 || length > 0 || !last)
         {
            //A Block1 option is used in control usage in a response
            error = coapClientGetUintOption(responseMsg, COAP_OPT_BLOCK1, 0, &value);
            //Any error to report?
            if(error)
               break;

            //The value 7 for SZX is reserved
            if(COAP_GET_BLOCK_SZX(value) >= COAP_BLOCK_SIZE_RESERVED)
            {
               error = ERROR_FAILURE;
               break;
            }

            //The NUM field of the Block1 option indicates what block number is
            //being acknowledged
            if(COAP_GET_BLOCK_POS(value) != blockPos)
            {
               error = ERROR_FAILURE;
               break;
            }

            //A server receiving a block-wise PUT or POST may want to indicate a
            //smaller block size preference (late negotiation)
            if(blockSzx > COAP_GET_BLOCK_SZX(value))
               blockSzx = COAP_GET_BLOCK_SZX(value);

            //Next block
            blockPos += COAP_GET_BLOCK_SIZE(blockSzx);

            //The NUM field in the option value describes what block number
            //is contained in the payload of this message
            COAP_SET_BLOCK_NUM(value, blockPos >> (blockSzx + 4));

            //Set block size
            COAP_SET_BLOCK_SZX(value, blockSzx);

            //Block1 option is used in descriptive usage in a request
            error = coapClientSetUintOption(requestMsg, COAP_OPT_BLOCK1, 0, value);
            //Any error to report?
            if(error)
               break;
         }

         //Trim the existing payload
         error = coapClientSetPayload(requestMsg, NULL, 0);
         //Any error to report?
         if(error)
            break;

         //Last block?
         if(length == 0 && last)
         {
            //Delete Block1 option
            error = coapClientDeleteOption(requestMsg, COAP_OPT_BLOCK1, 0);
            //We are done
            break;
         }
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Read resource body using block-wise mode
 * @param[in] request CoAP request handle
 * @param[out] data Buffer into which received data will be placed
 * @param[in] size Maximum number of bytes that can be received
 * @param[out] received Number of bytes that have been received
 * @return Error code
 **/

error_t coapClientReadBody(CoapClientRequest *request, void *data,
   size_t size, size_t *received)
{
   error_t error;
   size_t n;
   uint32_t value;
   uint32_t blockPos;
   uint32_t blockSzx;
   size_t payloadLen;
   const uint8_t *payload;
   CoapMessage *requestMsg;
   CoapMessage *responseMsg;
   CoapCode responseCode;

   //Initialize status code
   error = NO_ERROR;

   //Total number of bytes that have been received
   *received = 0;

   //Block-wise transfers are realized as combinations of exchanges, each
   //of which is performed according to the CoAP base protocol
   while(*received < size)
   {
      //Point to the response message
      responseMsg = coapClientGetResponseMessage(request);

      //Read payload data
      error = coapClientReadPayload(responseMsg, data, size - *received, &n);

      //Check status code
      if(error == NO_ERROR)
      {
         //Advance data pointer
         data = (uint8_t *) data + n;

         //Total number of bytes that have been received
         *received += n;
      }
      else if(error == ERROR_END_OF_STREAM)
      {
         //Point to the request message
         requestMsg = coapClientGetRequestMessage(request);

         //A Block2 Option is used in control usage in a request
         error = coapClientGetUintOption(requestMsg, COAP_OPT_BLOCK2, 0,
            &value);

         //Block2 option found?
         if(!error)
         {
            //Retrieve current block parameters
            blockPos = COAP_GET_BLOCK_POS(value);
            blockSzx = COAP_GET_BLOCK_SZX(value);
         }
         else
         {
            //Initialize block parameters
            blockPos = 0;
            blockSzx = request->rxBlockSzx;
         }

         //Block2 option is used in descriptive usage in a response
         error = coapClientGetUintOption(responseMsg, COAP_OPT_BLOCK2, 0,
            &value);

         //Block-wise transfer?
         if(!error)
         {
            //The value 7 for SZX is reserved
            if(COAP_GET_BLOCK_SZX(value) >= COAP_BLOCK_SIZE_RESERVED)
            {
               error = ERROR_FAILURE;
               break;
            }

            //The NUM field in the option value describes what block number
            //is contained in the payload of this message
            if(COAP_GET_BLOCK_POS(value) != blockPos)
            {
               //Report an error
               error = ERROR_FAILURE;
               break;
            }

            //The M bit indicates whether further blocks need to be
            //transferred to complete the transfer of that body
            if(!COAP_GET_BLOCK_M(value))
            {
               //Exit immediately
               error = ERROR_END_OF_STREAM;
               break;
            }

            //Retrieve the length of the payload
            error = coapClientGetPayload(responseMsg, &payload, &payloadLen);
            //Any error to report?
            if(error)
               break;

            //Make sure the length of the payload matches the block size
            if(payloadLen != COAP_GET_BLOCK_SIZE(value))
            {
               //Report an error
               error = ERROR_FAILURE;
               break;
            }

            //If the first request uses a bigger block size than the receiver
            //prefers, subsequent requests will use the preferred block size
            if(blockSzx > COAP_GET_BLOCK_SZX(value))
               blockSzx = COAP_GET_BLOCK_SZX(value);

            //Next block
            blockPos += COAP_GET_BLOCK_SIZE(value);

            //The NUM field in the Block2 option gives the block number of the
            //payload that is being requested to be returned in the response
            COAP_SET_BLOCK_NUM(value, blockPos >> (blockSzx + 4));

            //the M bit has no function and must be set to zero
            COAP_SET_BLOCK_M(value, 0);

            //The block size given suggests a block size (in the case of block
            //number 0) or repeats the block size of previous blocks received
            //(in the case of a non-zero block number)
            COAP_SET_BLOCK_SZX(value, blockSzx);

            //A Block2 Option is used in control usage in a request
            error = coapClientSetUintOption(requestMsg, COAP_OPT_BLOCK2, 0,
               value);
            //Any error to report?
            if(error)
               break;

            //Perform message exchange
            error = coapClientSendRequest(request, NULL, NULL);
            //Any error to report?
            if(error)
               break;

            //Point to the response message
            responseMsg = coapClientGetResponseMessage(request);

            //Retrieve response code
            error = coapClientGetResponseCode(responseMsg, &responseCode);
            //Any error to report?
            if(error)
               break;

            //Check response code
            if(COAP_GET_CODE_CLASS(responseCode) != COAP_CODE_CLASS_SUCCESS)
            {
               error = ERROR_INVALID_STATUS;
               break;
            }
         }
         else
         {
            //The Block2 option is not present in the response
            if(blockPos == 0)
            {
               error = ERROR_END_OF_STREAM;
            }
            else
            {
               error = ERROR_FAILURE;
            }

            //Exit immediately
            break;
         }
      }
      else
      {
         //Failed to read payload data
         break;
      }
   }

   //Any data received?
   if(*received > 0)
   {
      //Catch exception
      if(error == ERROR_END_OF_STREAM)
         error = NO_ERROR;
   }

   //Return status code
   return error;
}


/**
 * @brief Get maximum block size
 * @return Block size
 **/

CoapBlockSize coapClientGetMaxBlockSize(void)
{
   CoapBlockSize blockSize;

   //Retrieve maximum block size
#if (COAP_MAX_MSG_SIZE > 1024)
   blockSize = COAP_BLOCK_SIZE_1024;
#elif (COAP_MAX_MSG_SIZE > 512)
   blockSize = COAP_BLOCK_SIZE_512;
#elif (COAP_MAX_MSG_SIZE > 256)
   blockSize = COAP_BLOCK_SIZE_256;
#elif (COAP_MAX_MSG_SIZE > 128)
   blockSize = COAP_BLOCK_SIZE_128;
#elif (COAP_MAX_MSG_SIZE > 64)
   blockSize = COAP_BLOCK_SIZE_64;
#elif (COAP_MAX_MSG_SIZE > 32)
   blockSize = COAP_BLOCK_SIZE_32;
#else
   blockSize = COAP_BLOCK_SIZE_16;
#endif

   //Return maximum block size
   return blockSize;
}

#endif
