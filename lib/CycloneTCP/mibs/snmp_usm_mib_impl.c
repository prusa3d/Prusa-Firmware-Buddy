/**
 * @file snmp_usm_mib_impl.c
 * @brief SNMP USM MIB module implementation
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
#include "mibs/snmp_usm_mib_module.h"
#include "mibs/snmp_usm_mib_impl.h"
#include "snmp/snmp_agent.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "encoding/oid.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_USM_MIB_SUPPORT == ENABLED)

//usmNoAuthProtocol OID (1.3.6.1.6.3.10.1.1.1)
const uint8_t usmNoAuthProtocolOid[9] = {43, 6, 1, 6, 3, 10, 1, 1, 1};
//usmHMACMD5AuthProtocol OID (1.3.6.1.6.3.10.1.1.2)
const uint8_t usmHMACMD5AuthProtocolOid[9] = {43, 6, 1, 6, 3, 10, 1, 1, 2};
//usmHMACSHAAuthProtocol OID (1.3.6.1.6.3.10.1.1.3)
const uint8_t usmHMACSHAAuthProtocolOid[9] = {43, 6, 1, 6, 3, 10, 1, 1, 3};
//usmHMAC128SHA224AuthProtocol OID (1.3.6.1.6.3.10.1.1.4)
const uint8_t usmHMAC128SHA224AuthProtocolOid[9] = {43, 6, 1, 6, 3, 10, 1, 1, 4};
//usmHMAC192SHA256AuthProtocol OID (1.3.6.1.6.3.10.1.1.5)
const uint8_t usmHMAC192SHA256AuthProtocolOid[9] = {43, 6, 1, 6, 3, 10, 1, 1, 5};
//usmHMAC256SHA384AuthProtocol OID (1.3.6.1.6.3.10.1.1.6)
const uint8_t usmHMAC256SHA384AuthProtocolOid[9] = {43, 6, 1, 6, 3, 10, 1, 1, 6};
//usmHMAC384SHA512AuthProtocol OID (1.3.6.1.6.3.10.1.1.7)
const uint8_t usmHMAC384SHA512AuthProtocolOid[9] = {43, 6, 1, 6, 3, 10, 1, 1, 7};

//usmNoPrivProtocol OID (1.3.6.1.6.3.10.1.2.1)
const uint8_t usmNoPrivProtocolOid[9] = {43, 6, 1, 6, 3, 10, 1, 2, 1};
//usmDESPrivProtocol OID (1.3.6.1.6.3.10.1.2.2)
const uint8_t usmDESPrivProtocolOid[9] = {43, 6, 1, 6, 3, 10, 1, 2, 2};
//usmAesCfb128Protocol OID (1.3.6.1.6.3.10.1.2.4)
const uint8_t usmAesCfb128ProtocolOid[9] = {43, 6, 1, 6, 3, 10, 1, 2, 4};

//usmUserEntry OID (1.3.6.1.6.3.15.1.2.2.1)
const uint8_t usmUserEntryOid[10] = {43, 6, 1, 6, 3, 15, 1, 2, 2, 1};


/**
 * @brief SNMP USM MIB module initialization
 * @return Error code
 **/

error_t snmpUsmMibInit(void)
{
   //Debug message
   TRACE_INFO("Initializing SNMP-USM-MIB base...\r\n");

   //Clear SNMP USM MIB base
   osMemset(&snmpUsmMibBase, 0, sizeof(snmpUsmMibBase));

   //usmUserSpinLock object
   snmpUsmMibBase.usmUserSpinLock = netGetRandRange(1, INT32_MAX);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Load SNMP USM MIB module
 * @param[in] context Pointer to the SNMP agent context
 * @return Error code
 **/

error_t snmpUsmMibLoad(void *context)
{
   //Register SNMP agent context
   snmpUsmMibBase.context = context;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Unload SNMP USM MIB module
 * @param[in] context Pointer to the SNMP agent context
 **/

void snmpUsmMibUnload(void *context)
{
   //Unregister SNMP agent context
   snmpUsmMibBase.context = NULL;
}


/**
 * @brief Lock SNMP USM MIB base
 **/

void snmpUsmMibLock(void)
{
   //Clear temporary user
   osMemset(&snmpUsmMibBase.tempUser, 0, sizeof(SnmpUserEntry));
}


/**
 * @brief Unlock SNMP USM MIB base
 **/

void snmpUsmMibUnlock(void)
{
   //Clear temporary user
   osMemset(&snmpUsmMibBase.tempUser, 0, sizeof(SnmpUserEntry));
}


/**
 * @brief Set usmUserSpinLock object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t snmpUsmMibSetUserSpinLock(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
   //Test and increment spin lock
   return mibTestAndIncSpinLock(&snmpUsmMibBase.usmUserSpinLock,
      value->integer, commit);
}


/**
 * @brief Get usmUserSpinLock object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpUsmMibGetUserSpinLock(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   //Get the current value of the spin lock
   value->integer = snmpUsmMibBase.usmUserSpinLock;

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Set usmUserEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[in] value Object value
 * @param[in] valueLen Length of the object value, in bytes
 * @param[in] commit This flag tells whether the changes shall be committed
 *   to the MIB base
 * @return Error code
 **/

error_t snmpUsmMibSetUserEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, const MibVariant *value, size_t valueLen, bool_t commit)
{
#if (SNMP_USM_MIB_SET_SUPPORT == ENABLED)
   error_t error;
   size_t n;
   uint8_t userEngineId[SNMP_MAX_CONTEXT_ENGINE_SIZE];
   size_t userEngineIdLen;
   char_t userName[SNMP_MAX_USER_NAME_LEN + 1];
   SnmpAgentContext *context;
   SnmpUserEntry *user;

   //Point to the instance identifier
   n = object->oidLen;

   //usmUserEngineID is used as 1st instance identifier
   error = mibDecodeOctetString(oid, oidLen, &n, userEngineId,
      SNMP_MAX_CONTEXT_ENGINE_SIZE, &userEngineIdLen, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //Accept malformed SNMP engine IDs
   if(n < oidLen && oid[n] == 0)
      n++;

   //usmUserName is used as 2nd instance identifier
   error = mibDecodeString(oid, oidLen, &n, userName,
      SNMP_MAX_USER_NAME_LEN, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Make sure the user name is acceptable
   if(userName[0] == '\0')
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpUsmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Retrieve the security profile of the specified user
   user = snmpFindUserEntry(context, userName, osStrlen(userName));

   //usmUserCloneFrom object?
   if(!osStrcmp(object->name, "usmUserCloneFrom"))
   {
      SnmpUserEntry *cloneFromUser;

      //Check the length of the OID
      if(valueLen <= sizeof(usmUserEntryOid))
         return ERROR_INCONSISTENT_VALUE;

      //The OID must point to another conceptual row in the usmUserTable
      if(osMemcmp(value->oid, usmUserEntryOid, sizeof(usmUserEntryOid)))
         return ERROR_INCONSISTENT_VALUE;

      //Point to the instance identifier
      n = sizeof(usmUserEntryOid) + 1;

      //usmUserEngineID is used as 1st instance identifier
      error = mibDecodeOctetString(value->oid, valueLen, &n, userEngineId,
         SNMP_MAX_CONTEXT_ENGINE_SIZE, &userEngineIdLen, FALSE);
      //Invalid instance identifier?
      if(error)
         return error;

      //usmUserName is used as 2nd instance identifier
      error = mibDecodeString(value->oid, valueLen, &n, userName,
         SNMP_MAX_USER_NAME_LEN, FALSE);
      //Invalid instance identifier?
      if(error)
         return error;

      //Retrieve the security profile of the clone-from user
      cloneFromUser = snmpFindUserEntry(context, userName, osStrlen(userName));

      //The cloning process fails with an inconsistentName error if the
      //conceptual row representing the clone-from user does not exist or
      //is not in an active state when the cloning process is invoked
      if(cloneFromUser == NULL || cloneFromUser->status != MIB_ROW_STATUS_ACTIVE)
         return ERROR_INCONSISTENT_VALUE;

      //Check whether the row has already been instantiated
      if(user != NULL)
      {
         //Commit phase?
         if(commit)
         {
            //The first time an instance of this object is set by a management
            //operation, the cloning process is invoked
            if(user->mode == SNMP_ACCESS_NONE)
            {
               //Clone the security parameters of the specified user
               snmpCloneSecurityParameters(user, cloneFromUser);
            }
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //The first time an instance of this object is set by a management
            //operation, the cloning process is invoked
            snmpCloneSecurityParameters(&snmpUsmMibBase.tempUser, cloneFromUser);
         }
      }
   }
   //usmUserAuthProtocol object?
   else if(!osStrcmp(object->name, "usmUserAuthProtocol"))
   {
      //Check whether the user name exists
      if(user != NULL)
      {
         //If a set operation tries to change the value of an existing instance
         //of this object to any value other than usmNoAuthProtocol, then an
         //inconsistentValue error must be returned
         if(oidComp(value->oid, valueLen, usmNoAuthProtocolOid,
            sizeof(usmNoAuthProtocolOid)))
         {
            return ERROR_INCONSISTENT_VALUE;
         }

         //If a set operation tries to set the value to the usmNoAuthProtocol
         //while the usmUserPrivProtocol value in the same row is not equal to
         //usmNoPrivProtocol, then an inconsistentValue error must be returned
         if(user->privProtocol != SNMP_PRIV_PROTOCOL_NONE)
            return ERROR_INCONSISTENT_VALUE;

         //Commit phase?
         if(commit)
         {
            //Once instantiated, the value of such an instance of this object
            //can only be changed via a set operation to the value of the
            //usmNoAuthProtocol
            user->authProtocol = SNMP_AUTH_PROTOCOL_NONE;
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //The usmUserAuthProtocol object specifies the type of authentication
            //protocol which is used
            if(!oidComp(value->oid, valueLen, usmNoAuthProtocolOid,
               sizeof(usmNoAuthProtocolOid)))
            {
               //No authentication
            }
            else if(!oidComp(value->oid, valueLen, usmHMACMD5AuthProtocolOid,
               sizeof(usmHMACMD5AuthProtocolOid)))
            {
               //HMAC-MD5-96 authentication protocol
            }
            else if(!oidComp(value->oid, valueLen, usmHMACSHAAuthProtocolOid,
               sizeof(usmHMACSHAAuthProtocolOid)))
            {
               //HMAC-SHA-1-96 authentication protocol
            }
            else if(!oidComp(value->oid, valueLen, usmHMAC128SHA224AuthProtocolOid,
               sizeof(usmHMAC128SHA224AuthProtocolOid)))
            {
               //HMAC-SHA-224-128 authentication protocol
            }
            else if(!oidComp(value->oid, valueLen, usmHMAC192SHA256AuthProtocolOid,
               sizeof(usmHMAC192SHA256AuthProtocolOid)))
            {
               //HMAC-SHA-256-192 authentication protocol
            }
            else if(!oidComp(value->oid, valueLen, usmHMAC256SHA384AuthProtocolOid,
               sizeof(usmHMAC256SHA384AuthProtocolOid)))
            {
               //HMAC-SHA-384-256 authentication protocol
            }
            else if(!oidComp(value->oid, valueLen, usmHMAC384SHA512AuthProtocolOid,
               sizeof(usmHMAC384SHA512AuthProtocolOid)))
            {
               //HMAC-SHA-512-384 authentication protocol
            }
            else
            {
               //If an initial set operation (at row creation time) tries to
               //set a value for an unknown or unsupported protocol, then a
               //wrongValue error must be returned
               return ERROR_WRONG_VALUE;
            }
         }
      }
   }
   //usmUserAuthKeyChange or usmUserOwnAuthKeyChange object?
   else if(!osStrcmp(object->name, "usmUserAuthKeyChange") ||
      !osStrcmp(object->name, "usmUserOwnAuthKeyChange"))
   {
      const HashAlgo *hashAlgo;

      //Unknown user name?
      if(user == NULL)
         return ERROR_INSTANCE_NOT_FOUND;

      //usmUserOwnAuthKeyChange object?
      if(!osStrcmp(object->name, "usmUserOwnAuthKeyChange"))
      {
         //When the user name of the requester is not the same as the umsUserName
         //that indexes the row, then a noAccess error must be returned
         if(osStrcmp(user->name, context->user.name))
            return ERROR_ACCESS_DENIED;

         //When a set is received and the security model in use is not USM, then
         //a noAccess error must be returned
         if(context->request.msgSecurityModel != SNMP_SECURITY_MODEL_USM)
            return ERROR_ACCESS_DENIED;
      }

      //Get the hash algorithm to be used to update the key
      hashAlgo = snmpGetHashAlgo(user->authProtocol);

      //Invalid authentication protocol?
      if(hashAlgo == NULL)
         return ERROR_WRITE_FAILED;

      //The value of an instance of this object is the concatenation of
      //two components of fixed length: first a random component and then
      //a delta component
      if(valueLen != (hashAlgo->digestSize * 2))
         return ERROR_WRONG_LENGTH;

      //Commit phase?
      if(commit)
      {
         //Update the localized authentication key (Kul)
         snmpChangeKey(hashAlgo, value->octetString,
            value->octetString + hashAlgo->digestSize, &user->localizedAuthKey);

         //The raw authentication key (Ku) is no longer valid
         osMemset(&user->rawAuthKey, 0, sizeof(SnmpKey));
      }
   }
   //usmUserPrivProtocol object?
   else if(!osStrcmp(object->name, "usmUserPrivProtocol"))
   {
      //Check whether the user name exists
      if(user != NULL)
      {
         //If a set operation tries to change the value of an existing instance
         //of this object to any value other than usmNoPrivProtocol, then an
         //inconsistentValue error must be returned
         if(oidComp(value->oid, valueLen, usmNoPrivProtocolOid,
            sizeof(usmNoPrivProtocolOid)))
         {
            return ERROR_INCONSISTENT_VALUE;
         }

         //Commit phase?
         if(commit)
         {
            //Once instantiated, the value of such an instance of this object
            //can only be changed via a set operation to the value of the
            //usmNoPrivProtocol
            user->privProtocol = SNMP_PRIV_PROTOCOL_NONE;
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //The usmUserPrivProtocol object specifies the type of privacy
            //protocol which is used
            if(!oidComp(value->oid, valueLen, usmNoPrivProtocolOid,
               sizeof(usmNoPrivProtocolOid)))
            {
               //No privacy
            }
            else if(!oidComp(value->oid, valueLen, usmDESPrivProtocolOid,
               sizeof(usmDESPrivProtocolOid)))
            {
               //DES-CBC privacy protocol
            }
            else if(!oidComp(value->oid, valueLen, usmAesCfb128ProtocolOid,
               sizeof(usmAesCfb128ProtocolOid)))
            {
               //AES-128-CFB privacy protocol
            }
            else
            {
               //If an initial set operation (at row creation time) tries to
               //set a value for an unknown or unsupported protocol, then a
               //wrongValue error must be returned
               return ERROR_WRONG_VALUE;
            }
         }
      }
   }
   //usmUserPrivKeyChange or usmUserOwnPrivKeyChangeobject?
   else if(!osStrcmp(object->name, "usmUserPrivKeyChange") ||
      !osStrcmp(object->name, "usmUserOwnPrivKeyChange"))
   {
      const HashAlgo *hashAlgo;

      //Unknown user name?
      if(user == NULL)
         return ERROR_INSTANCE_NOT_FOUND;

      //usmUserOwnPrivKeyChange object?
      if(!osStrcmp(object->name, "usmUserOwnPrivKeyChange"))
      {
         //When the user name of the requester is not the same as the umsUserName
         //that indexes the row, then a noAccess error must be returned
         if(osStrcmp(user->name, context->user.name))
            return ERROR_ACCESS_DENIED;

         //When a set is received and the security model in use is not USM, then
         //a noAccess error must be returned
         if(context->request.msgSecurityModel != SNMP_SECURITY_MODEL_USM)
            return ERROR_ACCESS_DENIED;
      }

      //Get the hash algorithm to be used to update the key
      hashAlgo = snmpGetHashAlgo(user->authProtocol);

      //Invalid authentication protocol?
      if(hashAlgo == NULL)
         return ERROR_WRITE_FAILED;

      //The value of an instance of this object is the concatenation of
      //two components of fixed length: first a random component and then
      //a delta component
      if(valueLen != (hashAlgo->digestSize * 2))
         return ERROR_WRONG_LENGTH;

      //Commit phase?
      if(commit)
      {
         //Update the localized privacy key (Kul)
         snmpChangeKey(hashAlgo, value->octetString,
            value->octetString + hashAlgo->digestSize, &user->localizedPrivKey);

         //The raw privacy key (Ku) is no longer valid
         osMemset(&user->rawPrivKey, 0, sizeof(SnmpKey));
      }
   }
   //usmUserPublic object?
   else if(!osStrcmp(object->name, "usmUserPublic"))
   {
      //Check the length of the public value
      if(valueLen > SNMP_MAX_PUBLIC_VALUE_SIZE)
         return ERROR_WRONG_LENGTH;

      //Check whether the user name exists
      if(user != NULL)
      {
         //Commit phase?
         if(commit)
         {
            //The usmUserPublic can be written as part of the procedure for
            //changing a user's secret authentication and/or privacy key,
            //and later read to determine whether the change of the secret
            //was effected
            osMemcpy(user->publicValue, value->octetString, valueLen);

            //Update the length of the public value
            user->publicValueLen = valueLen;
         }
      }
      else
      {
         //Prepare phase?
         if(!commit)
         {
            //Save the value of the public value for later use
            osMemcpy(snmpUsmMibBase.tempUser.publicValue, value->octetString,
               valueLen);

            //Save the length of the public value
            snmpUsmMibBase.tempUser.publicValueLen = valueLen;
         }
      }
   }
   //usmUserStorageType object?
   else if(!osStrcmp(object->name, "usmUserStorageType"))
   {
      //The usmUserStorageType object specifies the storage type for this
      //conceptual row
      if(value->integer != MIB_STORAGE_TYPE_OTHER &&
         value->integer != MIB_STORAGE_TYPE_VOLATILE &&
         value->integer != MIB_STORAGE_TYPE_NON_VOLATILE &&
         value->integer != MIB_STORAGE_TYPE_PERMANENT &&
         value->integer != MIB_STORAGE_TYPE_READ_ONLY)
      {
         return ERROR_WRONG_VALUE;
      }
   }
   //usmUserStatus object?
   else if(!osStrcmp(object->name, "usmUserStatus"))
   {
      MibRowStatus status;

      //Get row status
      status = (MibRowStatus) value->integer;

      //Check the value specified by the set operation
      if(status == MIB_ROW_STATUS_ACTIVE ||
         status == MIB_ROW_STATUS_NOT_IN_SERVICE)
      {
         //Unknown user name?
         if(user == NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Commit phase?
         if(commit)
         {
            //The first time an instance of this object is set by a management
            //operation, the cloning process is invoked
            if(user->mode == SNMP_ACCESS_NONE)
            {
               //Valid clone-from user?
               if(snmpUsmMibBase.tempUser.mode != SNMP_ACCESS_NONE)
               {
                  //Copy the security profile of the clone-from user
                  snmpCloneSecurityParameters(user, &snmpUsmMibBase.tempUser);
               }
            }

            //When the agent processes the set operation, it verifies that it
            //has sufficient information to make the conceptual row available
            //for use by the managed device
            if(user->mode == SNMP_ACCESS_NONE)
               return ERROR_INCONSISTENT_VALUE;

            //Update the status of the conceptual row
            user->status = status;
         }
      }
      else if(status == MIB_ROW_STATUS_CREATE_AND_GO)
      {
         //User name already in use?
         if(user != NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Create a security profile for the new user
         user = snmpCreateUserEntry(context);
         //Unable to create a new user?
         if(user == NULL)
            return ERROR_WRITE_FAILED;

         //Commit phase?
         if(commit)
         {
            //Valid clone-from user?
            if(snmpUsmMibBase.tempUser.mode != SNMP_ACCESS_NONE)
            {
               //Clear the security profile of the user
               osMemset(user, 0, sizeof(SnmpUserEntry));
               //Save user name
               osStrcpy(user->name, userName);

               //Valid public value specified?
               if(snmpUsmMibBase.tempUser.publicValueLen > 0)
               {
                  //Copy the public value
                  osMemcpy(user->publicValue, snmpUsmMibBase.tempUser.publicValue,
                     snmpUsmMibBase.tempUser.publicValueLen);

                  //Set the length of the public value
                  user->publicValueLen = snmpUsmMibBase.tempUser.publicValueLen;
               }

               //Copy the security profile of the clone-from user
               snmpCloneSecurityParameters(user, &snmpUsmMibBase.tempUser);

               //The conceptual row is now available for use by the managed device
               user->status = MIB_ROW_STATUS_ACTIVE;
            }
            else
            {
               //The newly created row cannot be made active
               return ERROR_INCONSISTENT_VALUE;
            }
         }
      }
      else if(status == MIB_ROW_STATUS_CREATE_AND_WAIT)
      {
         //User name already in use?
         if(user != NULL)
            return ERROR_INCONSISTENT_VALUE;

         //Create a security profile for the new user
         user = snmpCreateUserEntry(context);
         //Unable to create a new user?
         if(user == NULL)
            return ERROR_WRITE_FAILED;

         //Commit phase?
         if(commit)
         {
            //Clear the security profile of the user
            osMemset(user, 0, sizeof(SnmpUserEntry));
            //Save user name
            osStrcpy(user->name, userName);

            //Valid public value specified?
            if(snmpUsmMibBase.tempUser.publicValueLen > 0)
            {
               //Copy the public value
               osMemcpy(user->publicValue, snmpUsmMibBase.tempUser.publicValue,
                  snmpUsmMibBase.tempUser.publicValueLen);

               //Set the length of the public value
               user->publicValueLen = snmpUsmMibBase.tempUser.publicValueLen;
            }

            //Check whether the cloning process has been invoked
            if(user->mode != SNMP_ACCESS_NONE)
            {
               //Copy the security profile of the clone-from user
               snmpCloneSecurityParameters(user, &snmpUsmMibBase.tempUser);

               //Instances of all corresponding columns are now configured
               user->status = MIB_ROW_STATUS_NOT_IN_SERVICE;
            }
            else
            {
               //Initialize columns with default values
               user->authProtocol = SNMP_AUTH_PROTOCOL_NONE;
               user->privProtocol = SNMP_PRIV_PROTOCOL_NONE;

               //Until instances of all corresponding columns are appropriately
               //configured, the value of the corresponding instance of the
               //usmUserStatus column is notReady
               user->status = MIB_ROW_STATUS_NOT_READY;
            }
         }
      }
      else if(status == MIB_ROW_STATUS_DESTROY)
      {
         //Check whether the user name exists
         if(user != NULL)
         {
            //Commit phase?
            if(commit)
            {
               //Clear the security profile of the user
               osMemset(user, 0, sizeof(SnmpUserEntry));

               //Delete the conceptual row from the table
               user->status = MIB_ROW_STATUS_UNUSED;
            }
         }
      }
      else
      {
         //Unsupported action
         return ERROR_INCONSISTENT_VALUE;
      }
   }
   //Unknown object?
   else
   {
      //The specified object does not exist
      return ERROR_OBJECT_NOT_FOUND;
   }

   //Successful processing
   return NO_ERROR;
#else
   //SET operation is not supported
   return ERROR_WRITE_FAILED;
#endif
}


/**
 * @brief Get usmUserEntry object value
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier (object name and instance identifier)
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] value Object value
 * @param[in,out] valueLen Length of the object value, in bytes
 * @return Error code
 **/

error_t snmpUsmMibGetUserEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, MibVariant *value, size_t *valueLen)
{
   error_t error;
   size_t n;
   uint8_t userEngineId[SNMP_MAX_CONTEXT_ENGINE_SIZE];
   size_t userEngineIdLen;
   char_t userName[SNMP_MAX_USER_NAME_LEN + 1];
   SnmpAgentContext *context;
   SnmpUserEntry *user;

   //Point to the instance identifier
   n = object->oidLen;

   //usmUserEngineID is used as 1st instance identifier
   error = mibDecodeOctetString(oid, oidLen, &n, userEngineId,
      SNMP_MAX_CONTEXT_ENGINE_SIZE, &userEngineIdLen, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //usmUserName is used as 2nd instance identifier
   error = mibDecodeString(oid, oidLen, &n, userName,
      SNMP_MAX_USER_NAME_LEN, FALSE);
   //Invalid instance identifier?
   if(error)
      return error;

   //Sanity check
   if(n != oidLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpUsmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //Check the length of the SNMP engine ID
   if(userEngineIdLen != context->contextEngineLen)
      return ERROR_INSTANCE_NOT_FOUND;

   //Check SNMP engine ID
   if(osMemcmp(userEngineId, context->contextEngine, userEngineIdLen))
      return ERROR_INSTANCE_NOT_FOUND;

   //Retrieve the security profile of the specified user
   user = snmpFindUserEntry(context, userName, osStrlen(userName));
   //Unknown user name?
   if(user == NULL)
      return ERROR_INSTANCE_NOT_FOUND;

   //usmUserSecurityName object?
   if(!osStrcmp(object->name, "usmUserSecurityName"))
   {
      //The security name is the same as the user name
      n = osStrlen(user->name);

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= n)
      {
         //Copy object value
         osMemcpy(value->octetString, user->name, n);
         //Return object length
         *valueLen = n;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //usmUserCloneFrom object?
   else if(!osStrcmp(object->name, "usmUserCloneFrom"))
   {
      //When this object is read, the ZeroDotZero OID is returned
      uint8_t zeroDotZeroOid[] = {0};

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= sizeof(zeroDotZeroOid))
      {
         //Copy object value
         osMemcpy(value->octetString, zeroDotZeroOid, sizeof(zeroDotZeroOid));
         //Return object length
         *valueLen = sizeof(zeroDotZeroOid);
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //usmUserAuthProtocol object?
   else if(!osStrcmp(object->name, "usmUserAuthProtocol"))
   {
      size_t authProtocolLen;
      const uint8_t *authProtocol;

      //Check the type of authentication protocol which is used
      switch(user->authProtocol)
      {
      //HMAC-MD5-96 authentication protocol?
      case SNMP_AUTH_PROTOCOL_MD5:
         authProtocol = usmHMACMD5AuthProtocolOid;
         authProtocolLen = sizeof(usmHMACMD5AuthProtocolOid);
         break;
      //HMAC-SHA-1-96 authentication protocol?
      case SNMP_AUTH_PROTOCOL_SHA1:
         authProtocol = usmHMACSHAAuthProtocolOid;
         authProtocolLen = sizeof(usmHMACSHAAuthProtocolOid);
         break;
      //HMAC-SHA-224-128 authentication protocol?
      case SNMP_AUTH_PROTOCOL_SHA224:
         authProtocol = usmHMAC128SHA224AuthProtocolOid;
         authProtocolLen = sizeof(usmHMAC128SHA224AuthProtocolOid);
         break;
      //HMAC-SHA-256-192 authentication protocol?
      case SNMP_AUTH_PROTOCOL_SHA256:
         authProtocol = usmHMAC192SHA256AuthProtocolOid;
         authProtocolLen = sizeof(usmHMAC192SHA256AuthProtocolOid);
         break;
      //HMAC-SHA-384-256 authentication protocol?
      case SNMP_AUTH_PROTOCOL_SHA384:
         authProtocol = usmHMAC256SHA384AuthProtocolOid;
         authProtocolLen = sizeof(usmHMAC256SHA384AuthProtocolOid);
         break;
      //HMAC-SHA-512-384 authentication protocol?
      case SNMP_AUTH_PROTOCOL_SHA512:
         authProtocol = usmHMAC384SHA512AuthProtocolOid;
         authProtocolLen = sizeof(usmHMAC384SHA512AuthProtocolOid);
         break;
      //No authentication?
      default:
         authProtocol = usmNoAuthProtocolOid;
         authProtocolLen = sizeof(usmNoAuthProtocolOid);
         break;
      }

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= authProtocolLen)
      {
         //Copy object value
         osMemcpy(value->octetString, authProtocol, authProtocolLen);
         //Return object length
         *valueLen = authProtocolLen;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //usmUserAuthKeyChange object?
   else if(!osStrcmp(object->name, "usmUserAuthKeyChange"))
   {
      //When this object is read, the zero-length (empty) string is returned
      *valueLen = 0;
   }
   //usmUserOwnAuthKeyChange object?
   else if(!osStrcmp(object->name, "usmUserOwnAuthKeyChange"))
   {
      //When this object is read, the zero-length (empty) string is returned
      *valueLen = 0;
   }
   //usmUserPrivProtocol object?
   else if(!osStrcmp(object->name, "usmUserPrivProtocol"))
   {
      size_t privProtocolLen;
      const uint8_t *privProtocol;

      //Check the type of privacy protocol which is used
      switch(user->privProtocol)
      {
      //DES-CBC privacy protocol?
      case SNMP_PRIV_PROTOCOL_DES:
         privProtocol = usmDESPrivProtocolOid;
         privProtocolLen = sizeof(usmDESPrivProtocolOid);
         break;
      //AES-128-CFB privacy protocol?
      case SNMP_PRIV_PROTOCOL_AES:
         privProtocol = usmAesCfb128ProtocolOid;
         privProtocolLen = sizeof(usmAesCfb128ProtocolOid);
         break;
      //No privacy?
      default:
         privProtocol = usmNoPrivProtocolOid;
         privProtocolLen = sizeof(usmNoPrivProtocolOid);
         break;
      }

      //Make sure the buffer is large enough to hold the entire object
      if(*valueLen >= privProtocolLen)
      {
         //Copy object value
         osMemcpy(value->octetString, privProtocol, privProtocolLen);
         //Return object length
         *valueLen = privProtocolLen;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //usmUserPrivKeyChange object?
   else if(!osStrcmp(object->name, "usmUserPrivKeyChange"))
   {
      //When this object is read, the zero-length (empty) string is returned
      *valueLen = 0;
   }
   //usmUserOwnPrivKeyChange object?
   else if(!osStrcmp(object->name, "usmUserOwnPrivKeyChange"))
   {
      //When this object is read, the zero-length (empty) string is returned
      *valueLen = 0;
   }
   //usmUserPublic object?
   else if(!osStrcmp(object->name, "usmUserPublic"))
   {
      //Make sure the buffer is large enough to hold the public value
      if(*valueLen >= user->publicValueLen)
      {
         //The public value can be read to determine whether the change of
         //the secret was effected
         osMemcpy(value->octetString, user->publicValue, user->publicValueLen);

         //Return object length
         *valueLen = user->publicValueLen;
      }
      else
      {
         //Report an error
         error = ERROR_BUFFER_OVERFLOW;
      }
   }
   //usmUserStorageType object?
   else if(!osStrcmp(object->name, "usmUserStorageType"))
   {
      //Get the storage type for this conceptual row
      value->integer = MIB_STORAGE_TYPE_VOLATILE;
   }
   //usmUserStatus object?
   else if(!osStrcmp(object->name, "usmUserStatus"))
   {
      //Get the status of this conceptual row
      value->integer = user->status;
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
 * @brief Get next usmUserEntry object
 * @param[in] object Pointer to the MIB object descriptor
 * @param[in] oid Object identifier
 * @param[in] oidLen Length of the OID, in bytes
 * @param[out] nextOid OID of the next object in the MIB
 * @param[out] nextOidLen Length of the next object identifier, in bytes
 * @return Error code
 **/

error_t snmpUsmMibGetNextUserEntry(const MibObject *object, const uint8_t *oid,
   size_t oidLen, uint8_t *nextOid, size_t *nextOidLen)
{
   error_t error;
   uint_t i;
   size_t n;
   bool_t acceptable;
   SnmpAgentContext *context;
   SnmpUserEntry *entry;
   SnmpUserEntry *nextEntry;

   //Initialize variables
   nextEntry = NULL;

   //Point to the SNMP agent context
   context = (SnmpAgentContext *) snmpUsmMibBase.context;
   //Sanity check
   if(context == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Make sure the buffer is large enough to hold the OID prefix
   if(*nextOidLen < object->oidLen)
      return ERROR_BUFFER_OVERFLOW;

   //Copy OID prefix
   osMemcpy(nextOid, object->oid, object->oidLen);

   //Loop through the list of users
   for(i = 0; i < SNMP_AGENT_MAX_USERS; i++)
   {
      //Point to the current entry
      entry = &context->userTable[i];

      //Check the status of the row
      if(entry->status != MIB_ROW_STATUS_UNUSED)
      {
         //Append the instance identifier to the OID prefix
         n = object->oidLen;

         //usmUserEngineID is used as 1st instance identifier
         error = mibEncodeOctetString(nextOid, *nextOidLen, &n,
            context->contextEngine, context->contextEngineLen, FALSE);
         //Any error to report?
         if(error)
            return error;

         //usmUserName is used as 2nd instance identifier
         error = mibEncodeString(nextOid, *nextOidLen, &n, entry->name, FALSE);
         //Any error to report?
         if(error)
            return error;

         //Check whether the resulting object identifier lexicographically
         //follows the specified OID
         if(oidComp(nextOid, n, oid, oidLen) > 0)
         {
            //Perform lexicographic comparison
            if(nextEntry == NULL)
            {
               acceptable = TRUE;
            }
            else if(osStrlen(entry->name) < osStrlen(nextEntry->name))
            {
               acceptable = TRUE;
            }
            else if(osStrlen(entry->name) > osStrlen(nextEntry->name))
            {
               acceptable = FALSE;
            }
            else if(osStrcmp(entry->name, nextEntry->name) < 0)
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
               nextEntry = entry;
         }
      }
   }

   //The specified OID does not lexicographically precede the name
   //of some object?
   if(nextEntry == NULL)
      return ERROR_OBJECT_NOT_FOUND;

   //Append the instance identifier to the OID prefix
   n = object->oidLen;

   //usmUserEngineID is used as 1st instance identifier
   error = mibEncodeOctetString(nextOid, *nextOidLen, &n,
      context->contextEngine, context->contextEngineLen, FALSE);
   //Any error to report?
   if(error)
      return error;

   //usmUserName is used as 2nd instance identifier
   error = mibEncodeString(nextOid, *nextOidLen, &n, nextEntry->name, FALSE);
   //Any error to report?
   if(error)
      return error;

   //Save the length of the resulting object identifier
   *nextOidLen = n;
   //Next object found
   return NO_ERROR;
}

#endif
