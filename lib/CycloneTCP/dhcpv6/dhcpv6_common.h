/**
 * @file dhcpv6_common.h
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
 * @author Oryx Embedded SARL (www.oryx-embedded.com)
 * @version 2.0.4
 **/

#ifndef _DHCPV6_COMMON_H
#define _DHCPV6_COMMON_H

//Dependencies
#include "core/net.h"
#include "core/ethernet.h"
#include "ipv6/ipv6.h"

//UDP ports used by DHCPv6 clients and servers
#define DHCPV6_CLIENT_PORT 546
#define DHCPV6_SERVER_PORT 547

//Maximum DHCPv6 message size
#define DHCPV6_MAX_MSG_SIZE 1232
//Maximum DUID size (128 octets not including the type code)
#define DHCPV6_MAX_DUID_SIZE 130

//Maximum hop count in a relay-forward message
#define DHCPV6_HOP_COUNT_LIMIT 32
//Highest server preference value
#define DHCPV6_MAX_SERVER_PREFERENCE 255
//Infinite lifetime representation
#define DHCPV6_INFINITE_TIME 0xFFFFFFFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief DUID types
 **/

typedef enum
{
   DHCPV6_DUID_LLT = 1,
   DHCPV6_DUID_EN  = 2,
   DHCPV6_DUID_LL  = 3
} Dhcpv6DuidType;


/**
 * @brief Hardware types
 **/

typedef enum
{
   DHCPV6_HARDWARE_TYPE_ETH   = 1,
   DHCPV6_HARDWARE_TYPE_EUI64 = 27
} Dhcpv6HardwareType;


/**
 * @brief DHCPv6 message types
 **/

typedef enum
{
   DHCPV6_MSG_TYPE_SOLICIT      = 1,
   DHCPV6_MSG_TYPE_ADVERTISE    = 2,
   DHCPV6_MSG_TYPE_REQUEST      = 3,
   DHCPV6_MSG_TYPE_CONFIRM      = 4,
   DHCPV6_MSG_TYPE_RENEW        = 5,
   DHCPV6_MSG_TYPE_REBIND       = 6,
   DHCPV6_MSG_TYPE_REPLY        = 7,
   DHCPV6_MSG_TYPE_RELEASE      = 8,
   DHCPV6_MSG_TYPE_DECLINE      = 9,
   DHCPV6_MSG_TYPE_RECONFIGURE  = 10,
   DHCPV6_MSG_TYPE_INFO_REQUEST = 11,
   DHCPV6_MSG_TYPE_RELAY_FORW   = 12,
   DHCPV6_MSG_TYPE_RELAY_REPL   = 13
} Dhcpv6MessageType;


/**
 * @brief DHCPv6 option codes
 **/

typedef enum
{
   DHCPV6_OPTION_CLIENTID       = 1,
   DHCPV6_OPTION_SERVERID       = 2,
   DHCPV6_OPTION_IA_NA          = 3,
   DHCPV6_OPTION_IA_TA          = 4,
   DHCPV6_OPTION_IAADDR         = 5,
   DHCPV6_OPTION_ORO            = 6,
   DHCPV6_OPTION_PREFERENCE     = 7,
   DHCPV6_OPTION_ELAPSED_TIME   = 8,
   DHCPV6_OPTION_RELAY_MSG      = 9,
   DHCPV6_OPTION_AUTH           = 11,
   DHCPV6_OPTION_UNICAST        = 12,
   DHCPV6_OPTION_STATUS_CODE    = 13,
   DHCPV6_OPTION_RAPID_COMMIT   = 14,
   DHCPV6_OPTION_USER_CLASS     = 15,
   DHCPV6_OPTION_VENDOR_CLASS   = 16,
   DHCPV6_OPTION_VENDOR_OPTS    = 17,
   DHCPV6_OPTION_INTERFACE_ID   = 18,
   DHCPV6_OPTION_RECONF_MSG     = 19,
   DHCPV6_OPTION_RECONF_ACCEPT  = 20,
   DHCPV6_OPTION_DNS_SERVERS    = 23,
   DHCPV6_OPTION_DOMAIN_LIST    = 24,
   DHCPV6_OPTION_IA_PD          = 25,
   DHCPV6_OPTION_IAPREFIX       = 26,
   DHCPV6_OPTION_FQDN           = 39,
   DHCPV6_OPTION_CAPTIVE_PORTAL = 103
} Dhcpv6OptionCode;


/**
 * @brief Status code
 **/

typedef enum
{
   DHCPV6_STATUS_SUCCESS            = 0,
   DHCPV6_STATUS_UNSPEC_FAILURE     = 1,
   DHCPV6_STATUS_NO_ADDRS_AVAILABLE = 2,
   DHCPV6_STATUS_NO_BINDING         = 3,
   DHCPV6_STATUS_NOT_ON_LINK        = 4,
   DHCPV6_STATUS_USE_MULTICAST      = 5
} Dhcpv6StatusCode;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief DUID-LLT structure
 **/

typedef __start_packed struct
{
   uint16_t type;           //0-1
   uint16_t hardwareType;   //2-3
   uint32_t time;           //4-7
   MacAddr linkLayerAddr;   //8-13
} __end_packed Dhcpv6DuidLlt;


/**
 * @brief DUID-EN structure
 **/

typedef __start_packed struct
{
   uint16_t type;             //0-1
   uint32_t enterpriseNumber; //2-5
   uint8_t identifier[];      //6
} __end_packed Dhcpv6DuidEn;


/**
 * @brief DUID-LL structure
 **/

typedef __start_packed struct
{
   uint16_t type;         //0-1
   uint16_t hardwareType; //2-3
#if (ETH_SUPPORT == ENABLED)
   MacAddr linkLayerAddr; //4-9
#else
   Eui64 linkLayerAddr;   //4-11
#endif
} __end_packed Dhcpv6DuidLl;


/**
 * @brief DHCPv6 message
 **/

typedef __start_packed struct
{
   uint8_t msgType;          //0
   uint8_t transactionId[3]; //1-3
   uint8_t options[];        //4
} __end_packed Dhcpv6Message;


/**
 * @brief DHCPv6 relay agent message
 **/

typedef __start_packed struct
{
   uint8_t msgType;      //0
   uint8_t hopCount;     //1
   Ipv6Addr linkAddress; //2-17
   Ipv6Addr peerAddress; //18-33
   uint8_t options[];    //34
} __end_packed Dhcpv6RelayMessage;


/**
 * @brief DHCPv6 option
 **/

typedef __start_packed struct
{
   uint16_t code;   //0-1
   uint16_t length; //2-3
   uint8_t value[]; //4
} __end_packed Dhcpv6Option;


/**
 * @brief Identity Association for Non-temporary Addresses option
 **/

typedef __start_packed struct
{
   uint32_t iaId;     //0-3
   uint32_t t1;       //4-7
   uint32_t t2;       //8-11
   uint8_t options[]; //12
} __end_packed Dhcpv6IaNaOption;


/**
 * @brief Identity Association for Temporary Addresses option
 **/

typedef __start_packed struct
{
   uint32_t iaId;     //0-3
   uint8_t options[]; //4
} Dhcpv6IaTaOption;


/**
 * @brief IA Address option
 **/

typedef __start_packed struct
{
   Ipv6Addr address;           //0-15
   uint32_t preferredLifetime; //16-19
   uint32_t validLifetime;     //20-23
   uint8_t options[];          //24
} __end_packed Dhcpv6IaAddrOption;


/**
 * @brief Option Request option
 **/

typedef __start_packed struct
{
   uint16_t requestedOption[1]; //0-1
} __end_packed Dhcpv6OroOption;


/**
 * @brief Preference option
 **/

typedef __start_packed struct
{
   uint8_t value; //0
} __end_packed Dhcpv6PreferenceOption;


/**
 * @brief Elapsed Time option
 **/

typedef __start_packed struct
{
   uint16_t value; //0-1
} __end_packed Dhcpv6ElapsedTimeOption;


/**
 * @brief Authentication option
 **/

typedef __start_packed struct
{
   uint8_t protocol;           //0
   uint8_t algorithm;          //1
   uint8_t rdm;                //2
   uint8_t replayDetection[8]; //3-10
   uint8_t authInfo[];         //11
} __end_packed Dhcpv6AuthOption;


/**
 * @brief Server Unicast option
 **/

typedef __start_packed struct
{
   Ipv6Addr serverAddr; //0-15
} __end_packed Dhcpv6ServerUnicastOption;


/**
 * @brief Status Code option
 **/

typedef __start_packed struct
{
   uint16_t statusCode;    //0-1
   char_t statusMessage[]; //2
} __end_packed Dhcpv6StatusCodeOption;


/**
 * @brief Reconfigure Message option
 **/

typedef __start_packed struct
{
   uint8_t msgType; //0
} __end_packed Dhcpv6ReconfMessageOption;


/**
 * @brief DNS Recursive Name Server option
 **/

typedef __start_packed struct
{
   Ipv6Addr address[1]; //0-15
} __end_packed Dhcpv6DnsServersOption;


/**
 * @brief Domain Search List option
 **/

typedef __start_packed struct
{
   uint8_t searchList[1]; //0
} __end_packed Dhcpv6DomainListOption;


/**
 * @brief Identity Association for Prefix Delegation Option
 **/

typedef __start_packed struct
{
   uint32_t iaId;     //0-3
   uint32_t t1;       //4-7
   uint32_t t2;       //8-11
   uint8_t options[]; //12
} __end_packed Dhcpv6IaPdOption;


/**
 * @brief IA_PD Prefix option
 **/

typedef __start_packed struct
{
   uint32_t preferredLifetime; //0-3
   uint32_t validLifetime;     //4-7
   uint8_t prefixLen;          //8
   Ipv6Addr prefix;            //9-24
   uint8_t options[];          //25
} __end_packed Dhcpv6IaPrefixOption;


/**
 * @brief Fully Qualified Domain Name option
 **/

typedef __start_packed struct
{
#if defined(_CPU_BIG_ENDIAN) && !defined(__IAR_SYSTEMS_ICC__)
   uint8_t mbz : 5;      //0
   uint8_t n : 1;
   uint8_t o : 1;
   uint8_t s : 1;
#else
   uint8_t s : 1;        //0
   uint8_t o : 1;
   uint8_t n : 1;
   uint8_t mbz : 5;
#endif
   uint8_t domainName[]; //1
} __end_packed Dhcpv6FqdnOption;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


//DHCPv6 related constants
extern const Ipv6Addr DHCPV6_ALL_RELAY_AGENTS_AND_SERVERS_ADDR;
extern const Ipv6Addr DHCPV6_ALL_SERVERS_ADDR;

//DHCPv6 related functions
Dhcpv6StatusCode dhcpv6GetStatusCode(const uint8_t *options, size_t length);

Dhcpv6Option *dhcpv6AddOption(void *message, size_t *messageLen,
   uint16_t optionCode, const void *optionValue, size_t optionLen);

Dhcpv6Option *dhcpv6AddSubOption(Dhcpv6Option *baseOption, size_t *messageLen,
   uint16_t optionCode, const void *optionValue, size_t optionLen);

Dhcpv6Option *dhcpv6GetOption(const uint8_t *options,
   size_t optionsLength, uint16_t optionCode);

int32_t dhcpv6Rand(int32_t value);
int32_t dhcpv6RandRange(int32_t min, int32_t max);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
