/**
 * @file dhcpv6_common.c
 * @brief Definitions common to DHCPv6 client, server and relay agent
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
 * The Dynamic Host Configuration Protocol for IPv6 enables DHCP servers to
 * pass configuration parameters such as IPv6 network addresses to IPv6
 * nodes. This protocol is a stateful counterpart to IPv6 Stateless Address
 * Autoconfiguration (RFC 2462), and can be used separately or concurrently
 * with the latter to obtain configuration parameters. Refer to RFC 3315
 *
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

//Switch to the appropriate trace level
#define TRACE_LEVEL DHCPV6_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "dhcpv6/dhcpv6_common.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (IPV6_SUPPORT == ENABLED)

//All DHCPv6 relay agents and servers (FF02::1:2)
const Ipv6Addr DHCPV6_ALL_RELAY_AGENTS_AND_SERVERS_ADDR =
   IPV6_ADDR(0xFF02, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0002);

//All DHCPv6 servers (FF05::1:3)
const Ipv6Addr DHCPV6_ALL_SERVERS_ADDR =
   IPV6_ADDR(0xFF05, 0x0000, 0x0000, 0x0000, 0x0000, 0x0000, 0x0001, 0x0003);


/**
 * @brief Retrieve status code
 *
 * This function returns a status indication related to the DHCPv6
 * message or option in which it appears
 *
 * @param[in] options Pointer to the Options field
 * @param[in] length Length of the Options field
 * @return Status code
 **/

Dhcpv6StatusCode dhcpv6GetStatusCode(const uint8_t *options, size_t length)
{
   uint16_t statusCode;
   Dhcpv6Option *option;
   Dhcpv6StatusCodeOption *statusCodeOption;

   //Search for the Status Code option
   option = dhcpv6GetOption(options, length, DHCPV6_OPTION_STATUS_CODE);

   //Check whether the option has been found
   if(option != NULL && ntohs(option->length) >= sizeof(Dhcpv6StatusCodeOption))
   {
      //The option contains a status code and a status message
      statusCodeOption = (Dhcpv6StatusCodeOption *) option->value;

      //Convert the status code from network byte order
      statusCode = ntohs(statusCodeOption->statusCode);
   }
   else
   {
      //If the Status Code option does not appear in a message in which the option
      //could appear, the status of the message is assumed to be Success
      statusCode = DHCPV6_STATUS_SUCCESS;
   }

   //Return status code
   return (Dhcpv6StatusCode) statusCode;
}


/**
 * @brief Add an option to a DHCPv6 message
 * @param[in] message Pointer to the DHCPv6 message
 * @param[in,out] messageLen Length of the overall DHCPv6 message
 * @param[in] optionCode Option code
 * @param[in] optionValue Option value
 * @param[in] optionLen Length of the option value
 * @return If the option was successfully added, a pointer to the freshly
 *   created option is returned. Otherwise NULL pointer is returned
 **/

Dhcpv6Option *dhcpv6AddOption(void *message, size_t *messageLen,
   uint16_t optionCode, const void *optionValue, size_t optionLen)
{
   Dhcpv6Option *option;

   //Check the length of the DHCPv6 message
   if(*messageLen < sizeof(Dhcpv6Message))
      return NULL;
   //Check the length of the option
   if(optionLen > UINT16_MAX)
      return NULL;

   //Make sure there is enough room to add the option
   if((*messageLen + sizeof(Dhcpv6Option) + optionLen) > DHCPV6_MAX_MSG_SIZE)
      return NULL;

   //Point to the end of the DHCPv6 message
   option = (Dhcpv6Option *) ((uint8_t *) message + *messageLen);
   //Write specified option at current location
   option->code = htons(optionCode);
   option->length = htons(optionLen);
   //Copy option data
   osMemcpy(option->value, optionValue, optionLen);

   //Update the length of the DHCPv6 message
   *messageLen += sizeof(Dhcpv6Option) + optionLen;
   //Return a pointer to the freshly created option
   return option;
}


/**
 * @brief Add a suboption under an existing base option
 * @param[in] baseOption Pointer to the base option
 * @param[in,out] messageLen Length of the overall DHCPv6 message
 * @param[in] optionCode Option code
 * @param[in] optionValue Option value
 * @param[in] optionLen Length of the option value
 * @return If the option was successfully added, a pointer to the freshly
 *   created option is returned. Otherwise NULL pointer is returned
 **/

Dhcpv6Option *dhcpv6AddSubOption(Dhcpv6Option *baseOption, size_t *messageLen,
   uint16_t optionCode, const void *optionValue, size_t optionLen)
{
   uint_t n;
   Dhcpv6Option *option;

   //The pointer to the base option must be valid
   if(baseOption == NULL)
      return NULL;
   //Check the length of the DHCPv6 message
   if(*messageLen < sizeof(Dhcpv6Message))
      return NULL;
   //Check the length of the suboption
   if(optionLen > UINT16_MAX)
      return NULL;

   //Make sure there is enough room to add the option
   if((*messageLen + sizeof(Dhcpv6Option) + optionLen) > DHCPV6_MAX_MSG_SIZE)
      return NULL;

   //Get the actual length of the base option
   n = ntohs(baseOption->length);

   //Point to the location that follows the base option
   option = (Dhcpv6Option *) (baseOption->value + n);

   //Write specified option at current location
   option->code = htons(optionCode);
   option->length = htons(optionLen);
   //Copy option data
   osMemcpy(option->value, optionValue, optionLen);

   //Update the length of the base option
   n += sizeof(Dhcpv6Option) + optionLen;
   //Convert the 16-bit value to network byte order
   baseOption->length = htons(n);

   //Update the length of the DHCPv6 message
   *messageLen += sizeof(Dhcpv6Option) + optionLen;
   //Return a pointer to the freshly created option
   return option;
}


/**
 * @brief Find the specified option in a DHCPv6 message
 * @param[in] options Pointer to the Options field
 * @param[in] optionsLength Length of the Options field
 * @param[in] optionCode Code of the option to find
 * @return If the specified option was found, a pointer to the corresponding
 *   option is returned. Otherwise NULL pointer is returned
 **/

Dhcpv6Option *dhcpv6GetOption(const uint8_t *options,
   size_t optionsLength, uint16_t optionCode)
{
   uint_t i;
   Dhcpv6Option *option;

   //Parse DHCPv6 options
   for(i = 0; i < optionsLength; )
   {
      //Point to the current option
      option = (Dhcpv6Option *) (options + i);

      //Make sure the option is valid
      if((i + sizeof(Dhcpv6Option)) > optionsLength)
         break;
      //Check the length of the option data
      if((i + sizeof(Dhcpv6Option) + ntohs(option->length)) > optionsLength)
         break;

      //Option code matches the specified one?
      if(ntohs(option->code) == optionCode)
         return option;

      //Jump to the next option
      i += sizeof(Dhcpv6Option) + ntohs(option->length);
   }

   //The specified option code was not found
   return NULL;
}


/**
 * @brief Multiplication by a randomization factor
 *
 * Each of the computations of a new RT include a randomization factor
 * RAND, which is a random number chosen with a uniform distribution
 * between -0.1 and +0.1. The randomization factor is included to
 * minimize synchronization of messages transmitted by DHCPv6 clients
 *
 * @param[in] value Input value
 * @return Value resulting from the randomization process
 **/

int32_t dhcpv6Rand(int32_t value)
{
   //Use a randomization factor chosen with a uniform
   //distribution between -0.1 and +0.1
   return value * dhcpv6RandRange(-100, 100) / 1000;
}


/**
 * @brief Get a random value in the specified range
 * @param[in] min Lower bound
 * @param[in] max Upper bound
 * @return Random value in the specified range
 **/

int32_t dhcpv6RandRange(int32_t min, int32_t max)
{
   //Return a random value in the given range
   return min + netGetRand() % (max - min + 1);
}

#endif
