/**
 * @file snmp_agent_usm.c
 * @brief User-based Security Model (USM) for SNMPv3
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
 * @section Description
 *
 * This module implements the User-based Security Model (USM) for Simple
 * Network Management Protocol (SNMP) version 3. Refer to the following
 * RFCs for complete details:
 * - RFC 3414: User-based Security Model (USM) for SNMPv3
 * - RFC 3826: AES Cipher Algorithm in the SNMP User-based Security Model
 * - RFC 7860: HMAC-SHA-2 Authentication Protocols in the User-based Security Model
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL SNMP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "snmp/snmp_agent.h"
#include "snmp/snmp_agent_usm.h"
#include "core/crypto.h"
#include "encoding/asn1.h"
#include "mac/hmac.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (SNMP_AGENT_SUPPORT == ENABLED && SNMP_V3_SUPPORT == ENABLED)

//usmStatsUnsupportedSecLevels.0 object (1.3.6.1.6.3.15.1.1.1.0)
const uint8_t usmStatsUnsupportedSecLevelsObject[10] = {43, 6, 1, 6, 3, 15, 1, 1, 1, 0};
//usmStatsNotInTimeWindows.0 object (1.3.6.1.6.3.15.1.1.2.0)
const uint8_t usmStatsNotInTimeWindowsObject[10] = {43, 6, 1, 6, 3, 15, 1, 1, 2, 0};
//usmStatsUnknownUserNames.0 object (1.3.6.1.6.3.15.1.1.3.0)
const uint8_t usmStatsUnknownUserNamesObject[10] = {43, 6, 1, 6, 3, 15, 1, 1, 3, 0};
//usmStatsUnknownEngineIDs.0 object (1.3.6.1.6.3.15.1.1.4.0)
const uint8_t usmStatsUnknownEngineIdsObject[10] = {43, 6, 1, 6, 3, 15, 1, 1, 4, 0};
//usmStatsWrongDigests.0 object (1.3.6.1.6.3.15.1.1.5.0)
const uint8_t usmStatsWrongDigestsObject[10] = {43, 6, 1, 6, 3, 15, 1, 1, 5, 0};
//usmStatsDecryptionErrors.0 object (1.3.6.1.6.3.15.1.1.6.0)
const uint8_t usmStatsDecryptionErrorsObject[10] = {43, 6, 1, 6, 3, 15, 1, 1, 6, 0};


/**
 * @brief Create a new user entry
 * @param[in] context Pointer to the SNMP agent context
 * @return Pointer to the newly created entry
 **/

SnmpUserEntry *snmpCreateUserEntry(SnmpAgentContext *context)
{
   uint_t i;
   SnmpUserEntry *entry;

   //Initialize pointer
   entry = NULL;

   //Sanity check
   if(context != NULL)
   {
      //Loop through the list of users
      for(i = 0; i < SNMP_AGENT_MAX_USERS; i++)
      {
         //Check current status
         if(context->userTable[i].status == MIB_ROW_STATUS_UNUSED)
         {
            //An unused entry has been found
            entry = &context->userTable[i];
            //We are done
            break;
         }
      }

      //Check whether the user table runs out of space
      if(entry == NULL)
      {
         //Loop through the list of users
         for(i = 0; i < SNMP_AGENT_MAX_USERS; i++)
         {
            //Check current status
            if(context->userTable[i].status == MIB_ROW_STATUS_NOT_READY)
            {
               //Reuse the current entry
               entry = &context->userTable[i];
               //We are done
               break;
            }
         }
      }
   }

   //Return a pointer to the newly created entry
   return entry;
}


/**
 * @brief Search the user table for a given user name
 * @param[in] context Pointer to the SNMP agent context
 * @param[in] name Pointer to the user name
 * @param[in] length Length of the user name
 * @return Pointer to the matching entry
 **/

SnmpUserEntry *snmpFindUserEntry(SnmpAgentContext *context,
   const char_t *name, size_t length)
{
   uint_t i;
   SnmpUserEntry *entry;

   //Initialize pointer
   entry = NULL;

   //Sanity check
   if(context != NULL && name != NULL)
   {
      //Loop through the list of users
      for(i = 0; i < SNMP_AGENT_MAX_USERS; i++)
      {
         //Check current status
         if(context->userTable[i].status != MIB_ROW_STATUS_UNUSED)
         {
            //Check the length of the user name
            if(osStrlen(context->userTable[i].name) == length)
            {
               //Compare user names
               if(!osStrncmp(context->userTable[i].name, name, length))
               {
                  //A matching entry has been found
                  entry = &context->userTable[i];
                  //We are done
                  break;
               }
            }
         }
      }
   }

   //Return a pointer to the matching entry
   return entry;
}


/**
 * @brief Password to key algorithm
 * @param[in] authProtocol Authentication protocol (MD5, SHA-1, SHA-224,
 *   SHA-256, SHA384 or SHA512)
 * @param[in] password NULL-terminated string that contains the password
 * @param[out] key Pointer to the resulting key (Ku)
 * @return Error code
 **/

error_t snmpGenerateKey(SnmpAuthProtocol authProtocol, const char_t *password,
   SnmpKey *key)
{
   size_t i;
   size_t n;
   size_t passwordLen;
   const HashAlgo *hashAlgo;
   uint8_t hashContext[MAX_HASH_CONTEXT_SIZE];

   //Check parameters
   if(password == NULL || key == NULL)
      return ERROR_INVALID_PARAMETER;

   //Clear SNMP key
   osMemset(key, 0, sizeof(SnmpKey));

   //Get the hash algorithm to be used to generate the key
   hashAlgo = snmpGetHashAlgo(authProtocol);

   //Invalid authentication protocol?
   if(hashAlgo == NULL)
      return ERROR_INVALID_PARAMETER;

   //Retrieve the length of the password
   passwordLen = osStrlen(password);

   //SNMP implementations must ensure that passwords are at least 8 characters
   //in length (see RFC 3414 11.2)
   if(passwordLen < 8)
      return ERROR_INVALID_LENGTH;

   //Initialize hash context
   hashAlgo->init(hashContext);

   //Loop until we have done 1 megabyte
   for(i = 0; i < 1048576; i += n)
   {
      n = MIN(passwordLen, 1048576 - i);
      hashAlgo->update(hashContext, password, n);
   }

   //Finalize hash computation
   hashAlgo->final(hashContext, key->b);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Key localization algorithm
 * @param[in] authProtocol Authentication protocol (MD5, SHA-1, SHA-224,
 *   SHA-256, SHA384 or SHA512)
 * @param[in] engineId Pointer to the engine ID
 * @param[in] engineIdLen Length of the engine ID
 * @param[in] key Pointer to the key to be localized (Ku)
 * @param[out] localizedKey Pointer to the resulting key (Kul)
 * @return Error code
 **/

error_t snmpLocalizeKey(SnmpAuthProtocol authProtocol, const uint8_t *engineId,
   size_t engineIdLen, SnmpKey *key, SnmpKey *localizedKey)
{
   const HashAlgo *hashAlgo;
   uint8_t hashContext[MAX_HASH_CONTEXT_SIZE];

   //Check parameters
   if(engineId == NULL && engineIdLen > 0)
      return ERROR_INVALID_PARAMETER;
   if(key == NULL || localizedKey == NULL)
      return ERROR_INVALID_PARAMETER;

   //Get the hash algorithm to be used to generate the key
   hashAlgo = snmpGetHashAlgo(authProtocol);

   //Invalid authentication protocol?
   if(hashAlgo == NULL)
      return ERROR_INVALID_PARAMETER;

   //Localize the key with the engine ID
   hashAlgo->init(hashContext);
   hashAlgo->update(hashContext, key, hashAlgo->digestSize);
   hashAlgo->update(hashContext, engineId, engineIdLen);
   hashAlgo->update(hashContext, key, hashAlgo->digestSize);
   hashAlgo->final(hashContext, localizedKey->b);

   //Successful processing
   return NO_ERROR;
}


/**
 * @brief Change secret key
 * @param[in] hashAlgo Hash algorithm to be used
 * @param[in] random Pointer to the random component
 * @param[in] delta Pointer to the delta component
 * @param[in,out] key Pointer to the secret key K
 **/

void snmpChangeKey(const HashAlgo *hashAlgo, const uint8_t *random,
   const uint8_t *delta, SnmpKey *key)
{
   uint_t i;
   uint8_t hashContext[MAX_HASH_CONTEXT_SIZE];
   uint8_t digest[SNMP_MAX_KEY_SIZE];

   //The random component is appended to the existing value of the K, and the
   //result is input to the hash algorithm H to produce a digest value
   hashAlgo->init(hashContext);
   hashAlgo->update(hashContext, key, hashAlgo->digestSize);
   hashAlgo->update(hashContext, random, hashAlgo->digestSize);
   hashAlgo->final(hashContext, digest);

   //This digest value is XOR-ed with the unused portion of the delta component
   //to produce the new value of K
   for(i = 0; i < hashAlgo->digestSize; i++)
   {
      key->b[i] = digest[i] ^ delta[i];
   }
}


/**
 * @brief Clone security parameters
 * @param[in,out] user Security profile of the user
 * @param[in] cloneFromUser Security profile of the clone-from user
 **/

void snmpCloneSecurityParameters(SnmpUserEntry *user,
   const SnmpUserEntry *cloneFromUser)
{
   //Clone security parameters
   user->mode = cloneFromUser->mode;
   user->authProtocol = cloneFromUser->authProtocol;
   user->rawAuthKey = cloneFromUser->rawAuthKey;
   user->localizedAuthKey = cloneFromUser->localizedAuthKey;
   user->privProtocol = cloneFromUser->privProtocol;
   user->rawPrivKey = cloneFromUser->rawPrivKey;
   user->localizedPrivKey = cloneFromUser->localizedPrivKey;
}


/**
 * @brief Check security parameters
 * @param[in] user Security profile of the user
 * @param[in,out] message Pointer to the incoming SNMP message
 * @param[in] engineId Pointer to the authoritative engine ID
 * @param[in] engineIdLen Length of the authoritative engine ID
 * @return Error code
 **/

error_t snmpCheckSecurityParameters(const SnmpUserEntry *user,
   SnmpMessage *message, const uint8_t *engineId, size_t engineIdLen)
{
   //Check the length of the authoritative engine ID
   if(message->msgAuthEngineIdLen != engineIdLen)
      return ERROR_UNKNOWN_ENGINE_ID;

   //If the value of the msgAuthoritativeEngineID field is unknown, then an
   //error indication (unknownEngineID) is returned to the calling module
   if(osMemcmp(message->msgAuthEngineId, engineId, engineIdLen))
      return ERROR_UNKNOWN_ENGINE_ID;

   //If no information is available for the user, then an error indication
   //(unknownSecurityName) is returned to the calling module
   if(user == NULL || user->status != MIB_ROW_STATUS_ACTIVE)
      return ERROR_UNKNOWN_USER_NAME;

   //Check whether the securityLevel specifies that the message should
   //be authenticated
   if(user->authProtocol != SNMP_AUTH_PROTOCOL_NONE)
   {
      //Make sure the authFlag is set
      if((message->msgFlags & SNMP_MSG_FLAG_AUTH) == 0)
         return ERROR_UNSUPPORTED_SECURITY_LEVEL;
   }

   //Check whether the securityLevel specifies that the message should
   //be encrypted
   if(user->privProtocol != SNMP_PRIV_PROTOCOL_NONE)
   {
      //Make sure the privFlag is set
      if((message->msgFlags & SNMP_MSG_FLAG_PRIV) == 0)
         return ERROR_UNSUPPORTED_SECURITY_LEVEL;
   }

   //Security parameters are valid
   return NO_ERROR;
}


/**
 * @brief Refresh SNMP engine time
 * @param[in] context Pointer to the SNMP agent context
 **/

void snmpRefreshEngineTime(SnmpAgentContext *context)
{
   systime_t delta;
   int32_t newEngineTime;

   //Number of seconds elapsed since the last call
   delta = (osGetSystemTime() - context->systemTime) / 1000;
   //Increment SNMP engine time
   newEngineTime = context->engineTime + delta;

   //Check whether the SNMP engine time has rolled over
   if(newEngineTime < context->engineTime)
   {
      //If snmpEngineTime ever reaches its maximum value (2147483647), then
      //snmpEngineBoots is incremented as if the SNMP engine has re-booted
      //and snmpEngineTime is reset to zero and starts incrementing again
      context->engineBoots++;
      context->engineTime = 0;
   }
   else
   {
      //Update SNMP engine time
      context->engineTime = newEngineTime;
   }

   //Save timestamp
   context->systemTime += delta * 1000;
}


/**
 * @brief Replay protection
 * @param[in] context Pointer to the SNMP agent context
 * @param[in,out] message Pointer to the incoming SNMP message
 * @return Error code
 **/

error_t snmpCheckEngineTime(SnmpAgentContext *context, SnmpMessage *message)
{
   error_t error;

#if (SNMP_AGENT_INFORM_SUPPORT == ENABLED)
   //Check whether the discovery process is complete
   if(context->informContextEngineLen > 0)
   {
      //Compare engine IDs
      if(message->msgAuthEngineIdLen == context->informContextEngineLen)
      {
         if(!osMemcmp(message->msgAuthEngineId, context->informContextEngine,
            context->informContextEngineLen))
         {
            //We are done
            return NO_ERROR;
         }
      }
   }
#endif

   //Initialize status code
   error = NO_ERROR;

   //If any of the following conditions is true, then the message is
   //considered to be outside of the time window
   if(context->engineBoots == INT32_MAX)
   {
      //The local value of snmpEngineBoots is 2147483647
      error = ERROR_NOT_IN_TIME_WINDOW;
   }
   else if(context->engineBoots != message->msgAuthEngineBoots)
   {
      //The value of the msgAuthoritativeEngineBoots field differs from
      //the local value of snmpEngineBoots
      error = ERROR_NOT_IN_TIME_WINDOW;
   }
   else if((context->engineTime - message->msgAuthEngineTime) > SNMP_TIME_WINDOW ||
      (message->msgAuthEngineTime - context->engineTime) > SNMP_TIME_WINDOW)
   {
      //The value of the msgAuthoritativeEngineTime field differs from the
      //local notion of snmpEngineTime by more than +/- 150 seconds
      error = ERROR_NOT_IN_TIME_WINDOW;
   }

   //If the message is considered to be outside of the time window then an
   //error indication (notInTimeWindow) is returned to the calling module
   return error;
}


/**
 * @brief Authenticate outgoing SNMP message
 * @param[in] user Security profile of the user
 * @param[in,out] message Pointer to the outgoing SNMP message
 * @return Error code
 **/

error_t snmpAuthOutgoingMessage(const SnmpUserEntry *user, SnmpMessage *message)
{
   const HashAlgo *hashAlgo;
   size_t macLen;
   HmacContext hmacContext;

   //Get the hash algorithm to be used for HMAC computation
   hashAlgo = snmpGetHashAlgo(user->authProtocol);

   //Invalid authentication protocol?
   if(hashAlgo == NULL)
      return ERROR_FAILURE;

   //Retrieve the length of the truncated MAC
   macLen = snmpGetMacLength(user->authProtocol);

   //Check the length of the msgAuthenticationParameters field
   if(message->msgAuthParametersLen != macLen)
      return ERROR_FAILURE;

   //The MAC is calculated over the whole message
   hmacInit(&hmacContext, hashAlgo, user->localizedAuthKey.b, hashAlgo->digestSize);
   hmacUpdate(&hmacContext, message->pos, message->length);
   hmacFinal(&hmacContext, NULL);

   //Replace the msgAuthenticationParameters field with the calculated MAC
   osMemcpy(message->msgAuthParameters, hmacContext.digest, macLen);

   //Successful message authentication
   return NO_ERROR;
}


/**
 * @brief Authenticate incoming SNMP message
 * @param[in] user Security profile of the user
 * @param[in] message Pointer to the incoming SNMP message
 * @return Error code
 **/

error_t snmpAuthIncomingMessage(const SnmpUserEntry *user, SnmpMessage *message)
{
   const HashAlgo *hashAlgo;
   size_t macLen;
   uint8_t mac[SNMP_MAX_TRUNCATED_MAC_SIZE];
   HmacContext hmacContext;

   //Get the hash algorithm to be used for HMAC computation
   hashAlgo = snmpGetHashAlgo(user->authProtocol);

   //Invalid authentication protocol?
   if(hashAlgo == NULL)
      return ERROR_AUTHENTICATION_FAILED;

   //Retrieve the length of the truncated MAC
   macLen = snmpGetMacLength(user->authProtocol);

   //Check the length of the msgAuthenticationParameters field
   if(message->msgAuthParametersLen != macLen)
      return ERROR_AUTHENTICATION_FAILED;

   //The MAC received in the msgAuthenticationParameters field is saved
   osMemcpy(mac, message->msgAuthParameters, macLen);

   //The digest in the msgAuthenticationParameters field is replaced by
   //a null octet string
   osMemset(message->msgAuthParameters, 0, macLen);

   //The MAC is calculated over the whole message
   hmacInit(&hmacContext, hashAlgo, user->localizedAuthKey.b, hashAlgo->digestSize);
   hmacUpdate(&hmacContext, message->buffer, message->bufferLen);
   hmacFinal(&hmacContext, NULL);

   //Restore the value of the msgAuthenticationParameters field
   osMemcpy(message->msgAuthParameters, mac, macLen);

   //The newly calculated MAC is compared with the MAC value that was
   //saved in the first step
   if(osMemcmp(hmacContext.digest, mac, macLen))
      return ERROR_AUTHENTICATION_FAILED;

   //Successful message authentication
   return NO_ERROR;
}


/**
 * @brief Data encryption
 * @param[in] user Security profile of the user
 * @param[in,out] message Pointer to the outgoing SNMP message
 * @param[in,out] salt Pointer to the salt integer
 * @return Error code
 **/

error_t snmpEncryptData(const SnmpUserEntry *user, SnmpMessage *message,
   uint64_t *salt)
{
   error_t error;
   uint_t i;
   size_t n;
   Asn1Tag tag;

   //Debug message
   TRACE_DEBUG("Scoped PDU (%" PRIuSIZE " bytes):\r\n", message->length);
   //Display the contents of the scopedPDU
   TRACE_DEBUG_ARRAY("  ", message->pos, message->length);
   //Display ASN.1 structure
   asn1DumpObject(message->pos, message->length, 0);

#if (SNMP_DES_SUPPORT == ENABLED)
   //DES-CBC privacy protocol?
   if(user->privProtocol == SNMP_PRIV_PROTOCOL_DES)
   {
      DesContext desContext;
      uint8_t iv[DES_BLOCK_SIZE];

      //The data to be encrypted is treated as sequence of octets. Its length
      //should be an integral multiple of 8
      if((message->length % 8) != 0)
      {
         //If it is not, the data is padded at the end as necessary
         n = 8 - (message->length % 8);
         //The actual pad value is irrelevant
         osMemset(message->pos + message->length, n, n);
         //Update the length of the data
         message->length += n;
      }

      //The 32-bit snmpEngineBoots is converted to the first 4 octets of our salt
      STORE32BE(message->msgAuthEngineBoots, message->msgPrivParameters);
      //The 32-bit integer is then converted to the last 4 octet of our salt
      STORE32BE(*salt, message->msgPrivParameters + 4);

      //The resulting salt is then put into the msgPrivacyParameters field
      message->msgPrivParametersLen = 8;

      //Initialize DES context
      error = desInit(&desContext, user->localizedPrivKey.b, 8);
      //Initialization failed?
      if(error)
         return error;

      //The last 8 octets of the 16-octet secret (private privacy key) are
      //used as pre-IV
      osMemcpy(iv, user->localizedPrivKey.b + DES_BLOCK_SIZE, DES_BLOCK_SIZE);

      //The msgPrivacyParameters field is XOR-ed with the pre-IV to obtain the IV
      for(i = 0; i < DES_BLOCK_SIZE; i++)
      {
         iv[i] ^= message->msgPrivParameters[i];
      }

      //Perform CBC encryption
      error = cbcEncrypt(DES_CIPHER_ALGO, &desContext, iv, message->pos,
         message->pos, message->length);
      //Any error to report?
      if(error)
         return error;
   }
   else
#endif
#if (SNMP_AES_SUPPORT == ENABLED)
   //AES-128-CFB privacy protocol?
   if(user->privProtocol == SNMP_PRIV_PROTOCOL_AES)
   {
      AesContext aesContext;
      uint8_t iv[AES_BLOCK_SIZE];

      //The 32-bit snmpEngineBoots is converted to the first 4 octets of the IV
      STORE32BE(message->msgAuthEngineBoots, iv);
      //The 32-bit snmpEngineTime is converted to the subsequent 4 octets
      STORE32BE(message->msgAuthEngineTime, iv + 4);
      //The 64-bit integer is then converted to the last 8 octets
      STORE64BE(*salt, iv + 8);

      //The 64-bit integer must be placed in the msgPrivacyParameters field to
      //enable the receiving entity to compute the correct IV and to decrypt
      //the message
      STORE64BE(*salt, message->msgPrivParameters);
      message->msgPrivParametersLen = 8;

      //Initialize AES context
      error = aesInit(&aesContext, user->localizedPrivKey.b, 16);
      //Initialization failed?
      if(error)
         return error;

      //Perform CFB-128 encryption
      error = cfbEncrypt(AES_CIPHER_ALGO, &aesContext, 128, iv, message->pos,
         message->pos, message->length);
      //Any error to report?
      if(error)
         return error;
   }
   else
#endif
   //Invalid privacy protocol?
   {
      //Report an error
      return ERROR_FAILURE;
   }

   //The encryptedPDU is encapsulated within an octet string
   tag.constructed = FALSE;
   tag.objClass = ASN1_CLASS_UNIVERSAL;
   tag.objType = ASN1_TYPE_OCTET_STRING;
   tag.length = message->length;
   tag.value = NULL;

   //Write the corresponding ASN.1 tag
   error = asn1WriteTag(&tag, TRUE, message->pos, &n);
   //Any error to report?
   if(error)
      return error;

   //Move backward
   message->pos -= n;
   //Total length of the encryptedPDU
   message->length += n;

   //The salt integer is then modified. It is incremented by one and wrap
   //when it reaches its maximum value
   *salt += 1;

   //Successful encryption
   return NO_ERROR;
}


/**
 * @brief Data decryption
 * @param[in] user Security profile of the user
 * @param[in,out] message Pointer to the incoming SNMP message
 * @return Error code
 **/

error_t snmpDecryptData(const SnmpUserEntry *user, SnmpMessage *message)
{
   error_t error;
   uint_t i;
   Asn1Tag tag;

   //The encryptedPDU is encapsulated within an octet string
   error = asn1ReadTag(message->pos, message->length, &tag);
   //Failed to decode ASN.1 tag?
   if(error)
      return error;

   //Enforce encoding, class and type
   error = asn1CheckTag(&tag, FALSE, ASN1_CLASS_UNIVERSAL, ASN1_TYPE_OCTET_STRING);
   //The tag does not match the criteria?
   if(error)
      return error;

   //Point to the encryptedPDU
   message->pos = (uint8_t *) tag.value;
   //Length of the encryptedPDU
   message->length = tag.length;

#if (SNMP_DES_SUPPORT == ENABLED)
   //DES-CBC privacy protocol?
   if(user->privProtocol == SNMP_PRIV_PROTOCOL_DES)
   {
      DesContext desContext;
      uint8_t iv[DES_BLOCK_SIZE];

      //Before decryption, the encrypted data length is verified. The length
      //of the encrypted data must be a multiple of 8 octets
      if((message->length % 8) != 0)
         return ERROR_DECRYPTION_FAILED;

      //Check the length of the msgPrivacyParameters field
      if(message->msgPrivParametersLen != 8)
         return ERROR_DECRYPTION_FAILED;

      //Initialize DES context
      error = desInit(&desContext, user->localizedPrivKey.b, 8);
      //Initialization failed?
      if(error)
         return error;

      //The last 8 octets of the 16-octet secret (private privacy key) are
      //used as pre-IV
      osMemcpy(iv, user->localizedPrivKey.b + DES_BLOCK_SIZE, DES_BLOCK_SIZE);

      //The msgPrivacyParameters field is XOR-ed with the pre-IV to obtain the IV
      for(i = 0; i < DES_BLOCK_SIZE; i++)
      {
         iv[i] ^= message->msgPrivParameters[i];
      }

      //Perform CBC decryption
      error = cbcDecrypt(DES_CIPHER_ALGO, &desContext, iv, message->pos,
         message->pos, message->length);
      //Any error to report?
      if(error)
         return error;
   }
   else
#endif
#if (SNMP_AES_SUPPORT == ENABLED)
   //AES-128-CFB privacy protocol?
   if(user->privProtocol == SNMP_PRIV_PROTOCOL_AES)
   {
      AesContext aesContext;
      uint8_t iv[AES_BLOCK_SIZE];

      //Check the length of the msgPrivacyParameters field
      if(message->msgPrivParametersLen != 8)
         return ERROR_DECRYPTION_FAILED;

      //The 32-bit snmpEngineBoots is converted to the first 4 octets of the IV
      STORE32BE(message->msgAuthEngineBoots, iv);
      //The 32-bit snmpEngineTime is converted to the subsequent 4 octets
      STORE32BE(message->msgAuthEngineTime, iv + 4);
      //The 64-bit integer is then converted to the last 8 octets
      osMemcpy(iv + 8, message->msgPrivParameters, 8);

      //Initialize AES context
      error = aesInit(&aesContext, user->localizedPrivKey.b, 16);
      //Initialization failed?
      if(error)
         return error;

      //Perform CFB-128 encryption
      error = cfbDecrypt(AES_CIPHER_ALGO, &aesContext, 128, iv, message->pos,
         message->pos, message->length);
      //Any error to report?
      if(error)
         return error;
   }
   else
#endif
   //Invalid privacy protocol?
   {
      //Report an error
      return ERROR_DECRYPTION_FAILED;
   }

   //Debug message
   TRACE_DEBUG("Scoped PDU (%" PRIuSIZE " bytes):\r\n", message->length);
   //Display the contents of the scopedPDU
   TRACE_DEBUG_ARRAY("  ", message->pos, message->length);
   //Display ASN.1 structure
   asn1DumpObject(message->pos, message->length, 0);

   //Successful decryption
   return NO_ERROR;
}


/**
 * @brief Get the hash algorithm to be used for a given authentication protocol
 * @param[in] authProtocol Authentication protocol (MD5, SHA-1, SHA-224,
 *   SHA-256, SHA384 or SHA512)
 * @return Pointer to the corresponding hash algorithm
 **/

const HashAlgo *snmpGetHashAlgo(SnmpAuthProtocol authProtocol)
{
   const HashAlgo *hashAlgo;

#if (SNMP_MD5_SUPPORT == ENABLED)
   //HMAC-MD5-96 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_MD5)
   {
      //Use MD5 hash algorithm
      hashAlgo = MD5_HASH_ALGO;
   }
   else
#endif
#if (SNMP_SHA1_SUPPORT == ENABLED)
   //HMAC-SHA-1-96 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_SHA1)
   {
      //Use SHA-1 hash algorithm
      hashAlgo = SHA1_HASH_ALGO;
   }
   else
#endif
#if (SNMP_SHA224_SUPPORT == ENABLED)
   //HMAC-SHA-224-128 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_SHA224)
   {
      //Use SHA-224 hash algorithm
      hashAlgo = SHA224_HASH_ALGO;
   }
   else
#endif
#if (SNMP_SHA256_SUPPORT == ENABLED)
   //HMAC-SHA-256-192 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_SHA256)
   {
      //Use SHA-256 hash algorithm
      hashAlgo = SHA256_HASH_ALGO;
   }
   else
#endif
#if (SNMP_SHA384_SUPPORT == ENABLED)
   //HMAC-SHA-384-256 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_SHA384)
   {
      //Use SHA-384 hash algorithm
      hashAlgo = SHA384_HASH_ALGO;
   }
   else
#endif
#if (SNMP_SHA512_SUPPORT == ENABLED)
   //HMAC-SHA-512-384 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_SHA512)
   {
      //Use SHA-512 hash algorithm
      hashAlgo = SHA512_HASH_ALGO;
   }
   else
#endif
   //Invalid authentication protocol?
   {
      //The authentication protocol is not supported
      hashAlgo = NULL;
   }

   //Return the hash algorithm to be used
   return hashAlgo;
}


/**
 * @brief Get the length of the truncated MAC for a given authentication protocol
 * @param[in] authProtocol Authentication protocol (MD5, SHA-1, SHA-224,
 *   SHA-256, SHA384 or SHA512)
 * @return Length of the truncated MAC, in bytes
 **/

size_t snmpGetMacLength(SnmpAuthProtocol authProtocol)
{
   size_t macLen;

#if (SNMP_MD5_SUPPORT == ENABLED)
   //HMAC-MD5-96 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_MD5)
   {
      //The length of the truncated MAC is 96 bits
      macLen = 12;
   }
   else
#endif
#if (SNMP_SHA1_SUPPORT == ENABLED)
   //HMAC-SHA-1-96 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_SHA1)
   {
      //The length of the truncated MAC is 96 bits
      macLen = 12;
   }
   else
#endif
#if (SNMP_SHA224_SUPPORT == ENABLED)
   //HMAC-SHA-224-128 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_SHA224)
   {
      //The length of the truncated MAC is 128 bits
      macLen = 16;
   }
   else
#endif
#if (SNMP_SHA256_SUPPORT == ENABLED)
   //HMAC-SHA-256-192 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_SHA256)
   {
      //The length of the truncated MAC is 192 bits
      macLen = 24;
   }
   else
#endif
#if (SNMP_SHA384_SUPPORT == ENABLED)
   //HMAC-SHA-384-256 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_SHA384)
   {
      //The length of the truncated MAC is 256 bits
      macLen = 32;
   }
   else
#endif
#if (SNMP_SHA512_SUPPORT == ENABLED)
   //HMAC-SHA-512-384 authentication protocol?
   if(authProtocol == SNMP_AUTH_PROTOCOL_SHA512)
   {
      //The length of the truncated MAC is 384 bits
      macLen = 48;
   }
   else
#endif
   //Invalid authentication protocol?
   {
      //The authentication protocol is not supported
      macLen = 0;
   }

   //Return the length of the truncated MAC
   return macLen;
}

#endif
