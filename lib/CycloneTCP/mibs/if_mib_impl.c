/**
 * @file if_mib_impl.c
 * @brief Interfaces Group MIB module implementation
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
#include "mibs/if_mib_module.h"
#include "mibs/if_mib_impl.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IF_MIB_SUPPORT == ENABLED)


/**
 * @brief Interfaces Group MIB module initialization
 * @return Error code
 **/

error_t ifMibInit(void)
{
   uint_t i;

   //Debug message
   TRACE_INFO("Initializing IF-MIB base...\r\n");

   //Clear Interfaces Group MIB base
   osMemset(&ifMibBase, 0, sizeof(ifMibBase));

   //ifNumber object
   ifMibBase.ifNumber = NET_INTERFACE_COUNT;

   //Extension to the interface table
   for(i = 0; i < NET_INTERFACE_COUNT; i++)
   {
      //ifLinkUpDownTrapEnable object
      ifMibBase.ifXTable[i].ifLinkUpDownTrapEnable = IF_MIB_IF_LINK_UP_DOWN_TRAP_DISABLED;
      //ifPromiscuousMode object
      ifMibBase.ifXTable[i].ifPromiscuousMode = MIB_TRUTH_VALUE_FALSE;
      //ifConnectorPresent object
      ifMibBase.ifXTable[i].ifConnectorPresent = MIB_TRUTH_VALUE_TRUE;
   }

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set ifEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t ifMibSetIfEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
   //Not implemented
   return ERROR_WRITE_FAILED;
}


/**
 * @brief Get ifEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t ifMibGetIfEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   size_t n;
   uint_t index;
   IfMibIfEntry *entry;
   NetInterface *interface;
   NetInterface *physicalInterface;

   //Point to the instance identifier
   n = object->oidLen;

   //ifIndex is used as instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &index);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Check index range
   if(index < 1 || index > NET_INTERFACE_COUNT)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the underlying interface
   interface = &netInterface[index - 1];
   //Point to the interface table entry
   entry = &ifMibBase.ifTable[index - 1];

   //Point to the physical interface
   physicalInterface = nicGetPhysicalInterface(interface);

   //ifIndex object?
   if(!osStrcmp(object->name, "ifIndex"))
   {
      //Get object value
      value->integer = index;
   }
   //ifDescr object?
   else if(!osStrcmp(object->name, "ifDescr"))
   {
      //Retrieve the length of the interface name
      n = osStrlen(interface->name);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, interface->name, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //ifType object?
   else if(!osStrcmp(object->name, "ifType"))
   {
#if (ETH_VLAN_SUPPORT == ENABLED)
      //VLAN interface?
      if(interface->vlanId != 0)
      {
         //Layer 2 virtual LAN using 802.1Q
         value->integer = IF_MIB_IF_TYPE_L2_VLAN;
      }
      else
#endif
      {
         //Sanity check
         if(physicalInterface->nicDriver != NULL)
         {
            //Get interface type
            switch(physicalInterface->nicDriver->type)
            {
            //Ethernet interface
            case NIC_TYPE_ETHERNET:
               value->integer = IF_MIB_IF_TYPE_ETHERNET_CSMACD;
               break;
            //PPP interface
            case NIC_TYPE_PPP:
               value->integer = IF_MIB_IF_TYPE_PPP;
               break;
            //IEEE 802.15.4 WPAN interface
            case NIC_TYPE_6LOWPAN:
               value->integer = IF_MIB_IF_TYPE_IEEE_802_15_4;
               break;
            //Unknown interface type
            default:
               value->integer = IF_MIB_IF_TYPE_OTHER;
               break;
            }
         }
         else
         {
            //Unknown interface type
            value->integer = IF_MIB_IF_TYPE_OTHER;
         }
      }
   }
   //ifMtu object?
   else if(!osStrcmp(object->name, "ifMtu"))
   {
      //Get interface MTU
      if(physicalInterface->nicDriver != NULL)
      {
         value->integer = physicalInterface->nicDriver->mtu;
      }
      else
      {
         value->integer = 0;
      }
   }
   //ifSpeed object?
   else if(!osStrcmp(object->name, "ifSpeed"))
   {
      //Get interface's current bandwidth
      value->gauge32 = interface->linkSpeed;
   }
#if (ETH_SUPPORT == ENABLED)
   //ifPhysAddress object?
   else if(!osStrcmp(object->name, "ifPhysAddress"))
   {
      NetInterface *logicalInterface;

      //Point to the logical interface
      logicalInterface = nicGetLogicalInterface(interface);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= sizeof(MacAddr))
      {
         //Copy object value
         macCopyAddr(value->octetString, &logicalInterface->macAddr);
         //Return object length
         *valueLen = sizeof(MacAddr);
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
#endif
   //ifAdminStatus object?
   else if(!osStrcmp(object->name, "ifAdminStatus"))
   {
      //Check whether the interface is enabled for operation
      if(physicalInterface->nicDriver != NULL)
      {
         value->integer = IF_MIB_IF_ADMIN_STATUS_UP;
      }
      else
      {
         value->integer = IF_MIB_IF_ADMIN_STATUS_DOWN;
      }
   }
   //ifOperStatus object?
   else if(!osStrcmp(object->name, "ifOperStatus"))
   {
      //Get the current operational state of the interface
      if(interface->linkState)
      {
         value->integer = IF_MIB_IF_OPER_STATUS_UP;
      }
      else
      {
         value->integer = IF_MIB_IF_OPER_STATUS_DOWN;
      }
   }
   //ifLastChange object?
   else if(!osStrcmp(object->name, "ifLastChange"))
   {
      //Get object value
      value->timeTicks = entry->ifLastChange;
   }
   //ifInOctets object?
   else if(!osStrcmp(object->name, "ifInOctets"))
   {
      //Get object value
      value->counter32 = entry->ifInOctets;
   }
   //ifInUcastPkts object?
   else if(!osStrcmp(object->name, "ifInUcastPkts"))
   {
      //Get object value
      value->counter32 = entry->ifInUcastPkts;
   }
   //ifInDiscards object?
   else if(!osStrcmp(object->name, "ifInDiscards"))
   {
      //Get object value
      value->counter32 = entry->ifInDiscards;
   }
   //ifInErrors object?
   else if(!osStrcmp(object->name, "ifInErrors"))
   {
      //Get object value
      value->counter32 = entry->ifInErrors;
   }
   //ifInUnknownProtos object?
   else if(!osStrcmp(object->name, "ifInUnknownProtos"))
   {
      //Get object value
      value->counter32 = entry->ifInUnknownProtos;
   }
   //ifOutOctets object?
   else if(!osStrcmp(object->name, "ifOutOctets"))
   {
      //Get object value
      value->counter32 = entry->ifOutOctets;
   }
   //ifOutUcastPkts object?
   else if(!osStrcmp(object->name, "ifOutUcastPkts"))
   {
      //Get object value
      value->counter32 = entry->ifOutUcastPkts;
   }
   //ifOutDiscards object?
   else if(!osStrcmp(object->name, "ifOutDiscards"))
   {
      //Get object value
      value->counter32 = entry->ifOutDiscards;
   }
   //ifOutErrors object?
   else if(!osStrcmp(object->name, "ifOutErrors"))
   {
      //Get object value
      value->counter32 = entry->ifOutErrors;
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
 * @brief Get next ifEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t ifMibGetNextIfEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   size_t n;
   uint_t index;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through network interfaces
   for(index = 1; index <= NET_INTERFACE_COUNT; index++)
   {
      //Append the instance identifier to the OID prefix
      n = object->oidLen;

      //ifIndex is used as instance identifier
      error = mibEncodeIndex(nextOid, *nextOidLen, &n, index);
      //Any error to report?
      if(error)
         return error;

      //Check whether the resulting object identifier lexicographically
      //follows the specified OID
      if(oidComp(nextOid, n, oid, oidLen) > 0)
      {
         //Save the length of the resulting object identifier
         *nextOidLen = n;
         //Next object found
         return NO_ERROR;
      }
   }

   //The specified OID does not lexicographically precede the name
   //of some object
   return ERROR_OBJECT_NOT_FOUND;
}


/**
 * @brief Set ifXEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t ifMibSetIfXEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
   //Not implemented
   return ERROR_WRITE_FAILED;
}


/**
 * @brief Get ifXEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t ifMibGetIfXEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   size_t n;
   uint_t index;
   IfMibIfXEntry *entry;
   NetInterface *interface;

   //Point to the instance identifier
   n = object->oidLen;

   //ifIndex is used as instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &index);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Check index range
   if(index < 1 || index > NET_INTERFACE_COUNT)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the underlying interface
   interface = &netInterface[index - 1];
   //Point to the interface table entry
   entry = &ifMibBase.ifXTable[index - 1];

   //ifName object?
   if(!osStrcmp(object->name, "ifName"))
   {
      //Retrieve the length of the interface name
      n = osStrlen(interface->name);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, interface->name, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //ifInMulticastPkts object?
   else if(!osStrcmp(object->name, "ifInMulticastPkts"))
   {
      //Get object value
      value->counter32 = entry->ifInMulticastPkts;
   }
   //ifInBroadcastPkts object?
   else if(!osStrcmp(object->name, "ifInBroadcastPkts"))
   {
      //Get object value
      value->counter32 = entry->ifInBroadcastPkts;
   }
   //ifOutMulticastPkts object?
   else if(!osStrcmp(object->name, "ifOutMulticastPkts"))
   {
      //Get object value
      value->counter32 = entry->ifOutMulticastPkts;
   }
   //ifOutBroadcastPkts object?
   else if(!osStrcmp(object->name, "ifOutBroadcastPkts"))
   {
      //Get object value
      value->counter32 = entry->ifOutBroadcastPkts;
   }
   //ifHCInOctets object?
   else if(!osStrcmp(object->name, "ifHCInOctets"))
   {
      //Get object value
      value->counter64 = entry->ifHCInOctets;
   }
   //ifHCInUcastPkts object?
   else if(!osStrcmp(object->name, "ifHCInUcastPkts"))
   {
      //Get object value
      value->counter64 = entry->ifHCInUcastPkts;
   }
   //ifHCInMulticastPkts object?
   else if(!osStrcmp(object->name, "ifHCInMulticastPkts"))
   {
      //Get object value
      value->counter64 = entry->ifHCInMulticastPkts;
   }
   //ifHCInBroadcastPkts object?
   else if(!osStrcmp(object->name, "ifHCInBroadcastPkts"))
   {
      //Get object value
      value->counter64 = entry->ifHCInBroadcastPkts;
   }
   //ifHCOutOctets object?
   else if(!osStrcmp(object->name, "ifHCOutOctets"))
   {
      //Get object value
      value->counter64 = entry->ifHCOutOctets;
   }
   //ifHCOutUcastPkts object?
   else if(!osStrcmp(object->name, "ifHCOutUcastPkts"))
   {
      //Get object value
      value->counter64 = entry->ifHCOutUcastPkts;
   }
   //ifHCOutMulticastPkts object?
   else if(!osStrcmp(object->name, "ifHCOutMulticastPkts"))
   {
      //Get object value
      value->counter64 = entry->ifHCOutMulticastPkts;
   }
   //ifHCOutBroadcastPkts object?
   else if(!osStrcmp(object->name, "ifHCOutBroadcastPkts"))
   {
      //Get object value
      value->counter64 = entry->ifHCOutBroadcastPkts;
   }
   //ifLinkUpDownTrapEnable object?
   else if(!osStrcmp(object->name, "ifLinkUpDownTrapEnable"))
   {
      //Get object value
      value->integer = entry->ifLinkUpDownTrapEnable;
   }
   //ifHighSpeed object?
   else if(!osStrcmp(object->name, "ifHighSpeed"))
   {
      //Get interface's current bandwidth
      value->gauge32 = interface->linkSpeed / 1000000;
   }
   //ifPromiscuousMode object?
   else if(!osStrcmp(object->name, "ifPromiscuousMode"))
   {
      //Get object value
      value->integer = entry->ifPromiscuousMode;
   }
   //ifConnectorPresent object?
   else if(!osStrcmp(object->name, "ifConnectorPresent"))
   {
      //Get object value
      value->integer = entry->ifConnectorPresent;
   }
   //ifAlias object?
   else if(!osStrcmp(object->name, "ifAlias"))
   {
      //On the first instantiation of an interface, the value of ifAlias
      //associated with that interface is the zero-length string
      *valueLen = 0;
   }
   //ifCounterDiscontinuityTime object?
   else if(!osStrcmp(object->name, "ifCounterDiscontinuityTime"))
   {
      //Get object value
      value->timeTicks = 0;
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
 * @brief Get next ifXEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t ifMibGetNextIfXEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   size_t n;
   uint_t index;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through network interfaces
   for(index = 1; index <= NET_INTERFACE_COUNT; index++)
   {
      //Append the instance identifier to the OID prefix
      n = object->oidLen;

      //ifIndex is used as instance identifier
      error = mibEncodeIndex(nextOid, *nextOidLen, &n, index);
      //Any error to report?
      if(error)
         return error;

      //Check whether the resulting object identifier lexicographically
      //follows the specified OID
      if(oidComp(nextOid, n, oid, oidLen) > 0)
      {
         //Save the length of the resulting object identifier
         *nextOidLen = n;
         //Next object found
         return NO_ERROR;
      }
   }

   //The specified OID does not lexicographically precede the name
   //of some object
   return ERROR_OBJECT_NOT_FOUND;
}


/**
 * @brief Set ifStackEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t ifMibSetIfStackEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
   //Not implemented
   return ERROR_WRITE_FAILED;
}


/**
 * @brief Get ifStackEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t ifMibGetIfStackEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   size_t n;
   uint_t index;
   uint_t higherLayer;
   uint_t lowerLayer;

   //Point to the instance identifier
   n = object->oidLen;

   //ifStackHigherLayer is used as 1st instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &higherLayer);
   //Invalid instance identifier?
   if(error)
      return error;

   //ifStackLowerLayer is used as 2nd instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &lowerLayer);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Loop through network interfaces
   for(index = 1; index <= NET_INTERFACE_COUNT; index++)
   {
      //Check higher and lower sub-layers
      if(higherLayer == 0 && lowerLayer == index)
      {
         break;
      }
      else if(higherLayer == index && lowerLayer == 0)
      {
         break;
      }
   }

   //No matching interface?
   if(index > NET_INTERFACE_COUNT)
      return ERROR_INSTANCE_NOT_FOUND;

   //ifStackStatus object?
   if(!osStrcmp(object->name, "ifStackStatus"))
   {
      //status of the relationship between the two sub-layers
      value->integer = MIB_ROW_STATUS_ACTIVE;
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
 * @brief Get next ifStackEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t ifMibGetNextIfStackEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   uint_t k;
   size_t n;
   uint_t index;
   uint_t higherLayer;
   uint_t lowerLayer;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //two rows exist even for an interface which has no others stacked
   //on top or below it
   for(k = 0; k < 2; k++)
   {
      //Loop through network interfaces
      for(index = 1; index <= NET_INTERFACE_COUNT; index++)
      {
         //Append the instance identifier to the OID prefix
         n = object->oidLen;

         //Higher and lower sub-layers
         if(k == 0)
         {
            higherLayer = 0;
            lowerLayer = index;
         }
         else
         {
            higherLayer = index;
            lowerLayer = 0;
         }

         //ifStackHigherLayer is used as 1st instance identifier
         error = mibEncodeIndex(nextOid, *nextOidLen, &n, higherLayer);
         //Any error to report?
         if(error)
            return error;

         //ifStackLowerLayer is used as 2nd instance identifier
         error = mibEncodeIndex(nextOid, *nextOidLen, &n, lowerLayer);
         //Any error to report?
         if(error)
            return error;

         //Check whether the resulting object identifier lexicographically
         //follows the specified OID
         if(oidComp(nextOid, n, oid, oidLen) > 0)
         {
            //Save the length of the resulting object identifier
            *nextOidLen = n;
            //Next object found
            return NO_ERROR;
         }
      }
   }

   //The specified OID does not lexicographically precede the name
   //of some object
   return ERROR_OBJECT_NOT_FOUND;
}


/**
 * @brief Set ifRcvAddressEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t ifMibSetIfRcvAddressEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
   //Not implemented
   return ERROR_WRITE_FAILED;
}


/**
 * @brief Get ifRcvAddressEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t ifMibGetIfRcvAddressEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
#if (ETH_SUPPORT == ENABLED)
   error_t error;
   uint_t i;
   size_t n;
   uint_t index;
   MacAddr macAddr;
   NetInterface *interface;
   NetInterface *logicalInterface;

   //Point to the instance identifier
   n = object->oidLen;

   //ifIndex is used as 1st instance identifier
   error = mibDecodeIndex(oid, oidLen, &n, &index);
   //Invalid instance identifier?
   if(error)
      return error;

   //ifRcvAddressAddress is used as 2nd instance identifier
   error = mibDecodePhysAddr(oid, oidLen, &n, &macAddr);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Check index range
   if(index < 1 || index > NET_INTERFACE_COUNT)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the underlying interface
   interface = &netInterface[index - 1];
   //Point to the logical interface
   logicalInterface = nicGetLogicalInterface(interface);

   //Initialize status code
   error = ERROR_INSTANCE_NOT_FOUND;

   //Interface MAC address?
   if(macCompAddr(&macAddr, &logicalInterface->macAddr))
   {
      //The MAC address is acceptable
      error = NO_ERROR;
   }
   //Broadcast address?
   else if(macCompAddr(&macAddr, &MAC_BROADCAST_ADDR))
   {
      //The MAC address is acceptable
      error = NO_ERROR;
   }
   //Multicast address?
   else if(macIsMulticastAddr(&macAddr))
   {
      //Go through the MAC filter table
      for(i = 0; i < MAC_ADDR_FILTER_SIZE; i++)
      {
         //Valid entry?
         if(interface->macAddrFilter[i].refCount > 0)
         {
            //Check whether the MAC address matches a relevant multicast address
            if(macCompAddr(&macAddr, &interface->macAddrFilter[i].addr))
            {
               //The MAC address is acceptable
               error = NO_ERROR;
            }
         }
      }
   }

   //Check whether the MAC address is acceptable
   if(!error)
   {
      //ifRcvAddressStatus object?
      if(!osStrcmp(object->name, "ifRcvAddressStatus"))
      {
         //Get object value
         value->integer = MIB_ROW_STATUS_ACTIVE;
      }
      //ifRcvAddressType object?
      else if(!osStrcmp(object->name, "ifRcvAddressType"))
      {
         //Get object value
         if(macCompAddr(&macAddr, &logicalInterface->macAddr) ||
            macCompAddr(&macAddr, &MAC_BROADCAST_ADDR))
         {
            //The entry is not volatile
            value->integer = IF_MIB_RCV_ADDRESS_TYPE_NON_VOLATILE;
         }
         else
         {
            //The entry is volatile
            value->integer = IF_MIB_RCV_ADDRESS_TYPE_VOLATILE;
         }
      }
      //Unknown object?
      else
      {
         //The specified object does not exist
         error = ERROR_OBJECT_NOT_FOUND;
      }
   }

   //Return status code
   return error;
#else
   //Not implemented
   return ERROR_OBJECT_NOT_FOUND;
#endif
}


/**
 * @brief Get next ifRcvAddressEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t ifMibGetNextIfRcvAddressEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
#if (ETH_SUPPORT == ENABLED)
   error_t error;
   int_t i;
   size_t n;
   uint_t index;
   uint_t curIndex;
   bool_t acceptable;
   MacAddr macAddr;
   MacAddr curMacAddr;
   NetInterface *interface;
   NetInterface *logicalInterface;

   //Initialize variables
   index = 0;
   macAddr = MAC_UNSPECIFIED_ADDR;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through network interfaces
   for(curIndex = 1; curIndex <= NET_INTERFACE_COUNT; curIndex++)
   {
      //Point to the underlying interface
      interface = &netInterface[curIndex - 1];
      //Point to the logical interface
      logicalInterface = nicGetLogicalInterface(interface);

      //Go through the MAC filter table
      for(i = -2; i < MAC_ADDR_FILTER_SIZE; i++)
      {
         if(i == -2)
         {
            //Get interface MAC address
            curMacAddr = logicalInterface->macAddr;
         }
         else if(i == -1)
         {
            //Get broadcast address
            curMacAddr = MAC_BROADCAST_ADDR;
         }
         else
         {
            //Get multicast address
            if(interface->macAddrFilter[i].refCount > 0)
            {
               curMacAddr = interface->macAddrFilter[i].addr;
            }
            else
            {
               curMacAddr = MAC_UNSPECIFIED_ADDR;
            }
         }

         //Valid MAC address?
         if(!macCompAddr(&curMacAddr, &MAC_UNSPECIFIED_ADDR))
         {
            //Append the instance identifier to the OID prefix
            n = object->oidLen;

            //ifIndex is used as 1st instance identifier
            error = mibEncodeIndex(nextOid, *nextOidLen, &n, curIndex);
            //Invalid instance identifier?
            if(error)
               return error;

            //ifRcvAddressAddress is used as 2nd instance identifier
            error = mibEncodePhysAddr(nextOid, *nextOidLen, &n, &curMacAddr);
            //Invalid instance identifier?
            if(error)
               return error;

            //Check whether the resulting object identifier lexicographically
            //follows the specified OID
            if(oidComp(nextOid, n, oid, oidLen) > 0)
            {
               //Perform lexicographic comparison
               if(index == 0)
               {
                  acceptable = TRUE;
               }
               else if(curIndex < index)
               {
                  acceptable = TRUE;
               }
               else if(curIndex > index)
               {
                  acceptable = FALSE;
               }
               else if(osMemcmp(&curMacAddr, &macAddr, sizeof(MacAddr)) < 0)
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
                  macAddr = curMacAddr;
                  index = curIndex;
               }
            }
         }
      }
   }

   //The specified OID does not lexicographically precede the name
   //of some object?
   if(index == 0)
      return ERROR_OBJECT_NOT_FOUND;

   //Append the instance identifier to the OID prefix
   n = object->oidLen;

   //ifIndex is used as 1st instance identifier
   error = mibEncodeIndex(nextOid, *nextOidLen, &n, index);
   //Invalid instance identifier?
   if(error)
      return error;

   //ifRcvAddressAddress is used as 2nd instance identifier
   error = mibEncodePhysAddr(nextOid, *nextOidLen, &n, &macAddr);
   //Invalid instance identifier?
   if(error)
      return error;

   //Save the length of the resulting object identifier
   *nextOidLen = n;
   //Next object found
   return NO_ERROR;
#else
   //Not implemented
   return ERROR_OBJECT_NOT_FOUND;
#endif
}

#endif
