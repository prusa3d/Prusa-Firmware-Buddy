/**
 * @file dhcp_common.h
 * @brief Definitions common to DHCP client and server
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

#ifndef _DHCP_COMMON_H
#define _DHCP_COMMON_H

//Dependencies
#include "core/net.h"
#include "core/ethernet.h"
#include "ipv4/ipv4.h"

//UDP ports used by DHCP servers and clients
#define DHCP_SERVER_PORT 67
#define DHCP_CLIENT_PORT 68

//Minimum size of DHCP messages
#define DHCP_MIN_MSG_SIZE 300
//Maximum size of DHCP messages
#define DHCP_MAX_MSG_SIZE 548

//Hardware type
#define DHCP_HARDWARE_TYPE_ETH 1
//DHCP magic cookie
#define DHCP_MAGIC_COOKIE 0x63825363
//Infinite lifetime representation
#define DHCP_INFINITE_TIME 0xFFFFFFFF

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief DHCP opcodes
 **/

typedef enum
{
   DHCP_OPCODE_BOOTREQUEST = 1,
   DHCP_OPCODE_BOOTREPLY   = 2
} DhcpOpcode;


/**
 * @brief DHCP flags
 **/

typedef enum
{
   DHCP_FLAG_UNICAST   = 0x0000,
   DHCP_FLAG_BROADCAST = 0x8000
} DhcpFlags;


/**
 * @brief DHCP message types
 **/

typedef enum
{
   DHCP_MESSAGE_TYPE_DISCOVER = 1,
   DHCP_MESSAGE_TYPE_OFFER    = 2,
   DHCP_MESSAGE_TYPE_REQUEST  = 3,
   DHCP_MESSAGE_TYPE_DECLINE  = 4,
   DHCP_MESSAGE_TYPE_ACK      = 5,
   DHCP_MESSAGE_TYPE_NAK      = 6,
   DHCP_MESSAGE_TYPE_RELEASE  = 7,
   DHCP_MESSAGE_TYPE_INFORM   = 8
} DhcpMessageType;


/**
 * @brief DHCP option codes
 **/

typedef enum
{
   DHCP_OPT_PAD                          = 0,
   DHCP_OPT_SUBNET_MASK                  = 1,
   DHCP_OPT_TIME_OFFSET                  = 2,
   DHCP_OPT_ROUTER                       = 3,
   DHCP_OPT_TIME_SERVER                  = 4,
   DHCP_OPT_NAME_SERVER                  = 5,
   DHCP_OPT_DNS_SERVER                   = 6,
   DHCP_OPT_LOG_SERVER                   = 7,
   DHCP_OPT_COOKIE_SERVER                = 8,
   DHCP_OPT_LPR_SERVER                   = 9,
   DHCP_OPT_IMPRESS_SERVER               = 10,
   DHCP_OPT_RESOURCE_LOCATION_SERVER     = 11,
   DHCP_OPT_HOST_NAME                    = 12,
   DHCP_OPT_BOOT_FILE_SIZE               = 13,
   DHCP_OPT_MERIT_DUMP_FILE              = 14,
   DHCP_OPT_DOMAIN_NAME                  = 15,
   DHCP_OPT_SWAP_SERVER                  = 16,
   DHCP_OPT_ROOT_PATH                    = 17,
   DHCP_OPT_EXTENSIONS_PATH              = 18,
   DHCP_OPT_IP_FORWARDING                = 19,
   DHCP_OPT_NON_LOCAL_SOURCE_ROUTING     = 20,
   DHCP_OPT_POLICY_FILTER                = 21,
   DHCP_OPT_MAX_DATAGRAM_REASSEMBLY_SIZE = 22,
   DHCP_OPT_DEFAULT_IP_TTL               = 23,
   DHCP_OPT_PATH_MTU_AGING_TIMEOUT       = 24,
   DHCP_OPT_PATH_MTU_PLATEAU_TABLE       = 25,
   DHCP_OPT_INTERFACE_MTU                = 26,
   DHCP_OPT_ALL_SUBNETS_ARE_LOCAL        = 27,
   DHCP_OPT_BROADCAST_ADDRESS            = 28,
   DHCP_OPT_PERFORM_MASK_DISCOVERY       = 29,
   DHCP_OPT_MASK_SUPPLIER                = 30,
   DHCP_OPT_PERFORM_ROUTER_DISCOVERY     = 31,
   DHCP_OPT_ROUTER_SOLICITATION_ADDRESS  = 32,
   DHCP_OPT_STATIC_ROUTE                 = 33,
   DHCP_OPT_TRAILER_ENCAPSULATION        = 34,
   DHCP_OPT_ARP_CACHE_TIMEOUT            = 35,
   DHCP_OPT_ETHERNET_ENCAPSULATION       = 36,
   DHCP_OPT_TCP_DEFAULT_TTL              = 37,
   DHCP_OPT_TCP_KEEPALIVE_INTERVAL       = 38,
   DHCP_OPT_TCP_KEEPALIVE_GARBAGE        = 39,
   DHCP_OPT_NIS_DOMAIN                   = 40,
   DHCP_OPT_NIS_SERVER                   = 41,
   DHCP_OPT_NTP_SERVER                   = 42,
   DHCP_OPT_VENDOR_SPECIFIC_INFO         = 43,
   DHCP_OPT_NETBIOS_NBNS_SERVER          = 44,
   DHCP_OPT_NETBIOS_NBDD_SERVER          = 45,
   DHCP_OPT_NETBIOS_NODE_TYPE            = 46,
   DHCP_OPT_NETBIOS_SCOPE                = 47,
   DHCP_OPT_X11_FONT_SERVER              = 48,
   DHCP_OPT_X11_DISPLAY_MANAGER          = 49,
   DHCP_OPT_REQUESTED_IP_ADDRESS         = 50,
   DHCP_OPT_IP_ADDRESS_LEASE_TIME        = 51,
   DHCP_OPT_OPTION_OVERLOAD              = 52,
   DHCP_OPT_DHCP_MESSAGE_TYPE            = 53,
   DHCP_OPT_SERVER_IDENTIFIER            = 54,
   DHCP_OPT_PARAM_REQUEST_LIST           = 55,
   DHCP_OPT_MESSAGE                      = 56,
   DHCP_OPT_MAX_DHCP_MESSAGE_SIZE        = 57,
   DHCP_OPT_RENEWAL_TIME_VALUE           = 58,
   DHCP_OPT_REBINDING_TIME_VALUE         = 59,
   DHCP_OPT_VENDOR_CLASS_IDENTIFIER      = 60,
   DHCP_OPT_CLIENT_IDENTIFIER            = 61,
   DHCP_OPT_NISP_DOMAIN                  = 64,
   DHCP_OPT_NISP_SERVER                  = 65,
   DHCP_OPT_TFTP_SERVER_NAME             = 66,
   DHCP_OPT_BOOTFILE_NAME                = 67,
   DHCP_OPT_MOBILE_IP_HOME_AGENT         = 68,
   DHCP_OPT_SMTP_SERVER                  = 69,
   DHCP_OPT_POP3_SERVER                  = 70,
   DHCP_OPT_NNTP_SERVER                  = 71,
   DHCP_OPT_DEFAULT_WWW_SERVER           = 72,
   DHCP_OPT_DEFAULT_FINGER_SERVER        = 73,
   DHCP_OPT_DEFAULT_IRC_SERVER           = 74,
   DHCP_OPT_STREETTALK_SERVER            = 75,
   DHCP_OPT_STDA_SERVER                  = 76,
   DHCP_OPT_RAPID_COMMIT                 = 80,
   DHCP_OPT_CAPTIVE_PORTAL               = 160,
   DHCP_OPT_END                          = 255
} DhcpOptionCode;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief DHCP message
 **/

typedef __start_packed struct
{
   uint8_t op;           //0
   uint8_t htype;        //1
   uint8_t hlen;         //2
   uint8_t hops;         //3
   uint32_t xid;         //4-7
   uint16_t secs;        //8-9
   uint16_t flags;       //10-11
   Ipv4Addr ciaddr;      //12-15
   Ipv4Addr yiaddr;      //16-19
   Ipv4Addr siaddr;      //20-23
   Ipv4Addr giaddr;      //24-27
   MacAddr chaddr;       //28-33
   uint8_t unused[10];   //34-43
   uint8_t sname[64];    //44-107
   uint8_t file[128];    //108-235
   uint32_t magicCookie; //236-239
   uint8_t options[];    //240
} __end_packed DhcpMessage;


/**
 * @brief DHCP option
 **/

typedef __start_packed struct
{
   uint8_t code;    //0
   uint8_t length;  //1
   uint8_t value[]; //2
} __end_packed DhcpOption;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


//DHCP related functions
void dhcpAddOption(DhcpMessage *message, uint8_t optionCode,
   const void *optionValue, size_t optionLen);

DhcpOption *dhcpGetOption(const DhcpMessage *message,
   size_t length, uint8_t optionCode);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
