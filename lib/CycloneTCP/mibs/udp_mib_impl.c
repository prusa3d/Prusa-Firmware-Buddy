/**
 * @file udp_mib_impl.c
 * @brief UDP MIB module implementation
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
#include "mibs/udp_mib_module.h"
#include "mibs/udp_mib_impl.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (UDP_MIB_SUPPORT == ENABLED && UDP_SUPPORT == ENABLED)


/**
 * @brief UDP MIB module initialization
 * @return Error code
 **/

error_t udpMibInit(void)
{
   //Debug message
   TRACE_INFO("Initializing UDP-MIB base...\r\n");

   //Clear UDP MIB base
   osMemset(&udpMibBase, 0, sizeof(udpMibBase));

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Get udpEndpointEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t udpMibGetUdpEndpointEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   uint_t i;
   size_t n;
   IpAddr localIpAddr;
   uint16_t localPort;
   IpAddr remoteIpAddr;
   uint16_t remotePort;
   uint32_t instance;

   //Point to the instance identifier
   n = object->oidLen;

   //udpEndpointLocalAddressType and udpEndpointLocalAddress are used
   //as 1st and 2nd instance identifiers
   error = mibDecodeIpAddr(oid, oidLen, &n, &localIpAddr);
   //Invalid instance identifier?
   if(error)
      return error;

   //udpEndpointLocalPort is used as 3rd instance identifier
   error = mibDecodePort(oid, oidLen, &n, &localPort);
   //Invalid instance identifier?
   if(error)
      return error;

   //udpEndpointRemoteAddressType and udpEndpointRemoteAddress are used
   //as 4th and 5th instance identifiers
   error = mibDecodeIpAddr(oid, oidLen, &n, &remoteIpAddr);
   //Invalid instance identifier?
   if(error)
      return error;

   //udpEndpointRemotePort is used as 6th instance identifier
   error = mibDecodePort(oid, oidLen, &n, &remotePort);
   //Invalid instance identifier?
   if(error)
      return error;

   //udpEndpointInstance is used as 7th instance identifier
   error = mibDecodeUnsigned32(oid, oidLen, &n, &instance);
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
      Socket *socket = &socketTable[i];

      //UDP socket?
      if(socket->type == SOCKET_TYPE_DGRAM)
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

         //A matching socket has been found
         break;
      }
   }

   //No matching connection found in socket table?
   if(i >= SOCKET_MAX_COUNT)
   {
      //Loop through the UDP callback table
      for(i = 0; i < UDP_CALLBACK_TABLE_SIZE; i++)
      {
         //Point to the current entry
         UdpRxCallbackEntry *entry = &udpCallbackTable[i];

         //Check whether the entry is currently in use
         if(entry->callback != NULL)
         {
            //Check local port number
            if(entry->port == localPort)
               break;
         }
      }

      //No matching connection found in UDP callback table?
      if(i >= UDP_CALLBACK_TABLE_SIZE)
         return ERROR_INSTANCE_NOT_FOUND;
   }

   //udpEndpointProcess object?
   if(!osStrcmp(object->name, "udpEndpointProcess"))
   {
      //ID of the process associated with this endpoint
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
 * @brief Get next udpEndpointEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t udpMibGetNextUdpEndpointEntry(const MibObject *object, const uint8_t *oid,
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
   uint32_t instance;

   //Initialize variables
   localIpAddr = IP_ADDR_ANY;
   localPort = 0;
   remoteIpAddr = IP_ADDR_ANY;
   remotePort = 0;
   instance = 1;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through socket descriptors
   for(i = 0; i < SOCKET_MAX_COUNT; i++)
   {
      //Point to current socket
      Socket *socket = &socketTable[i];

      //UDP socket?
      if(socket->type == SOCKET_TYPE_DGRAM)
      {
         //Append the instance identifier to the OID prefix
         n = object->oidLen;

         //udpEndpointLocalAddressType and udpEndpointLocalAddress are used
         //as 1st and 2nd instance identifiers
         error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &socket->localIpAddr);
         //Any error to report?
         if(error)
            return error;

         //udpEndpointLocalPort is used as 3rd instance identifier
         error = mibEncodePort(nextOid, *nextOidLen, &n, socket->localPort);
         //Any error to report?
         if(error)
            return error;

         //udpEndpointRemoteAddressType and udpEndpointRemoteAddress are used
         //as 4th and 5th instance identifiers
         error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &socket->remoteIpAddr);
         //Any error to report?
         if(error)
            return error;

         //udpEndpointRemotePort is used as 6th instance identifier
         error = mibEncodePort(nextOid, *nextOidLen, &n, socket->remotePort);
         //Any error to report?
         if(error)
            return error;

         //udpEndpointInstance is used as 7th instance identifier
         error = mibEncodeUnsigned32(nextOid, *nextOidLen, &n, instance);
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

   //Loop through the UDP callback table
   for(i = 0; i < UDP_CALLBACK_TABLE_SIZE; i++)
   {
      //Point to the current entry
      UdpRxCallbackEntry *entry = &udpCallbackTable[i];

      //Check whether the entry is currently in use
      if(entry->callback != NULL)
      {
         //Append the instance identifier to the OID prefix
         n = object->oidLen;

         //udpEndpointLocalAddressType and udpEndpointLocalAddress are used
         //as 1st and 2nd instance identifiers
         error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &IP_ADDR_ANY);
         //Any error to report?
         if(error)
            return error;

         //udpEndpointLocalPort is used as 3rd instance identifier
         error = mibEncodePort(nextOid, *nextOidLen, &n, entry->port);
         //Any error to report?
         if(error)
            return error;

         //udpEndpointRemoteAddressType and udpEndpointRemoteAddress are used
         //as 4th and 5th instance identifiers
         error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &IP_ADDR_ANY);
         //Any error to report?
         if(error)
            return error;

         //udpEndpointRemotePort is used as 6th instance identifier
         error = mibEncodePort(nextOid, *nextOidLen, &n, 0);
         //Any error to report?
         if(error)
            return error;

         //udpEndpointInstance is used as 7th instance identifier
         error = mibEncodeUnsigned32(nextOid, *nextOidLen, &n, instance);
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
            else if(mibCompIpAddr(&IP_ADDR_ANY, &localIpAddr) < 0)
            {
               acceptable = TRUE;
            }
            else if(mibCompIpAddr(&IP_ADDR_ANY, &localIpAddr) > 0)
            {
               acceptable = FALSE;
            }
            else if(entry->port < localPort)
            {
               acceptable = TRUE;
            }
            else if(entry->port > localPort)
            {
               acceptable = FALSE;
            }
            else if(mibCompIpAddr(&IP_ADDR_ANY, &remoteIpAddr) < 0)
            {
               acceptable = TRUE;
            }
            else if(mibCompIpAddr(&IP_ADDR_ANY, &remoteIpAddr) > 0)
            {
               acceptable = FALSE;
            }
            else if(0 < remotePort)
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
               localIpAddr = IP_ADDR_ANY;
               localPort = entry->port;
               remoteIpAddr = IP_ADDR_ANY;
               remotePort = 0;
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

   //udpEndpointLocalAddressType and udpEndpointLocalAddress are used
   //as 1st and 2nd instance identifiers
   error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &localIpAddr);
   //Any error to report?
   if(error)
      return error;

   //udpEndpointLocalPort is used as 3rd instance identifier
   error = mibEncodePort(nextOid, *nextOidLen, &n, localPort);
   //Any error to report?
   if(error)
      return error;

   //udpEndpointRemoteAddressType and udpEndpointRemoteAddress are used
   //as 4th and 5th instance identifiers
   error = mibEncodeIpAddr(nextOid, *nextOidLen, &n, &remoteIpAddr);
   //Any error to report?
   if(error)
      return error;

   //udpEndpointRemotePort is used as 6th instance identifier
   error = mibEncodePort(nextOid, *nextOidLen, &n, remotePort);
   //Any error to report?
   if(error)
      return error;

   //udpEndpointInstance is used as 7th instance identifier
   error = mibEncodeUnsigned32(nextOid, *nextOidLen, &n, instance);
   //Any error to report?
   if(error)
      return error;

   //Save the length of the resulting object identifier
   *nextOidLen = n;
   //Next object found
   return NO_ERROR;
}

#endif
