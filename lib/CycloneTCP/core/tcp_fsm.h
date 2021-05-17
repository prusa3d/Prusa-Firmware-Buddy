/**
 * @file tcp_fsm.h
 * @brief TCP finite state machine
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

#ifndef _TCP_FSM_H
#define _TCP_FSM_H

//Dependencies
#include "core/tcp.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//TCP FSM related functions
void tcpProcessSegment(NetInterface *interface, IpPseudoHeader *pseudoHeader,
   const NetBuffer *buffer, size_t offset, NetRxAncillary *ancillary);

void tcpStateClosed(NetInterface *interface,
   IpPseudoHeader *pseudoHeader, TcpHeader *segment, size_t length);

void tcpStateListen(Socket *socket, NetInterface *interface,
   IpPseudoHeader *pseudoHeader, TcpHeader *segment, size_t length);

void tcpStateSynSent(Socket *socket, TcpHeader *segment, size_t length);

void tcpStateSynReceived(Socket *socket, TcpHeader *segment,
   const NetBuffer *buffer, size_t offset, size_t length);

void tcpStateEstablished(Socket *socket, TcpHeader *segment,
   const NetBuffer *buffer, size_t offset, size_t length);

void tcpStateCloseWait(Socket *socket, TcpHeader *segment, size_t length);

void tcpStateLastAck(Socket *socket, TcpHeader *segment, size_t length);

void tcpStateFinWait1(Socket *socket, TcpHeader *segment,
   const NetBuffer *buffer, size_t offset, size_t length);

void tcpStateFinWait2(Socket *socket, TcpHeader *segment,
   const NetBuffer *buffer, size_t offset, size_t length);

void tcpStateClosing(Socket *socket, TcpHeader *segment, size_t length);

void tcpStateTimeWait(Socket *socket, TcpHeader *segment, size_t length);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
