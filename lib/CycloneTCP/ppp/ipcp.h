/**
 * @file ipcp.h
 * @brief IPCP (PPP Internet Protocol Control Protocol)
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

#ifndef _IPCP_H
#define _IPCP_H

//Dependencies
#include "core/net.h"
#include "ppp/ppp.h"

//Subnet mask
#define IPCP_DEFAULT_SUBNET_MASK IPV4_ADDR(255, 255, 255, 255)

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief IPCP option types
 **/

typedef enum
{
   IPCP_OPTION_IP_ADDRESSES     = 1,   ///<IP-Addresses
   IPCP_OPTION_IP_COMP_PROTOCOL = 2,   ///<IP-Compression-Protocol
   IPCP_OPTION_IP_ADDRESS       = 3,   ///<IP-Address
   IPCP_OPTION_PRIMARY_DNS      = 129, ///<Primary-DNS-Server-Address
   IPCP_OPTION_PRIMARY_NBNS     = 130, ///<Primary-NBNS-Server-Address
   IPCP_OPTION_SECONDARY_DNS    = 131, ///<Secondary-DNS-Server-Address
   IPCP_OPTION_SECONDARY_NBNS   = 132  ///<Secondary-NBNS-Server-Address
} IpcpOptionType;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief IP-Addresses option
 **/

typedef __start_packed struct
{
   uint8_t type;        //0
   uint8_t length;      //1
   Ipv4Addr srcIpAddr;  //2-5
   Ipv4Addr destIpAddr; //6-9
} __end_packed IpcpIpAddressesOption;


/**
 * @brief IP-Compression-Protocol option
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t length;    //1
   uint16_t protocol; //2-3
   uint8_t data[];    //4
} __end_packed IpcpIpCompProtocolOption;


/**
 * @brief IP-Address option
 **/

typedef __start_packed struct
{
   uint8_t type;    //0
   uint8_t length;  //1
   Ipv4Addr ipAddr; //2-5
} __end_packed IpcpIpAddressOption;


/**
 * @brief Primary-DNS-Server-Address option
 **/

typedef __start_packed struct
{
   uint8_t type;    //0
   uint8_t length;  //1
   Ipv4Addr ipAddr; //2-5
} __end_packed IpcpPrimaryDnsOption;


/**
 * @brief Primary-NBNS-Server-Address option
 **/

typedef __start_packed struct
{
   uint8_t type;    //0
   uint8_t length;  //1
   Ipv4Addr ipAddr; //2-5
} __end_packed IpcpPrimaryNbnsOption;


/**
 * @brief Secondary-DNS-Server-Address option
 **/

typedef __start_packed struct
{
   uint8_t type;    //0
   uint8_t length;  //1
   Ipv4Addr ipAddr; //2-5
} __end_packed IpcpSecondaryDnsOption;


/**
 * @brief Secondary-NBNS-Server-Address option
 **/

typedef __start_packed struct
{
   uint8_t type;    //0
   uint8_t length;  //1
   Ipv4Addr ipAddr; //2-5
} __end_packed IpcpSecondaryNbnsOption;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


//IPCP FSM events
error_t ipcpOpen(PppContext *context);
error_t ipcpClose(PppContext *context);

void ipcpTick(PppContext *context);

void ipcpProcessPacket(PppContext *context, const PppPacket *packet, size_t length);

error_t ipcpProcessConfigureReq(PppContext *context,
   const PppConfigurePacket *configureReqPacket);

error_t ipcpProcessConfigureAck(PppContext *context,
   const PppConfigurePacket *configureAckPacket);

error_t ipcpProcessConfigureNak(PppContext *context,
   const PppConfigurePacket *configureNakPacket);

error_t ipcpProcessConfigureReject(PppContext *context,
   const PppConfigurePacket *configureRejPacket);

error_t ipcpProcessTerminateReq(PppContext *context,
   const PppTerminatePacket *terminateReqPacket);

error_t ipcpProcessTerminateAck(PppContext *context,
   const PppTerminatePacket *terminateAckPacket);

error_t ipcpProcessCodeRej(PppContext *context,
   const PppCodeRejPacket *codeRejPacket);

error_t ipcpProcessUnknownCode(PppContext *context,
   const PppPacket *packet);

//IPCP FSM callback functions
void ipcpThisLayerUp(PppContext *context);
void ipcpThisLayerDown(PppContext *context);
void ipcpThisLayerStarted(PppContext *context);
void ipcpThisLayerFinished(PppContext *context);

void ipcpInitRestartCount(PppContext *context, uint_t value);
void ipcpZeroRestartCount(PppContext *context);

error_t ipcpSendConfigureReq(PppContext *context);

error_t ipcpSendConfigureAck(PppContext *context,
   const PppConfigurePacket *configureReqPacket);

error_t ipcpSendConfigureNak(PppContext *context,
   const PppConfigurePacket *configureReqPacket);

error_t ipcpSendConfigureRej(PppContext *context,
   const PppConfigurePacket *configureReqPacket);

error_t ipcpSendTerminateReq(PppContext *context);

error_t ipcpSendTerminateAck(PppContext *context,
   const PppTerminatePacket *terminateReqPacket);

error_t ipcpSendCodeRej(PppContext *context, const PppPacket *packet);

//IPCP options checking
error_t ipcpParseOption(PppContext *context, PppOption *option,
   size_t inPacketLen, PppConfigurePacket *outPacket);

error_t ipcpParseIpAddressOption(PppContext *context,
   IpcpIpAddressOption *option, PppConfigurePacket *outPacket);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
