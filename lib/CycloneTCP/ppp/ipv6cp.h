/**
 * @file ipv6cp.h
 * @brief IPV6CP (PPP IPv6 Control Protocol)
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

#ifndef _IPV6CP_H
#define _IPV6CP_H

//Dependencies
#include "core/net.h"
#include "ppp/ppp.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief IPV6CP option types
 **/

typedef enum
{
   IPV6CP_OPTION_INTERFACE_ID       = 1, ///<Interface-Identifier
   IPV6CP_OPTION_IPV6_COMP_PROTOCOL = 2  ///<IPv6-Compression-Protocol
} Ipv6cpOptionType;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(push, 1)
#endif


/**
 * @brief Interface-Identifier option
 **/

typedef __start_packed struct
{
   uint8_t type;       //0
   uint8_t length;     //1
   Eui64 interfaceId;  //2-9
} __end_packed Ipv6cpInterfaceIdOption;


/**
 * @brief IPv6-Compression-Protocol option
 **/

typedef __start_packed struct
{
   uint8_t type;      //0
   uint8_t length;    //1
   uint16_t protocol; //2-3
   uint8_t data[];    //4
} __end_packed Ipv6cpIpCompProtocolOption;


//CodeWarrior or Win32 compiler?
#if defined(__CWCC__) || defined(_WIN32)
   #pragma pack(pop)
#endif


//IPV6CP FSM events
error_t ipv6cpOpen(PppContext *context);
error_t ipv6cpClose(PppContext *context);

void ipv6cpTick(PppContext *context);

void ipv6cpProcessPacket(PppContext *context, const PppPacket *packet, size_t length);

error_t ipv6cpProcessConfigureReq(PppContext *context,
   const PppConfigurePacket *configureReqPacket);

error_t ipv6cpProcessConfigureAck(PppContext *context,
   const PppConfigurePacket *configureAckPacket);

error_t ipv6cpProcessConfigureNak(PppContext *context,
   const PppConfigurePacket *configureNakPacket);

error_t ipv6cpProcessConfigureReject(PppContext *context,
   const PppConfigurePacket *configureRejPacket);

error_t ipv6cpProcessTerminateReq(PppContext *context,
   const PppTerminatePacket *terminateReqPacket);

error_t ipv6cpProcessTerminateAck(PppContext *context,
   const PppTerminatePacket *terminateAckPacket);

error_t ipv6cpProcessCodeRej(PppContext *context,
   const PppCodeRejPacket *codeRejPacket);

error_t ipv6cpProcessUnknownCode(PppContext *context,
   const PppPacket *packet);

//IPV6CP FSM callback functions
void ipv6cpThisLayerUp(PppContext *context);
void ipv6cpThisLayerDown(PppContext *context);
void ipv6cpThisLayerStarted(PppContext *context);
void ipv6cpThisLayerFinished(PppContext *context);

void ipv6cpInitRestartCount(PppContext *context, uint_t value);
void ipv6cpZeroRestartCount(PppContext *context);

error_t ipv6cpSendConfigureReq(PppContext *context);

error_t ipv6cpSendConfigureAck(PppContext *context,
   const PppConfigurePacket *configureReqPacket);

error_t ipv6cpSendConfigureNak(PppContext *context,
   const PppConfigurePacket *configureReqPacket);

error_t ipv6cpSendConfigureRej(PppContext *context,
   const PppConfigurePacket *configureReqPacket);

error_t ipv6cpSendTerminateReq(PppContext *context);

error_t ipv6cpSendTerminateAck(PppContext *context,
   const PppTerminatePacket *terminateReqPacket);

error_t ipv6cpSendCodeRej(PppContext *context, const PppPacket *packet);

//IPV6CP options checking
error_t ipv6cpParseOption(PppContext *context, PppOption *option,
   size_t inPacketLen, PppConfigurePacket *outPacket);

error_t ipv6cpParseInterfaceIdOption(PppContext *context,
   Ipv6cpInterfaceIdOption *option, PppConfigurePacket *outPacket);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
