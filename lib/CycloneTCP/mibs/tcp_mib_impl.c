/**
 * @file tcp_mib_impl.c
 * @brief TCP MIB module implementation
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
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "mibs/mib_common.h"
#include "mibs/tcp_mib_module.h"
#include "mibs/tcp_mib_impl.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (TCP_MIB_SUPPORT == ENABLED && TCP_SUPPORT == ENABLED)


/**
 * @brief TCP MIB module initialization
 * @return Error code
 **/

error_t tcpMibInit(void)
{
   //Debug message
   TRACE_INFO("Initializing TCP-MIB base...\r\n");

   //Clear TCP MIB base
   osMemset(&tcpMibBase, 0, sizeof(tcpMibBase));

   //tcpRtoAlgorithm object
   tcpMibBase.tcpRtoAlgorithm = TCP_MIB_RTO_ALGORITHM_VANJ;
   //tcpRtoMin object
   tcpMibBase.tcpRtoMin = TCP_MIN_RTO;
   //tcpRtoMax object
   tcpMibBase.tcpRtoMax = TCP_MAX_RTO;
   //tcpMaxConn object
   tcpMibBase.tcpMaxConn = SOCKET_MAX_COUNT;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get tcpCurrEstab object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t tcpMibGetTcpCurrEstab(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   uint_t i;
   Socket *socket;

   //Initialize object value
   value->gauge32 = 0;

   //Loop through socket descriptors
   for(i = 0; i < SOCKET_MAX_COUNT; i++)
   {
      //Point to current socket
      socket = &socketTable[i];

      //TCP socket?
      if(socket->type == SOCKET_TYPE_STREAM)
      {
         //Check current state
         if(socket->state == TCP_STATE_ESTABLISHED ||
            socket->state == TCP_STATE_CLOSE_WAIT)
         {
            //Number of TCP connections for which the current state
            //is either ESTABLISHED or CLOSE-WAIT
            value->gauge32++;
         }
      }
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set tcpConnectionEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t tcpMibSetTcpConnectionEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
   //Not implemented
   return ERROR_WRITE_FAILED;
}


/**
 * @brief Get tcpConnectionEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t tcpMibGetTcpConnectionEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   uint_t i;
   size_t n;
   IpAddr localIpAddr;
   uint16_t localPort;
   IpAddr remoteIpAddr;
   uint16_t remotePort;
   Socket *socket;

   //Point to the instance identifier
   n = object->oidLen;

   //tcpConnectionLocalAddressType and tcpConnectionLocalAddress are used
   //as 1st and 2nd instance identifiers
   error = mibDecodeIpAddr(oid, oidLen, &n, &localIpAddr);
   //Invalid instance identifier?
   if(error)
      return error;

   //tcpConnectionLocalPort is used as 3rd instance identifier
   error = mibDecodePort(oid, oidLen, &n, &localPort);
   //Invalid instance identifier?
   if(error)
      return error;

   //tcpConnectionRemAddressType and tcpConnectionRemAddress are used
   //as 4th and 5th instance identifiers
   error = mibDecodeIpAddr(oid, oidLen, &n, &remoteIpAddr);
   //Invalid instance identifier?
   if(error)
      return error;

   //tcpConnectionRemPort is used as 6th instance identifier
   error = mibDecodePort(oid, oidLen, &n, &remotePort);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Loop through socket descriptors
   for(i = 0; i < SOCKET_MAX_COUNT; i++)
   {
      //Point to current socket
      socket = &socketTable[i];

      //TCP socket?
      if(socket->type == SOCKET_TYPE_STREAM)
      {
         //Check local IP address
         if(!ipCompAddr(&socket->localIpAddr, &localIpAddr))
            continue;
         //Check local port number
         if(socket->localPort != localPort)
            continue;
         //Check remote IP address
         if(!ipCompAddr(&socket->remoteIpAddr, &remoteIpAddr))
            continue;
         //Check local port number
         if(socket->remotePort != remotePort)
            continue;
         //Check current state
         if(socket->state == TCP_STATE_LISTEN)
            continue;

         //A matching socket has been found
         break;
      }
   }

   //No matching connection found in socket table?
   if(i >= SOCKET_MAX_COUNT)
      return ERROR_INSTANCE_NOT_FOUND;

   //tcpConnectionState object?
   if(!osStrcmp(object->name, "tcpConnectionState"))
   {
      //Get object value
      switch(socket->state)
      {
      case TCP_STATE_CLOSED:
         value->integer = TCP_MIB_CONN_STATE_CLOSED;
         break;
      case TCP_STATE_LISTEN:
         value->integer = TCP_MIB_CONN_STATE_LISTEN;
         break;
      case TCP_STATE_SYN_SENT:
         value->integer = TCP_MIB_CONN_STATE_SYN_SENT;
         break;
      case TCP_STATE_SYN_RECEIVED:
         value->integer = TCP_MIB_CONN_STATE_SYN_RECEIVED;
         break;
      case TCP_STATE_ESTABLISHED:
         value->integer = TCP_MIB_CONN_STATE_ESTABLISHED;
         break;
      case TCP_STATE_FIN_WAIT_1:
         value->integer = TCP_MIB_CONN_STATE_FIN_WAIT_1;
         break;
      case TCP_STATE_FIN_WAIT_2:
         value->integer = TCP_MIB_CONN_STATE_FIN_WAIT_2;
         break;
      case TCP_STATE_CLOSE_WAIT:
         value->integer = TCP_MIB_CONN_STATE_CLOSE_WAIT;
         break;
      case TCP_STATE_LAST_ACK:
         value->integer = TCP_MIB_CONN_STATE_LAST_ACK;
         break;
      case TCP_STATE_CLOSING:
         value->integer = TCP_MIB_CONN_STATE_CLOSING;
         break;
      case TCP_STATE_TIME_WAIT:
         value->integer = TCP_MIB_CONN_STATE_TIME_WAIT;
         break;
      default:
         value->integer = 0;
         break;
      }
   }
   //tcpConnectionProcess object?
   else if(!osStrcmp(object->name, "tcpConnectionProcess"))
   {
      //ID of the process associated with this connection
      value->unsigned32 = 0;
   }
   //Unknown object?
   else
   {
      //The specified object does not exist
      error = ERROR_OBJECT_NOT_FOUND;
   }

   //Return status code
   return error;
}


/**
 * @brief Get next tcpConnectionEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t tcpMibGetNextTcpConnectionEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   uint_t i;
   size_t n;
   bool_t acceptable;
   IpAddr localIpAddr;
   uint16_t localPort;
   IpAddr remoteIpAddr;
   uint16_t remotePort;
   Socket *socket;

   //Initialize variables
   localIpAddr = IP_ADDR_ANY;
   localPort = 0;
   remoteIpAddr = IP_ADDR_ANY;
   remotePort = 0;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through socket descriptors
   for(i = 0; i < SOCKET_MAX_COUNT; i++)
   {
      //Point to current socket
      socket = &socketTable[i];

      //TCP socket?
      if(socket->type == SOCKET_TYPE_STREAM)
      {
         //Check current state
         if(socket->state != TCP_STATE_LISTEN)
         {
            //Append the instance identifier to the OID prefix
            n = object->oidLen;

            //tcpConnectionLocalAddressType and tcpConnectionLocalAddress are used
            //as 1st and 2nd instance identifiers
            error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &socket->localIpAddr);
            //Any error to report?
            if(error)
               return error;

            //tcpConnectionLocalPort is used as 3rd instance identifier
            error = mibEncodePort(nextOid, *nextOidLen, &n, socket->localPort);
            //Any error to report?
            if(error)
               return error;

            //tcpConnectionRemAddressType and tcpConnectionRemAddress are used
            //as 4th and 5th instance identifiers
            error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &socket->remoteIpAddr);
            //Any error to report?
            if(error)
               return error;

            //tcpConnectionRemPort is used as 6th instance identifier
            error = mibEncodePort(nextOid, *nextOidLen, &n, socket->remotePort);
            //Any error to report?
            if(error)
               return error;

            //Check whether the resulting object identifier lexicographically
            //follows the specified OID
            if(oidComp(nextOid, n, oid, oidLen) > 0)
            {
               //Perform lexicographic comparison
               if(localPort == 0 && remotePort == 0)
               {
                  acceptable = TRUE;
               }
               else if(mibCompIpAddr(&socket->localIpAddr, &localIpAddr) < 0)
               {
                  acceptable = TRUE;
               }
               else if(mibCompIpAddr(&socket->localIpAddr, &localIpAddr) > 0)
               {
                  acceptable = FALSE;
               }
               else if(socket->localPort < localPort)
               {
                  acceptable = TRUE;
               }
               else if(socket->localPort > localPort)
               {
                  acceptable = FALSE;
               }
               else if(mibCompIpAddr(&socket->remoteIpAddr, &remoteIpAddr) < 0)
               {
                  acceptable = TRUE;
               }
               else if(mibCompIpAddr(&socket->remoteIpAddr, &remoteIpAddr) > 0)
               {
                  acceptable = FALSE;
               }
               else if(socket->remotePort < remotePort)
               {
                  acceptable = TRUE;
               }
               else
               {
                  acceptable = FALSE;
               }

               //Save the closest object identifier that follows the specified
               //OID in lexicographic order
               if(acceptable)
               {
                  localIpAddr = socket->localIpAddr;
                  localPort = socket->localPort;
                  remoteIpAddr = socket->remoteIpAddr;
                  remotePort = socket->remotePort;
               }
            }
         }
      }
   }

   //The specified OID does not lexicographically precede the name
   //of some object?
   if(localPort == 0 && remotePort == 0)
      return ERROR_OBJECT_NOT_FOUND;

   //Append the instance identifier to the OID prefix
   n = object->oidLen;

   //tcpConnectionLocalAddressType and tcpConnectionLocalAddress are used
   //as 1st and 2nd instance identifiers
   error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &localIpAddr);
   //Any error to report?
   if(error)
      return error;

   //tcpConnectionLocalPort is used as 3rd instance identifier
   error = mibEncodePort(nextOid, *nextOidLen, &n, localPort);
   //Any error to report?
   if(error)
      return error;

   //tcpConnectionRemAddressType and tcpConnectionRemAddress are used
   //as 4th and 5th instance identifiers
   error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &remoteIpAddr);
   //Any error to report?
   if(error)
      return error;

   //tcpConnectionRemPort is used as 6th instance identifier
   error = mibEncodePort(nextOid, *nextOidLen, &n, remotePort);
   //Any error to report?
   if(error)
      return error;

   //Save the length of the resulting object identifier
   *nextOidLen = n;
   //Next object found
   return NO_ERROR;
}


/**
 * @brief Get tcpListenerEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t tcpMibGetTcpListenerEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   uint_t i;
   size_t n;
   IpAddr localIpAddr;
   uint16_t localPort;
   Socket *socket;

   //Point to the instance identifier
   n = object->oidLen;

   //tcpListenerLocalAddressType and tcpListenerLocalAddress are used
   //as 1st and 2nd instance identifiers
   error = mibDecodeIpAddr(oid, oidLen, &n, &localIpAddr);
   //Invalid instance identifier?
   if(error)
      return error;

   //tcpListenerLocalPort is used as 3rd instance identifier
   error = mibDecodePort(oid, oidLen, &n, &localPort);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Loop through socket descriptors
   for(i = 0; i < SOCKET_MAX_COUNT; i++)
   {
      //Point to current socket
      socket = &socketTable[i];

      //TCP socket?
      if(socket->type == SOCKET_TYPE_STREAM)
      {
         //Check local IP address
         if(!ipCompAddr(&socket->localIpAddr, &localIpAddr))
            continue;
         //Check local port number
         if(socket->localPort != localPort)
            continue;
         //Check current state
         if(socket->state != TCP_STATE_LISTEN)
            continue;

         //A matching socket has been found
         break;
      }
   }

   //No matching connection found in socket table?
   if(i >= SOCKET_MAX_COUNT)
      return ERROR_INSTANCE_NOT_FOUND;

   //tcpListenerProcess object?
   if(!osStrcmp(object->name, "tcpListenerProcess"))
   {
      //ID of the process associated with this listener
      value->unsigned32 = 0;
   }
   //Unknown object?
   else
   {
      //The specified object does not exist
      error = ERROR_OBJECT_NOT_FOUND;
   }

   //Return status code
   return error;
}


/**
 * @brief Get next tcpListenerEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t tcpMibGetNextTcpListenerEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   uint_t i;
   size_t n;
   bool_t acceptable;
   IpAddr localIpAddr;
   uint16_t localPort;
   Socket *socket;

   //Initialize variables
   localIpAddr = IP_ADDR_ANY;
   localPort = 0;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through socket descriptors
   for(i = 0; i < SOCKET_MAX_COUNT; i++)
   {
      //Point to current socket
      socket = &socketTable[i];

      //TCP socket?
      if(socket->type == SOCKET_TYPE_STREAM)
      {
         //Check current state
         if(socket->state == TCP_STATE_LISTEN)
         {
            //Append the instance identifier to the OID prefix
            n = object->oidLen;

            //tcpListenerLocalAddressType and tcpListenerLocalAddress are used
            //as 1st and 2nd instance identifiers
            error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &socket->localIpAddr);
            //Any error to report?
            if(error)
               return error;

            //tcpListenerLocalPort is used as 3rd instance identifier
            error = mibEncodePort(nextOid, *nextOidLen, &n, socket->localPort);
            //Any error to report?
            if(error)
               return error;

            //Check whether the resulting object identifier lexicographically
            //follows the specified OID
            if(oidComp(nextOid, n, oid, oidLen) > 0)
            {
               //Perform lexicographic comparison
               if(localPort == 0)
               {
                  acceptable = TRUE;
               }
               else if(mibCompIpAddr(&socket->localIpAddr, &localIpAddr) < 0)
               {
                  acceptable = TRUE;
               }
               else if(mibCompIpAddr(&socket->localIpAddr, &localIpAddr) > 0)
               {
                  acceptable = FALSE;
               }
               else if(socket->localPort < localPort)
               {
                  acceptable = TRUE;
               }
               else
               {
                  acceptable = FALSE;
               }

               //Save the closest object identifier that follows the specified
               //OID in lexicographic order
               if(acceptable)
               {
                  localIpAddr = socket->localIpAddr;
                  localPort = socket->localPort;
               }
            }
         }
      }
   }

   //The specified OID does not lexicographically precede the name
   //of some object?
   if(localPort == 0)
      return ERROR_OBJECT_NOT_FOUND;

   //Append the instance identifier to the OID prefix
   n = object->oidLen;

   //tcpListenerLocalAddressType and tcpListenerLocalAddress are used
   //as 1st and 2nd instance identifiers
   error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &localIpAddr);
   //Any error to report?
   if(error)
      return error;

   //tcpListenerLocalPort is used as 3rd instance identifier
   error = mibEncodePort(nextOid, *nextOidLen, &n, localPort);
   //Any error to report?
   if(error)
      return error;

   //Save the length of the resulting object identifier
   *nextOidLen = n;
   //Next object found
   return NO_ERROR;
}

#endif
