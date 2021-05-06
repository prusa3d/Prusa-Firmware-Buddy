/**
 * @file web_socket_frame.c
 * @brief WebSocket frame parsing and formatting
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
#define TRACE_LEVEL WEB_SOCKET_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "web_socket/web_socket.h"
#include "web_socket/web_socket_frame.h"
#include "web_socket/web_socket_transport.h"
#include "web_socket/web_socket_misc.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (WEB_SOCKET_SUPPORT == ENABLED)


/**
 * @brief Format WebSocket frame header
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] fin FIN flag
 * @param[in] type Frame type
 * @param[in] payloadLen Length of the payload data
 * @return Error code
 **/

error_t webSocketFormatFrameHeader(WebSocket *webSocket,
   bool_t fin, WebSocketFrameType type, size_t payloadLen)
{
   error_t error;
   WebSocketFrameContext *txContext;
   WebSocketFrame *frame;

   //Point to the TX context
   txContext = &webSocket->txContext;

   //Flush the transmit buffer
   txContext->bufferPos = 0;
   txContext->bufferLen = 0;

   //The endpoint must encapsulate the data in a WebSocket frame
   frame = (WebSocketFrame *) txContext->buffer;

   //The frame needs to be formatted according to the WebSocket framing
   //format
   frame->fin = fin;
   frame->reserved = 0;
   frame->opcode = type;

   //All frames sent from the client to the server are masked by a 32-bit
   //value that is contained within the frame
   if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
   {
      //All frames sent from client to server have the Mask bit set to 1
      frame->mask = TRUE;

      //Make sure that the RNG callback function has been registered
      if(webSockRandCallback != NULL)
      {
         //Generate a random masking key
         error = webSockRandCallback(txContext->maskingKey, sizeof(uint32_t));
         //Any error to report?
         if(error)
            return error;
      }
      else
      {
         //A cryptographically strong random number generator
         //must be used to generate the masking key
         return ERROR_PRNG_NOT_READY;
      }
   }
   else
   {
      //Clear the Mask bit
      frame->mask = FALSE;
   }

   //Size of the frame header
   txContext->bufferLen = sizeof(WebSocketFrame);

   //Compute the number of application data to be transmitted
   txContext->payloadLen = payloadLen;

   //Check the length of the payload
   if(payloadLen <= 125)
   {
      //Payload length
      frame->payloadLen = payloadLen;
   }
   else if(payloadLen <= 65535)
   {
      //If the Payload Length field is set to 126, then the following
      //2 bytes are interpreted as a 16-bit unsigned integer
      frame->payloadLen = 126;

      //Save the length of the payload data
      STORE16BE(payloadLen, frame->extPayloadLen);

      //Adjust the length of the frame header
      txContext->bufferLen += sizeof(uint16_t);
   }
   else
   {
      //If the Payload Length field is set to 127, then the following
      //8 bytes are interpreted as a 64-bit unsigned integer
      frame->payloadLen = 127;

      //Save the length of the payload data
      STORE64BE(payloadLen, frame->extPayloadLen);

      //Adjust the length of the frame header
      txContext->bufferLen += sizeof(uint64_t);
   }

   //Debug message
   TRACE_DEBUG("WebSocket: Sending frame\r\n");
   TRACE_DEBUG("  FIN = %u\r\n", frame->fin);
   TRACE_DEBUG("  Reserved = %u\r\n", frame->reserved);
   TRACE_DEBUG("  Opcode = %u\r\n", frame->opcode);
   TRACE_DEBUG("  Mask = %u\r\n", frame->mask);
   TRACE_DEBUG("  Payload Length = %u\r\n", txContext->payloadLen);

   //The Masking Key field is present the mask bit is set to 1
   if(frame->mask)
   {
      //Debug message
      TRACE_DEBUG_ARRAY("  Masking Key = ", txContext->maskingKey, sizeof(uint32_t));

      //Copy the masking key
      osMemcpy(txContext->buffer + txContext->bufferLen,
         txContext->maskingKey, sizeof(uint32_t));

      //Adjust the length of the frame header
      txContext->bufferLen += sizeof(uint32_t);
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Parse WebSocket frame header
 * @param[in] webSocket Handle to a WebSocket
 * @param[in] frame Pointer to the WebSocket frame header
 * @param[out] type Frame type
 * @return Error code
 **/

error_t webSocketParseFrameHeader(WebSocket *webSocket,
   const WebSocketFrame *frame, WebSocketFrameType *type)
{
   size_t j;
   size_t k;
   size_t n;
   uint16_t statusCode;
   WebSocketFrameContext *rxContext;

   //Point to the RX context
   rxContext = &webSocket->rxContext;

   //Point to the Extended Payload Length
   n = sizeof(WebSocketFrame);

   //Check frame type
   if(type != NULL)
   {
      if(*type != WS_FRAME_TYPE_CONTINUATION)
      {
         if(frame->opcode != WS_FRAME_TYPE_CONTINUATION &&
            frame->opcode != *type)
         {
            return ERROR_UNEXPECTED_MESSAGE;
         }
      }
   }

   //Check the Payload Length field
   if(frame->payloadLen == 126)
   {
      //If the Payload Length field is set to 126, then the following
      //2 bytes are interpreted as a 16-bit unsigned integer
      rxContext->payloadLen = LOAD16BE(frame->extPayloadLen);

      //Point to the next field
      n += sizeof(uint16_t);
   }
   else if(frame->payloadLen == 127)
   {
      //If the Payload Length field is set to 127, then the following
      //8 bytes are interpreted as a 64-bit unsigned integer
      rxContext->payloadLen = LOAD64BE(frame->extPayloadLen);

      //Point to the next field
      n += sizeof(uint64_t);
   }
   else
   {
      //Retrieve the length of the payload data
      rxContext->payloadLen = frame->payloadLen;
   }

   //Debug message
   TRACE_DEBUG("WebSocket: frame received...\r\n");
   TRACE_DEBUG("  FIN = %u\r\n", frame->fin);
   TRACE_DEBUG("  Reserved = %u\r\n", frame->reserved);
   TRACE_DEBUG("  Opcode = %u\r\n", frame->opcode);
   TRACE_DEBUG("  Mask = %u\r\n", frame->mask);
   TRACE_DEBUG("  Payload Length = %u\r\n", rxContext->payloadLen);

   //Check whether the payload data is masked
   if(frame->mask)
   {
      //Save the masking key
      osMemcpy(rxContext->maskingKey, (uint8_t *) frame + n, sizeof(uint32_t));
      //Debug message
      TRACE_DEBUG_ARRAY("  Masking Key = ", rxContext->maskingKey, sizeof(uint32_t));

      //Point to the payload data
      n += sizeof(uint32_t);
   }

   //Text or Close frame received?
   if(frame->opcode == WS_FRAME_TYPE_TEXT ||
      frame->opcode == WS_FRAME_TYPE_CLOSE)
   {
      //Reinitialize UTF-8 decoding context
      webSocket->utf8Context.utf8CharSize = 0;
      webSocket->utf8Context.utf8CharIndex = 0;
      webSocket->utf8Context.utf8CodePoint = 0;
   }

   //If the RSV field is a nonzero value and none of the negotiated extensions
   //defines the meaning of such a nonzero value, the receiving endpoint must
   //fail the WebSocket connection
   if(frame->reserved != 0)
   {
      //Report a protocol error
      webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
      //Terminate the WebSocket connection
      return ERROR_INVALID_FRAME;
   }

   //The Opcode field defines the interpretation of the payload data
   if(frame->opcode == WS_FRAME_TYPE_CONTINUATION)
   {
      //A Continuation frame cannot be the first frame of a fragmented message
      if(rxContext->fin)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;

      rxContext->controlFrameType = WS_FRAME_TYPE_CONTINUATION;
   }
   else if(frame->opcode == WS_FRAME_TYPE_TEXT)
   {
      //The Opcode must be 0 in subsequent fragmented frames
      if(!rxContext->fin)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;

      //Save the Opcode field
      rxContext->dataFrameType = WS_FRAME_TYPE_TEXT;
      rxContext->controlFrameType = WS_FRAME_TYPE_CONTINUATION;
   }
   else if(frame->opcode == WS_FRAME_TYPE_BINARY)
   {
      //The Opcode must be 0 in subsequent fragmented frames
      if(!rxContext->fin)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;

      //Save the Opcode field
      rxContext->dataFrameType = WS_FRAME_TYPE_BINARY;
      rxContext->controlFrameType = WS_FRAME_TYPE_CONTINUATION;
   }
   else if(frame->opcode == WS_FRAME_TYPE_CLOSE)
   {
      //Check the length of the payload data
      if(rxContext->payloadLen == 0)
      {
         //The Close frame does not contain any body
         webSocket->statusCode = WS_STATUS_CODE_NORMAL_CLOSURE;
      }
      else if(rxContext->payloadLen == 1)
      {
         //Report a protocol error
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
      }
      else
      {
         //All frames sent from the client to the server are masked
         if(frame->mask)
         {
            //Unmask the data
            for(j = 0; j < rxContext->payloadLen; j++)
            {
               //Index of the masking key to be applied
               k = j % 4;
               //Convert masked data into unmasked data
               *((uint8_t *) frame + n + j) ^= rxContext->maskingKey[k];
            }
         }

         //If there is a body, the first two bytes of the body must be
         //a 2-byte unsigned integer representing a status code
         statusCode = LOAD16BE((uint8_t *) frame + n);

         //Debug message
         TRACE_DEBUG("  Status Code = %u\r\n", statusCode);

         //When sending a Close frame in response, the endpoint typically
         //echos the status code it received
         if(webSocketCheckStatusCode(statusCode))
            webSocket->statusCode = statusCode;
         else
            webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;

         //The body may contain UTF-8-encoded data
         if(rxContext->payloadLen > 2)
         {
            //Compute the length of the data
            k = rxContext->payloadLen - 2;

            //Invalid UTF-8 sequence?
            if(!webSocketCheckUtf8Stream(&webSocket->utf8Context,
               (uint8_t *) frame + n + 2, k, k))
            {
               //The received data is not consistent with the type of the message
               webSocket->statusCode = WS_STATUS_CODE_INVALID_PAYLOAD_DATA;
            }
         }
      }

      //A Close frame has been received
      webSocket->handshakeContext.closingFrameReceived = TRUE;
      //Exit immediately
      return ERROR_END_OF_STREAM;
   }
   else if(frame->opcode == WS_FRAME_TYPE_PING)
   {
      //Save the Opcode field
      rxContext->controlFrameType = WS_FRAME_TYPE_PING;

      //Control frames must not be fragmented
      if(!frame->fin)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;

      //All control frames must have a payload length of 125 bytes or less
      if(frame->payloadLen > 125)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
   }
   else if(frame->opcode == WS_FRAME_TYPE_PONG)
   {
      //Save the Opcode field
      rxContext->controlFrameType = WS_FRAME_TYPE_PONG;

      //Control frames must not be fragmented
      if(!frame->fin)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;

      //All control frames must have a payload length of 125 bytes or less
      if(frame->payloadLen > 125)
         webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
   }
   else
   {
      //If an unknown opcode is received, the receiving endpoint must fail
      //the WebSocket connection
      webSocket->statusCode = WS_STATUS_CODE_PROTOCOL_ERROR;
   }

   //Save the Mask flag
   rxContext->mask = frame->mask;

   //Control frame?
   if(rxContext->controlFrameType != WS_FRAME_TYPE_CONTINUATION)
   {
      //Return frame type
      if(type != NULL)
         *type = rxContext->controlFrameType;
   }
   else
   {
      //Save the FIN flag
      rxContext->fin = frame->fin;

      //Return frame type
      if(type != NULL)
         *type = rxContext->dataFrameType;
   }

   //Check status code
   if(webSocket->statusCode == WS_STATUS_CODE_NO_STATUS_RCVD)
      return NO_ERROR;
   else if(webSocket->statusCode == WS_STATUS_CODE_NORMAL_CLOSURE)
      return ERROR_END_OF_STREAM;
   else if(webSocket->statusCode == WS_STATUS_CODE_PROTOCOL_ERROR)
      return ERROR_INVALID_FRAME;
   else
      return ERROR_FAILURE;
}


/**
 * @brief Format a Close frame
 * @param[in] webSocket Handle to a WebSocket
 * @return Error code
 **/

error_t webSocketFormatCloseFrame(WebSocket *webSocket)
{
   error_t error;
   uint8_t *p;

   //Format Close frame
   error = webSocketFormatFrameHeader(webSocket,
      TRUE, WS_FRAME_TYPE_CLOSE, sizeof(uint16_t));

   //Check status code
   if(!error)
   {
      //1005 is a reserved value and must not be set as a status code in
      //a Close control frame by an endpoint
      if(webSocket->statusCode == WS_STATUS_CODE_NO_STATUS_RCVD)
         webSocket->statusCode = WS_STATUS_CODE_NORMAL_CLOSURE;

      //Debug message
      TRACE_DEBUG("  Status Code = %u\r\n", webSocket->statusCode);

      //Point to end of the WebSocket frame header
      p = webSocket->txContext.buffer + webSocket->txContext.bufferLen;

      //Write status code
      p[0] = MSB(webSocket->statusCode);
      p[1] = LSB(webSocket->statusCode);

      //All frames sent from the client to the server are masked
      if(webSocket->endpoint == WS_ENDPOINT_CLIENT)
      {
         //Apply masking
         p[0] ^= webSocket->txContext.maskingKey[0];
         p[1] ^= webSocket->txContext.maskingKey[1];
      }

      //Adjust the length of the frame
      webSocket->txContext.bufferLen += sizeof(uint16_t);
   }

   //Return status code
   return error;
}

#endif
