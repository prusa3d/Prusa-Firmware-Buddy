/**
 * @file modbus_server_security.c
 * @brief Modbus/TCP security layer
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
#define TRACE_LEVEL MODBUS_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "modbus/modbus_server.h"
#include "modbus/modbus_server_security.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (MODBUS_SERVER_SUPPORT == ENABLED && MODBUS_SERVER_TLS_SUPPORT == ENABLED)

//Modbus Role OID (1.3.6.1.4.1.50316.802.1)
const uint8_t MODBUS_ROLE_OID[11] = {0x2B, 0x06, 0x01, 0x04, 0x01, 0x83, 0x89, 0x0C, 0x86, 0x22, 0x01};


/**
 * @brief Parse client's certificate
 * @param[in] tlsContext Pointer to the TLS context
 * @param[in] certInfo Pointer to the X.509 certificate
 * @param[in] pathLen Certificate path length
 * @param[in] param Handle referencing a Modbus/TCP client connection
 * @return Error code
 **/

error_t modbusServerParseCertificate(TlsContext *tlsContext,
   const X509CertificateInfo *certInfo, uint_t pathLen, void *param)
{
   error_t error;
   size_t n;
   size_t length;
   const uint8_t *data;
   ModbusClientConnection *connection;
   X509Extension extension;

   //Point to the client connection
   connection = (ModbusClientConnection *) param;

   //End-user certificate?
   if(pathLen == 0)
   {
      //The X.509 v3 certificate format also allows communities to define
      //private extensions to carry information unique to those communities
      data = certInfo->tbsCert.extensions.rawData;
      length = certInfo->tbsCert.extensions.rawDataLen;

      //Loop through the extensions
      while(length > 0)
      {
         //Each extension includes an OID and a value
         error = x509ParseExtension(data, length, &n, &extension);
         //Any error to report?
         if(error)
            return error;

         //Role OID extension found?
         if(!oidComp(extension.oid, extension.oidLen, MODBUS_ROLE_OID,
            sizeof(MODBUS_ROLE_OID)))
         {
            //Extract the client role OID from the certificate
            error = modbusServerParseRoleOid(connection, extension.value,
               extension.valueLen);
            //Any error to report?
            if(error)
               return error;
         }

         //Next extension
         data += n;
         length -= n;
      }
   }

   //Upon receipt of a certificate chain from the remote peer, the TLS end point
   //will verify each certificate signature using the next CA certificate in the
   //chain until it can verify the root of the chain
   return ERROR_UNKNOWN_CA;
}


/**
 * @brief Parse client role OID
 * @param[in] connection Pointer to the client connection
 * @param[in] data Pointer to the ASN.1 structure to parse
 * @param[in] length Length of the ASN.1 structure
 * @return Error code
 **/

error_t modbusServerParseRoleOid(ModbusClientConnection *connection,
   const uint8_t *data, size_t length)
{
   error_t error;
   Asn1Tag tag;

   //The Role extension must be a valid UTF-8 string
   error = asn1ReadTag(data, length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Enforce encoding, class and type
   error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL,
      ASN1_TYPE_UTF8_STRING);
   //Invalid tag?
   if(error)
      return error;

   //Extract the client role OID from the certificate
   if(tag.length <= MODBUS_SERVER_MAX_ROLE_LEN)
   {
      //Copy client role
      osMemcpy(connection->role, tag.value, tag.length);
      //Properly terminate the string with a NULL character
      connection->role[tag.length] = '\0';
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Open secure connection
 * @param[in] context Pointer to the Modbus/TCP server context
 * @param[in] connection Pointer to the client connection
 * @return Error code
 **/

error_t modbusServerOpenSecureConnection(ModbusServerContext *context,
   ModbusClientConnection *connection)
{
   error_t error;

   //Allocate TLS context
   connection->tlsContext = tlsInit();
   //Failed to allocate TLS context?
   if(connection->tlsContext == NULL)
      return ERROR_OPEN_FAILED;

   //Select server operation mode
   error = tlsSetConnectionEnd(connection->tlsContext,
      TLS_CONNECTION_END_SERVER);
   //Any error to report?
   if(error)
      return error;

   //Bind TLS to the relevant socket
   error = tlsSetSocket(connection->tlsContext, connection->socket);
   //Any error to report?
   if(error)
      return error;

   //Set TX and RX buffer size
   error = tlsSetBufferSize(connection->tlsContext,
      MODBUS_SERVER_TLS_TX_BUFFER_SIZE, MODBUS_SERVER_TLS_RX_BUFFER_SIZE);
   //Any error to report?
   if(error)
      return error;

   //Register certificate verification callback
   error = tlsSetCertificateVerifyCallback(connection->tlsContext,
      modbusServerParseCertificate, connection);
   //Any error to report?
   if(error)
      return error;

#if (TLS_TICKET_SUPPORT == ENABLED)
   //Enable session ticket mechanism
   error = tlsSetTicketCallbacks(connection->tlsContext, tlsEncryptTicket,
      tlsDecryptTicket, &context->tlsTicketContext);
   //Any error to report?
   if(error)
      return error;
#endif

   //Invoke user-defined callback, if any
   if(context->settings.tlsInitCallback != NULL)
   {
      //Perform TLS related initialization
      error = context->settings.tlsInitCallback(connection,
         connection->tlsContext);
      //Any error to report?
      if(error)
         return error;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Establish secure connection
 * @param[in] connection Pointer to the client connection
 * @return Error code
 **/

error_t modbusServerEstablishSecureConnection(ModbusClientConnection *connection)
{
   //Establish a TLS connection
   return tlsConnect(connection->tlsContext);
}

#endif
