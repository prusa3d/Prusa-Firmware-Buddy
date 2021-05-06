/**
 * @file dhcpv6_debug.c
 * @brief Data logging functions for debugging purpose (DHCPv6)
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
#define TRACE_LEVEL DHCPV6_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "dhcpv6/dhcpv6_debug.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED && DHCPV6_TRACE_LEVEL >= TRACE_LEVEL_DEBUG)

//DHCPv6 message types
static const char_t *messageLabel[] =
{
   "",
   "SOLICIT",
   "ADVERTISE",
   "REQUEST",
   "CONFIRM",
   "RENEW",
   "REBIND",
   "REPLY",
   "RELEASE",
   "DECLINE",
   "RECONFIGURE",
   "INFO-REQUEST",
   "RELAY-FORW",
   "RELAY-REPL"
};

//DHCPv6 options
static const char_t *optionLabel[] =
{
   "",
   "Client Identifier",
   "Server Identifier",
   "IA_NA",
   "IA_TA",
   "IA Address",
   "Option Request",
   "Preference",
   "Elapsed time",
   "Relay Message",
   "",
   "Authentication",
   "Server Unicast",
   "Status Code",
   "Rapid Commit",
   "User Class",
   "Vendor Class",
   "Vendor Specific Information",
   "Interface ID",
   "Reconfigure Message",
   "Reconfigure Accept",
   "",
   "",
   "DNS Recursive Name Server",
   "Domain Search List"
};

//DHCPv6 status codes
static const char_t *statusLabel[] =
{
   "Success",
   "Unspecified Failure",
   "No Address Available",
   "No Binding",
   "Not On Link",
   "Use Multicast",
};

//Prefix used to format the structure
static const char_t *prefix[8] =
{
   "",
   "  ",
   "    ",
   "      ",
   "        ",
   "          ",
   "            ",
   "              "
};


/**
 * @brief Dump DHCPv6 message for debugging purpose
 * @param[in] message Pointer to the DHCPv6 message to dump
 * @param[in] length Length of the message
 * @return Error code
 **/

error_t dhcpv6DumpMessage(const void *message, size_t length)
{
   error_t error;
   uint8_t type;
   const char_t *label;

   //Empty message?
   if(!length)
      return ERROR_INVALID_LENGTH;

   //Retrieve the message type
   type = *((uint8_t *) message);
   //Get the corresponding label
   label = (type < arraysize(messageLabel)) ? messageLabel[type] : "Unknown";

   //Relay agent/server message?
   if(type == DHCPV6_MSG_TYPE_RELAY_FORW || type == DHCPV6_MSG_TYPE_RELAY_REPL)
   {
      //Ensure the length of the DHCPv6 message is acceptable
      if(length < sizeof(Dhcpv6RelayMessage))
      {
         //Report an error
         error = ERROR_INVALID_LENGTH;
      }
      else
      {
         //Point to the DHCPv6 message
         const Dhcpv6RelayMessage *relayMessage = message;

         //Dump message header
         TRACE_DEBUG("  Message Type = %" PRIu8 " (%s)\r\n", relayMessage->msgType, label);
         TRACE_DEBUG("  Hop Count = %" PRIu8 "\r\n", relayMessage->hopCount);
         TRACE_DEBUG("  Link Address = %s\r\n", ipv6AddrToString(&relayMessage->linkAddress, NULL));
         TRACE_DEBUG("  Peer Address = %s\r\n", ipv6AddrToString(&relayMessage->peerAddress, NULL));

         //Dump message options
         error = dhcpv6DumpOptions(relayMessage->options, length - sizeof(Dhcpv6RelayMessage), 1);
      }

   }
   //Client/server message?
   else
   {
      //Ensure the length of the DHCPv6 message is acceptable
      if(length < sizeof(Dhcpv6Message))
      {
         //Report an error
         error = ERROR_INVALID_LENGTH;
      }
      else
      {
         //Point to the DHCPv6 message
         const Dhcpv6Message *clientMessage = message;

         //Dump message header
         TRACE_DEBUG("  Message Type = %" PRIu8 " (%s)\r\n", clientMessage->msgType, label);
         TRACE_DEBUG("  Transaction ID = 0x%06" PRIX32 "\r\n", LOAD24BE(clientMessage->transactionId));

         //Dump message options
         error = dhcpv6DumpOptions(clientMessage->options, length - sizeof(Dhcpv6Message), 1);
      }
   }

   //Did we encounter an error?
   if(error)
   {
      //Debug message
      TRACE_WARNING("DHCPv6 message is not valid!\r\n");
      //Dump message contents for debugging purpose
      TRACE_DEBUG_ARRAY("  ", message, length);
   }

   //Return status code
   return error;
}


/**
 * @brief Dump DHCPv6 options for debugging purpose
 * @param[in] options Pointer to the DHCPv6 options to dump
 * @param[in] length Length of the options
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpOptions(const uint8_t *options, size_t length, uint_t level)
{
   error_t error;
   size_t i;
   Dhcpv6Option *option;

   //Check whether the maximum level of recursion is reached
   if(level >= 6)
   {
      //If the maximum level of recursion is reached, then dump contents
      TRACE_DEBUG("%sOptions (%" PRIuSIZE " bytes)\r\n", prefix[level], length);
      TRACE_DEBUG_ARRAY(prefix[level + 1], options, length);
      //Exit immediately
      return NO_ERROR;
   }

   //Parse DHCPv6 options
   for(i = 0; i < length; )
   {
      //Point to the current option
      option = (Dhcpv6Option *) (options + i);

      //Make sure the option is valid
      if((i + sizeof(Dhcpv6Option)) > length)
         return ERROR_INVALID_OPTION;
      //Check the length of the option data
      if((i + sizeof(Dhcpv6Option) + ntohs(option->length)) > length)
         return ERROR_INVALID_OPTION;

      //Check option code
      switch(ntohs(option->code))
      {
      //Client Identifier option
      case DHCPV6_OPTION_CLIENTID:
         error = dhcpv6DumpClientIdOption(option, level);
         break;
      //Server Identifier option
      case DHCPV6_OPTION_SERVERID:
         error = dhcpv6DumpServerIdOption(option, level);
         break;
      //IA_NA option
      case DHCPV6_OPTION_IA_NA:
         error = dhcpv6DumpIaNaOption(option, level);
         break;
      //IA_TA option
      case DHCPV6_OPTION_IA_TA:
         error = dhcpv6DumpIaTaOption(option, level);
         break;
      //IA Address option
      case DHCPV6_OPTION_IAADDR:
         error = dhcpv6DumpIaAddrOption(option, level);
         break;
      //Option Request option
      case DHCPV6_OPTION_ORO:
         error = dhcpv6DumpOroOption(option, level);
         break;
      //Preference option
      case DHCPV6_OPTION_PREFERENCE:
         error = dhcpv6DumpPreferenceOption(option, level);
         break;
      //Elapsed Time option
      case DHCPV6_OPTION_ELAPSED_TIME:
         error = dhcpv6DumpElapsedTimeOption(option, level);
         break;
      //Relay Message option
      case DHCPV6_OPTION_RELAY_MSG:
         error = dhcpv6DumpRelayMessageOption(option, level);
         break;
      //Authentication option
      case DHCPV6_OPTION_AUTH:
         error = dhcpv6DumpAuthOption(option, level);
         break;
      //Server Unicast option
      case DHCPV6_OPTION_UNICAST:
         error = dhcpv6DumpServerUnicastOption(option, level);
         break;
      //Status Code option
      case DHCPV6_OPTION_STATUS_CODE:
         error = dhcpv6DumpStatusCodeOption(option, level);
         break;
      //Rapid Commit option
      case DHCPV6_OPTION_RAPID_COMMIT:
         error = dhcpv6DumpRapidCommitOption(option, level);
         break;
      //User Class option
      case DHCPV6_OPTION_USER_CLASS:
         error = dhcpv6DumpUserClassOption(option, level);
         break;
      //Vendor Class option
      case DHCPV6_OPTION_VENDOR_CLASS:
         error = dhcpv6DumpVendorClassOption(option, level);
         break;
      //Vendor Specific Information option
      case DHCPV6_OPTION_VENDOR_OPTS:
         error = dhcpv6DumpVendorSpecificInfoOption(option, level);
         break;
      //Interface ID option
      case DHCPV6_OPTION_INTERFACE_ID:
         error = dhcpv6DumpInterfaceIdOption(option, level);
         break;
      //Reconfigure Message option
      case DHCPV6_OPTION_RECONF_MSG:
         error = dhcpv6DumpReconfMessageOption(option, level);
         break;
      //Reconfigure Accept option
      case DHCPV6_OPTION_RECONF_ACCEPT:
         error = dhcpv6DumpReconfAcceptOption(option, level);
         break;
      //DNS Recursive Name Server option
      case DHCPV6_OPTION_DNS_SERVERS:
         error = dhcpv6DumpDnsServersOption(option, level);
         break;
      //Domain Search List option
      case DHCPV6_OPTION_DOMAIN_LIST:
         error = dhcpv6DumpDomainListOption(option, level);
         break;
      //Unknown option...
      default:
         error = dhcpv6DumpGenericOption(option, level);
         break;
      }

      //Failed to parse current option?
      if(error)
         return error;

      //Jump to the next option
      i += sizeof(Dhcpv6Option) + ntohs(option->length);
   }

   //No error to report
   return NO_ERROR;

}


/**
 * @brief Dump generic DHCPv6 option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpGenericOption(const Dhcpv6Option *option, uint_t level)
{
   //Dump contents
   TRACE_DEBUG("%sOption %" PRIu16 " (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->code), ntohs(option->length));
   TRACE_DEBUG_ARRAY(prefix[level + 1], option->value, ntohs(option->length));

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Client Identifier option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpClientIdOption(const Dhcpv6Option *option, uint_t level)
{
   //Dump contents
   TRACE_DEBUG("%sClient Identifier option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG_ARRAY(prefix[level + 1], option->value, ntohs(option->length));

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Server Identifier option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpServerIdOption(const Dhcpv6Option *option, uint_t level)
{
   //Dump contents
   TRACE_DEBUG("%sServer Identifier option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG_ARRAY(prefix[level + 1], option->value, ntohs(option->length));

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump IA_NA option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpIaNaOption(const Dhcpv6Option *option, uint_t level)
{
   Dhcpv6IaNaOption *iaNaOption;

   //Check the length of the option
   if(ntohs(option->length) < sizeof(Dhcpv6IaNaOption))
      return ERROR_INVALID_OPTION;

   //Point to the option contents
   iaNaOption = (Dhcpv6IaNaOption *) option->value;

   //Dump contents
   TRACE_DEBUG("%sIA_NA option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG("%sIA ID = 0x%08" PRIX32 "\r\n", prefix[level + 1], ntohl(iaNaOption->iaId));
   TRACE_DEBUG("%sT1 = %" PRIu32 "s\r\n", prefix[level + 1], ntohl(iaNaOption->t1));
   TRACE_DEBUG("%sT2 = %" PRIu32 "s\r\n", prefix[level + 1], ntohl(iaNaOption->t2));

   //Dump the options associated with this IA_NA
   dhcpv6DumpOptions(iaNaOption->options, ntohs(option->length) - sizeof(Dhcpv6IaNaOption), level + 1);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump IA_TA option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpIaTaOption(const Dhcpv6Option *option, uint_t level)
{
   Dhcpv6IaTaOption *iaTaOption;

   //Check the length of the option
   if(ntohs(option->length) < sizeof(Dhcpv6IaTaOption))
      return ERROR_INVALID_OPTION;

   //Point to the option contents
   iaTaOption = (Dhcpv6IaTaOption *) option->value;

   //Dump contents
   TRACE_DEBUG("%sIA_TA option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG("%sIA ID = 0x%08" PRIX32 "\r\n", prefix[level + 1], ntohl(iaTaOption->iaId));

   //Dump the options associated with this IA_TA
   dhcpv6DumpOptions(iaTaOption->options, ntohs(option->length) - sizeof(Dhcpv6IaTaOption), level + 1);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump IA Address option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpIaAddrOption(const Dhcpv6Option *option, uint_t level)
{
   Dhcpv6IaAddrOption *iaAddrOption;

   //Check the length of the option
   if(ntohs(option->length) < sizeof(Dhcpv6IaAddrOption))
      return ERROR_INVALID_OPTION;

   //Point to the option contents
   iaAddrOption = (Dhcpv6IaAddrOption *) option->value;

   //Dump contents
   TRACE_DEBUG("%sIA Address option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG("%sIPv6 Address = %s\r\n", prefix[level + 1], ipv6AddrToString(&iaAddrOption->address, NULL));
   TRACE_DEBUG("%sPreferred Lifetime = %" PRIu32 "s\r\n", prefix[level + 1], ntohl(iaAddrOption->preferredLifetime));
   TRACE_DEBUG("%sValid Lifetime = %" PRIu32 "s\r\n", prefix[level + 1], ntohl(iaAddrOption->validLifetime));

   //Dump the options associated with this IA address
   dhcpv6DumpOptions(iaAddrOption->options, ntohs(option->length) - sizeof(Dhcpv6IaAddrOption), level + 1);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Option Request option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpOroOption(const Dhcpv6Option *option, uint_t level)
{
   uint_t i;
   uint_t n;
   uint16_t code;
   const char_t *label;
   Dhcpv6OroOption *oroOption;

   //Check the length of the option
   if(ntohs(option->length) % 2)
      return ERROR_INVALID_OPTION;

   //Point to the option contents
   oroOption = (Dhcpv6OroOption *) option->value;
   //Get the number of requested options
   n = ntohs(option->length) / 2;

   //Dump contents
   TRACE_DEBUG("%sOption Request option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));

   //Parse the list of requested options
   for(i = 0; i < n; i++)
   {
      //Get current option code
      code = ntohs(oroOption->requestedOption[i]);
      //Find the name associated with this option code
      label = (code < arraysize(optionLabel)) ? optionLabel[code] : "Unknown";
      //Display option code and option name
      TRACE_DEBUG("%s%" PRIu16 " (%s option)\r\n", prefix[level + 1], code, label);
   }

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Preference option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpPreferenceOption(const Dhcpv6Option *option, uint_t level)
{
   Dhcpv6PreferenceOption *preferenceOption;

   //Check the length of the option
   if(ntohs(option->length) != sizeof(Dhcpv6PreferenceOption))
      return ERROR_INVALID_OPTION;

   //Point to the option contents
   preferenceOption = (Dhcpv6PreferenceOption *) option->value;

   //Dump contents
   TRACE_DEBUG("%sPreference option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG("%s%" PRIu8 "\r\n", prefix[level + 1], preferenceOption->value);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Elapsed Time option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpElapsedTimeOption(const Dhcpv6Option *option, uint_t level)
{
   uint32_t value;
   Dhcpv6ElapsedTimeOption *elapsedTimeOption;

   //Check the length of the option
   if(ntohs(option->length) != sizeof(Dhcpv6ElapsedTimeOption))
      return ERROR_INVALID_OPTION;

   //Point to the option contents
   elapsedTimeOption = (Dhcpv6ElapsedTimeOption *) option->value;
   //Convert the value to milliseconds
   value = ntohs(elapsedTimeOption->value) * 10;

   //Dump contents
   TRACE_DEBUG("%sElapsed Time option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG("%s%" PRIu32 "ms\r\n", prefix[level + 1], value);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Relay Message option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpRelayMessageOption(const Dhcpv6Option *option, uint_t level)
{
   uint8_t type;
   const char_t *label;

   //Check the length of the option
   if(!ntohs(option->length))
      return ERROR_INVALID_OPTION;

   //Retrieve the message type
   type = option->value[0];
   //Get the corresponding label
   label = (type < arraysize(messageLabel)) ? messageLabel[type] : "Unknown";

   //Relay agent/server message?
   if(type == DHCPV6_MSG_TYPE_RELAY_FORW || type == DHCPV6_MSG_TYPE_RELAY_REPL)
   {
      //Get the inner message
      const Dhcpv6RelayMessage *message = (Dhcpv6RelayMessage *) option->value;

      //Ensure the length of the DHCPv6 message is acceptable
      if(ntohs(option->length) < sizeof(Dhcpv6RelayMessage))
         return ERROR_INVALID_OPTION;

      //Dump message header
      TRACE_DEBUG("%sRelay Message option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
      TRACE_DEBUG("%sMessage Type = %" PRIu8 " (%s)\r\n", prefix[level + 1], message->msgType, label);
      TRACE_DEBUG("%sHop Count = %" PRIu8 "\r\n", prefix[level + 1], message->hopCount);
      TRACE_DEBUG("%sLink Address = %s\r\n", prefix[level + 1], ipv6AddrToString(&message->linkAddress, NULL));
      TRACE_DEBUG("%sPeer Address = %s\r\n", prefix[level + 1], ipv6AddrToString(&message->peerAddress, NULL));

      //Dump message options
      return dhcpv6DumpOptions(message->options, ntohs(option->length) - sizeof(Dhcpv6RelayMessage), level + 1);
   }
   //Client/server message?
   else
   {
      //Get the inner message
      const Dhcpv6Message *message = (Dhcpv6Message *) option->value;

      //Ensure the length of the DHCPv6 message is acceptable
      if(ntohs(option->length) < sizeof(Dhcpv6Message))
         return ERROR_INVALID_OPTION;

      //Dump message header
      TRACE_DEBUG("%sRelay Message option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
      TRACE_DEBUG("%sMessage Type = %" PRIu8 " (%s)\r\n", prefix[level + 1], message->msgType, label);
      TRACE_DEBUG("%sTransaction ID = 0x%06" PRIX32 "\r\n", prefix[level + 1], LOAD24BE(message->transactionId));

      //Dump message options
      return dhcpv6DumpOptions(message->options, ntohs(option->length) - sizeof(Dhcpv6Message), level + 1);
   }
}


/**
 * @brief Dump Authentication option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpAuthOption(const Dhcpv6Option *option, uint_t level)
{
   size_t n;
   Dhcpv6AuthOption *authOption;

   //Check the length of the option
   if(ntohs(option->length) < sizeof(Dhcpv6AuthOption))
      return ERROR_INVALID_OPTION;

   //Point to the option contents
   authOption = (Dhcpv6AuthOption *) option->value;
   //Get the length of the authentication information
   n = ntohs(option->length) - sizeof(Dhcpv6AuthOption);

   //Dump contents
   TRACE_DEBUG("%sAuthentication option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG("%sProtocol = %" PRIu8 "\r\n", prefix[level + 1], authOption->protocol);
   TRACE_DEBUG("%sAlgorithm = %" PRIu8 "\r\n", prefix[level + 1], authOption->algorithm);
   TRACE_DEBUG("%sRDM = %" PRIu8 "\r\n", prefix[level + 1], authOption->rdm);
   TRACE_DEBUG("%sReplay Detection\r\n", prefix[level + 1]);
   TRACE_DEBUG_ARRAY(prefix[level + 2], authOption->replayDetection, 8);
   TRACE_DEBUG("%sAuthentication Information (%" PRIuSIZE " bytes)\r\n", prefix[level + 1], n);
   TRACE_DEBUG_ARRAY(prefix[level + 2], authOption->authInfo, n);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Server Unicast option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpServerUnicastOption(const Dhcpv6Option *option, uint_t level)
{
   Dhcpv6ServerUnicastOption *serverUnicastOption;

   //Check the length of the option
   if(ntohs(option->length) != sizeof(Dhcpv6ServerUnicastOption))
      return ERROR_INVALID_OPTION;

   //Point to the option contents
   serverUnicastOption = (Dhcpv6ServerUnicastOption *) option->value;

   //Dump contents
   TRACE_DEBUG("%sServer Unicast option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG("%s%s\r\n", prefix[level + 1], ipv6AddrToString(&serverUnicastOption->serverAddr, NULL));

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Status Code option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpStatusCodeOption(const Dhcpv6Option *option, uint_t level)
{
   uint16_t code;
   const char_t *label;
   Dhcpv6StatusCodeOption *statusCodeOption;

   //Check the length of the option
   if(ntohs(option->length) < sizeof(Dhcpv6StatusCodeOption))
      return ERROR_INVALID_OPTION;

   //Point to the option contents
   statusCodeOption = (Dhcpv6StatusCodeOption *) option->value;
   //Get the status code
   code = ntohs(statusCodeOption->statusCode);
   //Get the label associated with the status code
   label = (code < arraysize(statusLabel)) ? statusLabel[code] : "Unknown";

   //Dump contents
   TRACE_DEBUG("%sStatus Code option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG("%sCode = %" PRIu16 " (%s)\r\n", prefix[level + 1], code, label);
   TRACE_DEBUG("%sMessage = %s\r\n", prefix[level + 1], statusCodeOption->statusMessage);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Rapid Commit option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpRapidCommitOption(const Dhcpv6Option *option, uint_t level)
{
   //Check the length of the option
   if(ntohs(option->length) != 0)
      return ERROR_INVALID_OPTION;

   //Dump contents
   TRACE_DEBUG("%sRapid Commit option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump User Class option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpUserClassOption(const Dhcpv6Option *option, uint_t level)
{
   //Dump contents
   TRACE_DEBUG("%sUser Class option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG_ARRAY(prefix[level + 1], option->value, ntohs(option->length));

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Vendor Class option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpVendorClassOption(const Dhcpv6Option *option, uint_t level)
{
   //Dump contents
   TRACE_DEBUG("%sVendor Class option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG_ARRAY(prefix[level + 1], option->value, ntohs(option->length));

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Vendor Specific Information option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpVendorSpecificInfoOption(const Dhcpv6Option *option, uint_t level)
{
   //Dump contents
   TRACE_DEBUG("%sVendor Specific Information option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG_ARRAY(prefix[level + 1], option->value, ntohs(option->length));

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Interface ID option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpInterfaceIdOption(const Dhcpv6Option *option, uint_t level)
{
   //Dump contents
   TRACE_DEBUG("%sInterface ID option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG_ARRAY(prefix[level + 1], option->value, ntohs(option->length));

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Reconfigure Message option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpReconfMessageOption(const Dhcpv6Option *option, uint_t level)
{
   Dhcpv6ReconfMessageOption *reconfMessageOption;

   //Check the length of the option
   if(ntohs(option->length) != sizeof(Dhcpv6ReconfMessageOption))
      return ERROR_INVALID_OPTION;

   //Point to the option contents
   reconfMessageOption = (Dhcpv6ReconfMessageOption *) option->value;

   //Dump contents
   TRACE_DEBUG("%sReconfigure Message option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG("%sMessage Type = %" PRIu8 "\r\n", prefix[level + 1], reconfMessageOption->msgType);

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Reconfigure Accept option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpReconfAcceptOption(const Dhcpv6Option *option, uint_t level)
{
   //Check the length of the option
   if(ntohs(option->length) != 0)
      return ERROR_INVALID_OPTION;

   //Dump contents
   TRACE_DEBUG("%sReconfigure Accept option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump DNS Recursive Name Server option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpDnsServersOption(const Dhcpv6Option *option, uint_t level)
{
   uint_t i;
   uint_t n;
   Dhcpv6DnsServersOption *dnsServersOption;

   //Check the length of the option
   if(ntohs(option->length) % sizeof(Ipv6Addr))
      return ERROR_INVALID_OPTION;

   //Point to the option contents
   dnsServersOption = (Dhcpv6DnsServersOption *) option->value;
   //Calculate the number of IPv6 addresses in the list
   n = ntohs(option->length) / sizeof(Ipv6Addr);

   //Dump contents
   TRACE_DEBUG("%sDNS Recursive Name Server option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));

   //Diplay the DNS servers
   for(i = 0; i < n; i++)
      TRACE_DEBUG("%s%s\r\n", prefix[level + 1], ipv6AddrToString(dnsServersOption->address + i, NULL));

   //No error to report
   return NO_ERROR;
}


/**
 * @brief Dump Domain Search List option
 * @param[in] option Pointer to the option to dump
 * @param[in] level Current level of recursion
 * @return Error code
 **/

error_t dhcpv6DumpDomainListOption(const Dhcpv6Option *option, uint_t level)
{
   //Dump contents
   TRACE_DEBUG("%sDomain Search List option (%" PRIu16 " bytes)\r\n", prefix[level], ntohs(option->length));
   TRACE_DEBUG_ARRAY(prefix[level + 1], option->value, ntohs(option->length));

   //No error to report
   return NO_ERROR;
}

#endif
