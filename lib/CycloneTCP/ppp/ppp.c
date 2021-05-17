/**
 * @file ppp.c
 * @brief PPP (Point-to-Point Protocol)
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
#define TRACE_LEVEL PPP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "ppp/ppp.h"
#include "ppp/ppp_hdlc.h"
#include "ppp/ppp_debug.h"
#include "ppp/lcp.h"
#include "ppp/ipcp.h"
#include "ppp/ipv6cp.h"
#include "ppp/pap.h"
#include "ppp/chap.h"
#include "mibs/mib2_module.h"
#include "mibs/if_mib_module.h"
#include "str.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (PPP_SUPPORT == ENABLED)

//Tick counter to handle periodic operations
systime_t pppTickCounter;

//FCS lookup table
static const uint16_t fcsTable[256] =
{
   0x0000, 0x1189, 0x2312, 0x329B, 0x4624, 0x57AD, 0x6536, 0x74BF,
   0x8C48, 0x9DC1, 0xAF5A, 0xBED3, 0xCA6C, 0xDBE5, 0xE97E, 0xF8F7,
   0x1081, 0x0108, 0x3393, 0x221A, 0x56A5, 0x472C, 0x75B7, 0x643E,
   0x9CC9, 0x8D40, 0xBFDB, 0xAE52, 0xDAED, 0xCB64, 0xF9FF, 0xE876,
   0x2102, 0x308B, 0x0210, 0x1399, 0x6726, 0x76AF, 0x4434, 0x55BD,
   0xAD4A, 0xBCC3, 0x8E58, 0x9FD1, 0xEB6E, 0xFAE7, 0xC87C, 0xD9F5,
   0x3183, 0x200A, 0x1291, 0x0318, 0x77A7, 0x662E, 0x54B5, 0x453C,
   0xBDCB, 0xAC42, 0x9ED9, 0x8F50, 0xFBEF, 0xEA66, 0xD8FD, 0xC974,
   0x4204, 0x538D, 0x6116, 0x709F, 0x0420, 0x15A9, 0x2732, 0x36BB,
   0xCE4C, 0xDFC5, 0xED5E, 0xFCD7, 0x8868, 0x99E1, 0xAB7A, 0xBAF3,
   0x5285, 0x430C, 0x7197, 0x601E, 0x14A1, 0x0528, 0x37B3, 0x263A,
   0xDECD, 0xCF44, 0xFDDF, 0xEC56, 0x98E9, 0x8960, 0xBBFB, 0xAA72,
   0x6306, 0x728F, 0x4014, 0x519D, 0x2522, 0x34AB, 0x0630, 0x17B9,
   0xEF4E, 0xFEC7, 0xCC5C, 0xDDD5, 0xA96A, 0xB8E3, 0x8A78, 0x9BF1,
   0x7387, 0x620E, 0x5095, 0x411C, 0x35A3, 0x242A, 0x16B1, 0x0738,
   0xFFCF, 0xEE46, 0xDCDD, 0xCD54, 0xB9EB, 0xA862, 0x9AF9, 0x8B70,
   0x8408, 0x9581, 0xA71A, 0xB693, 0xC22C, 0xD3A5, 0xE13E, 0xF0B7,
   0x0840, 0x19C9, 0x2B52, 0x3ADB, 0x4E64, 0x5FED, 0x6D76, 0x7CFF,
   0x9489, 0x8500, 0xB79B, 0xA612, 0xD2AD, 0xC324, 0xF1BF, 0xE036,
   0x18C1, 0x0948, 0x3BD3, 0x2A5A, 0x5EE5, 0x4F6C, 0x7DF7, 0x6C7E,
   0xA50A, 0xB483, 0x8618, 0x9791, 0xE32E, 0xF2A7, 0xC03C, 0xD1B5,
   0x2942, 0x38CB, 0x0A50, 0x1BD9, 0x6F66, 0x7EEF, 0x4C74, 0x5DFD,
   0xB58B, 0xA402, 0x9699, 0x8710, 0xF3AF, 0xE226, 0xD0BD, 0xC134,
   0x39C3, 0x284A, 0x1AD1, 0x0B58, 0x7FE7, 0x6E6E, 0x5CF5, 0x4D7C,
   0xC60C, 0xD785, 0xE51E, 0xF497, 0x8028, 0x91A1, 0xA33A, 0xB2B3,
   0x4A44, 0x5BCD, 0x6956, 0x78DF, 0x0C60, 0x1DE9, 0x2F72, 0x3EFB,
   0xD68D, 0xC704, 0xF59F, 0xE416, 0x90A9, 0x8120, 0xB3BB, 0xA232,
   0x5AC5, 0x4B4C, 0x79D7, 0x685E, 0x1CE1, 0x0D68, 0x3FF3, 0x2E7A,
   0xE70E, 0xF687, 0xC41C, 0xD595, 0xA12A, 0xB0A3, 0x8238, 0x93B1,
   0x6B46, 0x7ACF, 0x4854, 0x59DD, 0x2D62, 0x3CEB, 0x0E70, 0x1FF9,
   0xF78F, 0xE606, 0xD49D, 0xC514, 0xB1AB, 0xA022, 0x92B9, 0x8330,
   0x7BC7, 0x6A4E, 0x58D5, 0x495C, 0x3DE3, 0x2C6A, 0x1EF1, 0x0F78
};


/**
 * @brief Initialize settings with default values
 * @param[out] settings Structure that contains PPP settings
 **/

void pppGetDefaultSettings(PppSettings *settings)
{
   //Use default interface
   settings->interface = netGetDefaultInterface();

   //Default MRU
   settings->mru = PPP_DEFAULT_MRU;
   //Default async control character map
   settings->accm = PPP_DEFAULT_ACCM;
   //Allowed authentication protocols
   settings->authProtocol = PPP_AUTH_PROTOCOL_PAP | PPP_AUTH_PROTOCOL_CHAP_MD5;

   //Random data generation callback function
   settings->randCallback = NULL;
   //PPP authentication callback function
   settings->authCallback = NULL;
}


/**
 * @brief PPP initialization
 * @param[in] context Pointer to the PPP context
 * @param[in] settings PPP specific settings
 * @return Error code
 **/

error_t pppInit(PppContext *context, const PppSettings *settings)
{
   error_t error;
   NetInterface *interface;

   //Debug message
   TRACE_INFO("PPP initialization\r\n");

   //Underlying network interface
   interface = settings->interface;

   //Initialize PPP context
   osMemset(context, 0, sizeof(PppContext));

   //Save user settings
   context->settings = *settings;

#if (PAP_SUPPORT == DISABLED)
   //PAP authentication is not supported
   context->settings.authProtocol &= ~PPP_AUTH_PROTOCOL_PAP;
#endif

#if (PAP_SUPPORT == DISABLED)
   //CHAP with MD5 authentication is not supported
   context->settings.authProtocol &= ~PPP_AUTH_PROTOCOL_CHAP_MD5;
#endif

   //Attach the PPP context to the network interface
   interface->pppContext = context;

   //Initialize structure
   context->interface = interface;
   context->timeout = INFINITE_DELAY;

   //Initialize PPP finite state machine
   context->pppPhase = PPP_PHASE_DEAD;
   context->lcpFsm.state = PPP_STATE_0_INITIAL;

#if (IPV4_SUPPORT == ENABLED)
   //Initialize IPCP finite state machine
   context->ipcpFsm.state = PPP_STATE_0_INITIAL;
#endif

#if (IPV6_SUPPORT == ENABLED)
   //Initialize IPV6CP finite state machine
   context->ipv6cpFsm.state = PPP_STATE_0_INITIAL;
#endif

#if (PAP_SUPPORT == ENABLED)
   //Initialize PAP finite state machine
   context->papFsm.localState = PAP_STATE_0_INITIAL;
   context->papFsm.peerState = PAP_STATE_0_INITIAL;
#endif

#if (CHAP_SUPPORT == ENABLED)
   //Initialize CHAP finite state machine
   context->chapFsm.localState = CHAP_STATE_0_INITIAL;
   context->chapFsm.peerState = CHAP_STATE_0_INITIAL;
#endif

   //Attach PPP HDLC driver
   error = netSetDriver(interface, &pppHdlcDriver);

   //Return status code
   return error;
}


/**
 * @brief Set timeout value for blocking operations
 * @param[in] interface Underlying network interface
 * @param[in] timeout Maximum time to wait
 * @return Error code
 **/

error_t pppSetTimeout(NetInterface *interface, systime_t timeout)
{
   PppContext *context;

   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;
   //Make sure PPP has been properly configured
   if(interface->pppContext == NULL)
      return ERROR_NOT_CONFIGURED;

   //Point to the PPP context
   context = interface->pppContext;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Set timeout value
   context->timeout = timeout;

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Set PPP authentication information
 * @param[in] interface Underlying network interface
 * @param[in] username NULL-terminated string containing the user name to be used
 * @param[in] password NULL-terminated string containing the password to be used
 * @return Error code
 **/

error_t pppSetAuthInfo(NetInterface *interface,
   const char_t *username, const char_t *password)
{
   PppContext *context;

   //Check parameters
   if(interface == NULL || username == NULL || password == NULL)
      return ERROR_INVALID_PARAMETER;
   //Make sure PPP has been properly configured
   if(interface->pppContext == NULL)
      return ERROR_NOT_CONFIGURED;

   //Point to the PPP context
   context = interface->pppContext;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Save user name
   strSafeCopy(context->username, username, PPP_MAX_USERNAME_LEN);
   //Save password
   strSafeCopy(context->password, password, PPP_MAX_PASSWORD_LEN);

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Password verification
 * @param[in] interface Underlying network interface
 * @param[in] password NULL-terminated string containing the password to be checked
 * @return TRUE if the password is valid, else FALSE
 **/

bool_t pppCheckPassword(NetInterface *interface, const char_t *password)
{
   bool_t status;
   PppContext *context;

   //Debug message
   TRACE_DEBUG("PPP password verification...\r\n");

   //The password has not been verified yet
   status = FALSE;

   //Point to the PPP context
   context = interface->pppContext;

   //Make sure PPP has been properly configured
   if(context != NULL)
   {
      //Check authentication protocol
      if(context->localConfig.authProtocol == PPP_PROTOCOL_PAP)
      {
#if (PAP_SUPPORT == ENABLED)
         //PAP authentication protocol
         status = papCheckPassword(context, password);
#endif
      }
      //CHAP authentication protocol?
      else if(context->localConfig.authProtocol == PPP_PROTOCOL_CHAP)
      {
#if (CHAP_SUPPORT == ENABLED)
         //CHAP authentication protocol
         status = chapCheckPassword(context, password);
#endif
      }
   }

   //Return TRUE is the password is valid, else FALSE
   return status;
}


/**
 * @brief Send AT command
 * @param[in] interface Underlying network interface
 * @param[in] data NULL-terminated string that contains the AT command to be sent
 * @return Error code
 **/

error_t pppSendAtCommand(NetInterface *interface, const char_t *data)
{
   error_t error;
   bool_t status;
   PppContext *context;

   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;
   //Make sure PPP has been properly configured
   if(interface->pppContext == NULL)
      return ERROR_NOT_CONFIGURED;

   //Point to the PPP context
   context = interface->pppContext;

   //Wait for the send buffer to be available for writing
   status = osWaitForEvent(&interface->nicTxEvent, context->timeout);

   //Check status
   if(status)
   {
      //Get exclusive access
      osAcquireMutex(&netMutex);

      //Check current PPP state
      if(context->pppPhase == PPP_PHASE_DEAD)
      {
         //Purge receive buffer
         error = pppHdlcDriverPurgeRxBuffer(context);

         //Send AT command
         if(!error)
            error = pppHdlcDriverSendAtCommand(interface, data);
      }
      else
      {
         //Report an error
         error = ERROR_ALREADY_CONNECTED;
      }

      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
   else
   {
      //Timeout error
      return ERROR_TIMEOUT;
   }

   //Return status code
   return error;
}


/**
 * @brief Wait for an incoming AT command
 * @param[in] interface Underlying network interface
 * @param[out] data Buffer where to store the incoming AT command
 * @param[in] size Size of the buffer, in bytes
 * @return Error code
 **/

error_t pppReceiveAtCommand(NetInterface *interface, char_t *data, size_t size)
{
   error_t error;
   systime_t time;
   systime_t start;
   PppContext *context;

   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;
   //Make sure PPP has been properly configured
   if(interface->pppContext == NULL)
      return ERROR_NOT_CONFIGURED;

   //Point to the PPP context
   context = interface->pppContext;
   //Save current time
   start = osGetSystemTime();

   //Wait for an incoming AT command
   while(1)
   {
      //Get exclusive access
      osAcquireMutex(&netMutex);

      //Check current PPP state
      if(context->pppPhase == PPP_PHASE_DEAD)
      {
         //Wait for an incoming AT command
         error = pppHdlcDriverReceiveAtCommand(interface, data, size);
      }
      else
      {
         //Report an error
         error = ERROR_ALREADY_CONNECTED;
      }

      //Release exclusive access
      osReleaseMutex(&netMutex);

      //No data received?
      if(error == ERROR_BUFFER_EMPTY || data[0] == '\0')
      {
         //Get current time
         time = osGetSystemTime();

         //Check whether the timeout period has elapsed
         if(timeCompare(time, start + context->timeout) >= 0)
         {
            //Timeout error
            error = ERROR_TIMEOUT;
            //Exit immediately
            break;
         }
         else
         {
            //Wait for more data to be received
            osDelayTask(PPP_POLLING_INTERVAL);
         }
      }
      else
      {
         //We are done
         break;
      }
   }

   //Return status code
   return error;
}


/**
 * @brief Establish a PPP connection
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t pppConnect(NetInterface *interface)
{
   error_t error;
   PppContext *context;
#if (NET_RTOS_SUPPORT == ENABLED)
   systime_t time;
   systime_t start;
#endif

   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;
   //Make sure PPP has been properly configured
   if(interface->pppContext == NULL)
      return ERROR_NOT_CONFIGURED;

   //Point to the PPP context
   context = interface->pppContext;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //Default PPP phase
   context->pppPhase = PPP_PHASE_DEAD;

   //Initialize LCP FSM
   context->lcpFsm.state = PPP_STATE_0_INITIAL;
   context->lcpFsm.identifier = 0;
   context->lcpFsm.restartCounter = 0;
   context->lcpFsm.failureCounter = 0;

#if (IPV4_SUPPORT == ENABLED)
   //Initialize IPCP FSM
   context->ipcpFsm.state = PPP_STATE_0_INITIAL;
   context->ipcpFsm.identifier = 0;
   context->ipcpFsm.restartCounter = 0;
   context->ipcpFsm.failureCounter = 0;
#endif

#if (IPV6_SUPPORT == ENABLED)
   //Initialize IPV6CP FSM
   context->ipv6cpFsm.state = PPP_STATE_0_INITIAL;
   context->ipv6cpFsm.identifier = 0;
   context->ipv6cpFsm.restartCounter = 0;
   context->ipv6cpFsm.failureCounter = 0;
#endif

   //Authentication has not been completed
   context->localAuthDone = FALSE;
   context->peerAuthDone = FALSE;

#if (PAP_SUPPORT == ENABLED)
   //Initialize PAP FSM
   context->papFsm.localState = PAP_STATE_0_INITIAL;
   context->papFsm.peerState = PAP_STATE_0_INITIAL;
   context->papFsm.identifier = 0;
   context->papFsm.restartCounter = 0;
#endif

#if (CHAP_SUPPORT == ENABLED)
   //Initialize CHAP FSM
   context->chapFsm.localState = CHAP_STATE_0_INITIAL;
   context->chapFsm.localIdentifier = 0;
   context->chapFsm.peerState = CHAP_STATE_0_INITIAL;
   context->chapFsm.peerIdentifier = 0;
#endif

   //Default local configuration
   context->localConfig.mru = context->settings.mru;
   context->localConfig.mruRejected = FALSE;
   context->localConfig.accm = context->settings.accm;
   context->localConfig.accmRejected = FALSE;
   context->localConfig.authProtocol = 0;
   context->localConfig.authAlgo = 0;
   context->localConfig.authProtocolRejected = FALSE;
   context->localConfig.magicNumber = PPP_DEFAULT_MAGIC_NUMBER;
   context->localConfig.magicNumberRejected = FALSE;
   context->localConfig.pfc = TRUE;
   context->localConfig.pfcRejected = FALSE;
   context->localConfig.acfc = TRUE;
   context->localConfig.acfcRejected = FALSE;

   //Check whether the other end of the PPP link must be authenticated
   if(context->settings.authCallback != NULL)
   {
#if (PAP_SUPPORT == ENABLED)
      //PAP provides an easy implementation of peer authentication
      if(context->settings.authProtocol & PPP_AUTH_PROTOCOL_PAP)
      {
         //Select PAP authentication protocol
         context->localConfig.authProtocol = PPP_PROTOCOL_PAP;
      }
#endif
#if (CHAP_SUPPORT == ENABLED)
      //CHAP with MD5 ensures greater security in the implementation
      if(context->settings.authProtocol & PPP_AUTH_PROTOCOL_CHAP_MD5)
      {
         //Select CHAP with MD5 authentication protocol
         context->localConfig.authProtocol = PPP_PROTOCOL_CHAP;
         context->localConfig.authAlgo = CHAP_ALGO_ID_CHAP_MD5;
      }
#endif
   }

   //Default peer's configuration
   context->peerConfig.mru = PPP_DEFAULT_MRU;
   context->peerConfig.accm = PPP_DEFAULT_ACCM;
   context->peerConfig.authProtocol = 0;
   context->peerConfig.magicNumber = PPP_DEFAULT_MAGIC_NUMBER;
   context->peerConfig.pfc = FALSE;
   context->peerConfig.acfc = FALSE;

#if (IPV4_SUPPORT == ENABLED)
   //Default local configuration
   context->localConfig.ipAddr = interface->ipv4Context.addrList[0].addr;
   context->localConfig.ipAddrRejected = FALSE;
   context->localConfig.primaryDns = interface->ipv4Context.dnsServerList[0];
   context->localConfig.primaryDnsRejected = FALSE;

#if (IPV4_DNS_SERVER_LIST_SIZE >= 2)
   context->localConfig.secondaryDns = interface->ipv4Context.dnsServerList[1];
   context->localConfig.secondaryDnsRejected = FALSE;
#else
   context->localConfig.secondaryDns = IPV4_UNSPECIFIED_ADDR;
   context->localConfig.secondaryDnsRejected = FALSE;
#endif

   //Manual primary DNS configuration?
   if(context->localConfig.primaryDns != IPV4_UNSPECIFIED_ADDR)
      context->localConfig.primaryDnsRejected = TRUE;

   //Manual secondary DNS configuration?
   if(context->localConfig.secondaryDns != IPV4_UNSPECIFIED_ADDR)
      context->localConfig.secondaryDnsRejected = TRUE;

   //Default peer's configuration
   context->peerConfig.ipAddr = interface->ipv4Context.addrList[0].defaultGateway;
#endif

#if (IPV6_SUPPORT == ENABLED)
   //Default local configuration
   eui64CopyAddr(&context->localConfig.interfaceId,
      interface->ipv6Context.addrList[0].addr.b + 8);

   context->localConfig.interfaceIdRejected = FALSE;

   //Default peer's configuration
   eui64CopyAddr(&context->peerConfig.interfaceId,
      interface->ipv6Context.routerList[0].addr.b + 8);
#endif

   //The link is available for traffic
   error = lcpOpen(context);

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Any error to report?
   if(error)
      return error;

#if (NET_RTOS_SUPPORT == ENABLED)
   //Save current time
   start = osGetSystemTime();

   //Wait for the connection to be established
   while(1)
   {
      //Check current PPP phase
      if(context->pppPhase == PPP_PHASE_NETWORK)
      {
#if (IPV4_SUPPORT == ENABLED)
         //Check current IPCP state
         if(context->ipcpFsm.state == PPP_STATE_9_OPENED)
         {
            //Connection successfully established
            error = NO_ERROR;
            //Exit immediately
            break;
         }
#endif
#if (IPV6_SUPPORT == ENABLED)
         //Check current IPV6CP state
         if(context->ipv6cpFsm.state == PPP_STATE_9_OPENED)
         {
            //Connection successfully established
            error = NO_ERROR;
            //Exit immediately
            break;
         }
#endif
      }
      else if(context->pppPhase == PPP_PHASE_DEAD)
      {
         //Failed to establish connection
         error = ERROR_CONNECTION_FAILED;
         //Exit immediately
         break;
      }

      //Check timeout value
      if(context->timeout != INFINITE_DELAY)
      {
         //Get current time
         time = osGetSystemTime();

         //Check whether the timeout period has elapsed
         if(timeCompare(time, start + context->timeout) >= 0)
         {
            //Report an error
            error = ERROR_TIMEOUT;
            //Exit immediately
            break;
         }
      }

      //Polling delay
      osDelayTask(PPP_POLLING_INTERVAL);
   }

   //Failed to establish connection?
   if(error)
   {
      //Get exclusive access
      osAcquireMutex(&netMutex);

      //Abort the PPP connection
      context->pppPhase = PPP_PHASE_DEAD;
      context->lcpFsm.state = PPP_STATE_0_INITIAL;

#if (IPV4_SUPPORT == ENABLED)
      //Reset IPCP finite state machine
      context->ipcpFsm.state = PPP_STATE_0_INITIAL;
#endif

#if (IPV6_SUPPORT == ENABLED)
      //Reset IPV6CP finite state machine
      context->ipv6cpFsm.state = PPP_STATE_0_INITIAL;
#endif

#if (PAP_SUPPORT == ENABLED)
      //Abort PAP authentication process
      context->papFsm.localState = PAP_STATE_0_INITIAL;
      context->papFsm.peerState = PAP_STATE_0_INITIAL;
#endif

#if (CHAP_SUPPORT == ENABLED)
      //Abort CHAP authentication process
      context->chapFsm.localState = CHAP_STATE_0_INITIAL;
      context->chapFsm.peerState = CHAP_STATE_0_INITIAL;
#endif

      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
#endif

   //Return status code
   return error;
}


/**
 * @brief Close a PPP connection
 * @param[in] interface Underlying network interface
 * @return Error code
 **/

error_t pppClose(NetInterface *interface)
{
   error_t error;
   PppContext *context;
#if (NET_RTOS_SUPPORT == ENABLED)
   systime_t time;
   systime_t start;
#endif

   //Check parameters
   if(interface == NULL)
      return ERROR_INVALID_PARAMETER;
   //Make sure PPP has been properly configured
   if(interface->pppContext == NULL)
      return ERROR_NOT_CONFIGURED;

   //Point to the PPP context
   context = interface->pppContext;

   //Get exclusive access
   osAcquireMutex(&netMutex);

   //The link is no longer available for traffic
   error = lcpClose(context);

   //Release exclusive access
   osReleaseMutex(&netMutex);

   //Any error to report?
   if(error)
      return error;

#if (NET_RTOS_SUPPORT == ENABLED)
   //Save current time
   start = osGetSystemTime();

   //Wait for the connection to be closed
   while(1)
   {
      //Check current PPP phase
      if(context->pppPhase == PPP_PHASE_DEAD)
      {
         //PPP connection is closed
         error = NO_ERROR;
         //Exit immediately
         break;
      }

      //Check timeout value
      if(context->timeout != INFINITE_DELAY)
      {
         //Get current time
         time = osGetSystemTime();

         //Check whether the timeout period has elapsed
         if(timeCompare(time, start + context->timeout) >= 0)
         {
            //Report an error
            error = ERROR_TIMEOUT;
            //Exit immediately
            break;
         }
      }

      //Poll the state
      osDelayTask(PPP_POLLING_INTERVAL);
   }

   //Failed to properly close the connection?
   if(error)
   {
      //Get exclusive access
      osAcquireMutex(&netMutex);

      //Abort the PPP connection
      context->pppPhase = PPP_PHASE_DEAD;
      context->lcpFsm.state = PPP_STATE_0_INITIAL;

#if (IPV4_SUPPORT == ENABLED)
      //Reset IPCP finite state machine
      context->ipcpFsm.state = PPP_STATE_0_INITIAL;
#endif

#if (IPV6_SUPPORT == ENABLED)
      //Reset IPV6CP finite state machine
      context->ipv6cpFsm.state = PPP_STATE_0_INITIAL;
#endif

#if (PAP_SUPPORT == ENABLED)
      //Abort PAP authentication process
      context->papFsm.localState = PAP_STATE_0_INITIAL;
      context->papFsm.peerState = PAP_STATE_0_INITIAL;
#endif

#if (CHAP_SUPPORT == ENABLED)
      //Abort CHAP authentication process
      context->chapFsm.localState = CHAP_STATE_0_INITIAL;
      context->chapFsm.peerState = CHAP_STATE_0_INITIAL;
#endif

      //Release exclusive access
      osReleaseMutex(&netMutex);
   }
#endif

   //Return status code
   return error;
}


/**
 * @brief PPP timer handler
 *
 * This routine must be periodically called by the TCP/IP stack to
 * manage retransmissions
 *
 * @param[in] interface Underlying network interface
 **/

void pppTick(NetInterface *interface)
{
   PppContext *context;

   //PPP driver?
   if(interface->nicDriver->type == NIC_TYPE_PPP)
   {
      //Point to the PPP context
      context = interface->pppContext;

      //Handle LCP retransmission timer
      lcpTick(context);

#if (IPV4_SUPPORT == ENABLED)
      //Handle IPCP retransmission timer
      ipcpTick(context);
#endif

#if (IPV6_SUPPORT == ENABLED)
      //Handle IPV6CP retransmission timer
      ipv6cpTick(context);
#endif

#if (PAP_SUPPORT == ENABLED)
      //Handle PAP timer
      papTick(context);
#endif

#if (CHAP_SUPPORT == ENABLED)
      //Handle CHAP timer
      chapTick(context);
#endif
   }
}


/**
 * @brief Process an incoming PPP frame
 * @param[in] interface Underlying network interface
 * @param[in] frame Incoming PPP frame to process
 * @param[in] length Total frame length
 * @param[in] ancillary Additional options passed to the stack along with
 *   the packet
 **/

void pppProcessFrame(NetInterface *interface, uint8_t *frame, size_t length,
   NetRxAncillary *ancillary)
{
   size_t n;
   uint16_t protocol;
   PppContext *context;
#if (IPV6_SUPPORT == ENABLED)
   NetBuffer1 buffer;
#endif

   //Point to the PPP context
   context = interface->pppContext;

   //Total number of octets received on the interface, including framing
   //characters
   MIB2_INC_COUNTER32(ifGroup.ifTable[interface->index].ifInOctets, length);
   IF_MIB_INC_COUNTER32(ifTable[interface->index].ifInOctets, length);
   IF_MIB_INC_COUNTER64(ifXTable[interface->index].ifHCInOctets, length);

   //Check the length of the PPP frame
   if(length < PPP_FCS_SIZE)
      return;

   //Debug message
   TRACE_DEBUG("PPP frame received (%" PRIuSIZE " bytes)...\r\n", length);

   //The value of the residue is 0x0F47 when no FCS errors are detected
   if(pppCalcFcs(frame, length) != 0x0F47)
   {
      //Debug message
      TRACE_WARNING("Wrong FCS detected!\r\n");
      //Drop the received frame
      return;
   }

   //Calculate the length of PPP frame excluding the FCS field
   length -= PPP_FCS_SIZE;

   //Decompress the frame header
   n = pppParseFrameHeader(frame, length, &protocol);
   //Malformed PPP frame?
   if(!n)
      return;

   //Point to the payload field
   frame += n;
   length -= n;

   //Check protocol field
   switch(protocol)
   {
   //Link control protocol?
   case PPP_PROTOCOL_LCP:
      //Process incoming LCP packet
      lcpProcessPacket(context, (PppPacket *) frame, length);
      break;

#if (IPV4_SUPPORT == ENABLED)
   //IP control protocol?
   case PPP_PROTOCOL_IPCP:
      //Process incoming IPCP packet
      ipcpProcessPacket(context, (PppPacket *) frame, length);
      break;

   //IP protocol?
   case PPP_PROTOCOL_IP:
      //Process incoming IPv4 packet
      ipv4ProcessPacket(interface, (Ipv4Header *) frame, length, ancillary);
      break;
#endif

#if (IPV6_SUPPORT == ENABLED)
   //IPv6 control protocol?
   case PPP_PROTOCOL_IPV6CP:
      //Process incoming IPV6CP packet
      ipv6cpProcessPacket(context, (PppPacket *) frame, length);
      break;

   //IPv6 protocol?
   case PPP_PROTOCOL_IPV6:
      //The incoming PPP frame fits in a single chunk
      buffer.chunkCount = 1;
      buffer.maxChunkCount = 1;
      buffer.chunk[0].address = frame;
      buffer.chunk[0].length = (uint16_t) length;
      buffer.chunk[0].size = 0;

      //Process incoming IPv6 packet
      ipv6ProcessPacket(interface, (NetBuffer *) &buffer, 0, ancillary);
      break;
#endif

#if (PAP_SUPPORT == ENABLED)
   //PAP protocol?
   case PPP_PROTOCOL_PAP:
      //Process incoming PAP packet
      papProcessPacket(context, (PppPacket *) frame, length);
      break;
#endif

#if (CHAP_SUPPORT == ENABLED)
   //CHAP protocol?
   case PPP_PROTOCOL_CHAP:
      //Process incoming CHAP packet
      chapProcessPacket(context, (PppPacket *) frame, length);
      break;
#endif

   //Unknown protocol field
   default:
      //The peer is attempting to use a protocol which is unsupported
      lcpProcessUnknownProtocol(context, protocol, frame, length);
      break;
   }
}


/**
 * @brief Send a PPP frame
 * @param[in] interface Underlying network interface
 * @param[in] buffer Multi-part buffer containing the data
 * @param[in] offset Offset to the first data byte
 * @param[in] protocol Protocol field value
 * @return Error code
 **/

error_t pppSendFrame(NetInterface *interface,
   NetBuffer *buffer, size_t offset, uint16_t protocol)
{
   error_t error;
   size_t length;
   uint16_t fcs;
   uint8_t *p;
   PppContext *context;
   NetTxAncillary ancillary;

   //Point to the PPP context
   context = interface->pppContext;

   //Check whether the Protocol field can be compressed
   if(context->peerConfig.pfc && MSB(protocol) == 0)
   {
      //Is there enough space in the buffer to store the compressed
      //Protocol field?
      if(offset < 1)
         return ERROR_FAILURE;

      //Make room for the Protocol field
      offset--;
      //Move backward
      p = netBufferAt(buffer, offset);
      //Compress the Protocol field
      p[0] = LSB(protocol);
   }
   else
   {
      //Is there enough space in the buffer to store the uncompressed
      //Protocol field?
      if(offset < 2)
         return ERROR_FAILURE;

      //Make room for the Protocol field
      offset -= 2;
      //Move backward
      p = netBufferAt(buffer, offset);
      //Do not compress the Protocol field
      p[0] = MSB(protocol);
      p[1] = LSB(protocol);
   }

   //Check whether the Address and Control fields can be compressed
   if(context->peerConfig.acfc && protocol != PPP_PROTOCOL_LCP)
   {
      //On transmission, compressed Address and Control fields
      //are simply omitted...
   }
   else
   {
      //Is there enough space in the buffer to store the uncompressed
      //Address and Control fields?
      if(offset < 2)
         return ERROR_FAILURE;

      //Make room for the Address and Control fields
      offset -= 2;
      //Move backward
      p = netBufferAt(buffer, offset);
      //Do not compress the Address and Control fields
      p[0] = PPP_ADDR_FIELD;
      p[1] = PPP_CTRL_FIELD;
   }

   //Retrieve the length of the frame
   length = netBufferGetLength(buffer) - offset;

   //Compute FCS over the header and payload
   fcs = pppCalcFcsEx(buffer, offset, length);
   //The FCS is transmitted least significant octet first
   fcs = htole16(fcs);

   //Append the calculated FCS value
   error = netBufferAppend(buffer, &fcs, PPP_FCS_SIZE);
   //Any error to report?
   if(error)
      return error;

   //Adjust frame length
   length += PPP_FCS_SIZE;

   //Total number of octets transmitted out of the interface, including
   //framing characters
   MIB2_INC_COUNTER32(ifGroup.ifTable[interface->index].ifOutOctets, length);
   IF_MIB_INC_COUNTER32(ifTable[interface->index].ifOutOctets, length);
   IF_MIB_INC_COUNTER64(ifXTable[interface->index].ifHCOutOctets, length);

   //Debug message
   TRACE_DEBUG("Sending PPP frame (%" PRIuSIZE " bytes)...\r\n", length);
   TRACE_DEBUG("  Protocol = 0x%04" PRIX16 "\r\n", protocol);

   //Additional options can be passed to the stack along with the packet
   ancillary = NET_DEFAULT_TX_ANCILLARY;

   //Send the resulting frame over the specified link
   error = nicSendPacket(interface, buffer, offset, &ancillary);

   //Return status code
   return error;
}


/**
 * @brief Parse PPP frame header
 * @param[in] frame Pointer to the PPP frame
 * @param[in] length Length of the frame, in bytes
 * @param[out] protocol Value of the Protocol field
 * @return If the PPP header was successfully parsed, the function returns the size
 *   of the PPP header, in bytes. If a parsing error occurred, zero is returned
 **/

size_t pppParseFrameHeader(const uint8_t *frame, size_t length, uint16_t *protocol)
{
   size_t n;

   //Size of the PPP header, in bytes
   n = 0;

   //On reception, the Address and Control fields are decompressed by
   //examining the first two octets
   if(length >= 2)
   {
      //If they contain the values 0xff and 0x03, they are assumed to be
      //the Address and Control fields. If not, it is assumed that the
      //fields were compressed and were not transmitted
      if(frame[0] == PPP_ADDR_FIELD && frame[1] == PPP_CTRL_FIELD)
      {
         //Move to the Protocol field
         n = 2;
      }
   }

   //Check the length of the PPP frame
   if(length >= (n + 1))
   {
      //PPP Protocol field numbers are chosen such that some values may be
      //compressed into a single octet form which is clearly distinguishable
      //from the two octet form
      if(frame[n] & 0x01)
      {
         //The presence of a binary 1 as the LSB marks the last octet of
         //the Protocol field
         *protocol = frame[n];

         //Update the length of the header
         n++;
      }
      else
      {
         //Check the length of the PPP frame
         if(length >= (n + 2))
         {
            //The Protocol field is not compressed
            *protocol = (frame[n] << 8) | frame[n + 1];

            //Update the length of the header
            n += 2;
         }
         else
         {
            //Malformed PPP frame
            n = 0;
         }
      }
   }
   else
   {
      //Malformed PPP frame
      n = 0;
   }

   //Return the size of the PPP header, in bytes
   return n;
}


/**
 * @brief FCS calculation
 * @param[in] data Pointer to the data over which to calculate the FCS
 * @param[in] length Number of bytes to process
 * @return Resulting FCS value
 **/

uint16_t pppCalcFcs(const uint8_t *data, size_t length)
{
   size_t i;
   uint16_t fcs;

   //FCS preset value
   fcs = 0xFFFF;

   //Loop through data
   for(i = 0; i < length; i++)
   {
      //The message is processed byte by byte
      fcs = (fcs >> 8) ^ fcsTable[(fcs & 0xFF) ^ data[i]];
   }

   //Return 1's complement value
   return ~fcs;
}


/**
 * @brief Calculate FCS over a multi-part buffer
 * @param[in] buffer Pointer to the multi-part buffer
 * @param[in] offset Offset from the beginning of the buffer
 * @param[in] length Number of bytes to process
 * @return Resulting FCS value
 **/

uint16_t pppCalcFcsEx(const NetBuffer *buffer, size_t offset, size_t length)
{
   uint_t i;
   uint_t n;
   uint16_t fcs;
   uint8_t *p;

   //FCS preset value
   fcs = 0xFFFF;

   //Loop through data chunks
   for(i = 0; i < buffer->chunkCount && length > 0; i++)
   {
      //Is there any data to process in the current chunk?
      if(offset < buffer->chunk[i].length)
      {
         //Point to the first data byte
         p = (uint8_t *) buffer->chunk[i].address + offset;
         //Compute the number of bytes to process
         n = MIN(buffer->chunk[i].length - offset, length);
         //Adjust byte counter
         length -= n;

         //Process current chunk
         while(n > 0)
         {
            //The message is processed byte by byte
            fcs = (fcs >> 8) ^ fcsTable[(fcs & 0xFF) ^ *p];

            //Next byte
            p++;
            n--;
         }

         //Process the next block from the start
         offset = 0;
      }
      else
      {
         //Skip the current chunk
         offset -= buffer->chunk[i].length;
      }
   }

   //Return 1's complement value
   return ~fcs;
}


/**
 * @brief Allocate a buffer to hold a PPP frame
 * @param[in] length Desired payload length
 * @param[out] offset Offset to the first byte of the payload
 * @return The function returns a pointer to the newly allocated
 *   buffer. If the system is out of resources, NULL is returned
 **/

NetBuffer *pppAllocBuffer(size_t length, size_t *offset)
{
   NetBuffer *buffer;

   //Allocate a buffer to hold the Ethernet header and the payload
   buffer = netBufferAlloc(length + PPP_FRAME_HEADER_SIZE);
   //Failed to allocate buffer?
   if(buffer == NULL)
      return NULL;

   //Offset to the first byte of the payload
   *offset = PPP_FRAME_HEADER_SIZE;

   //Return a pointer to the freshly allocated buffer
   return buffer;
}

#endif
