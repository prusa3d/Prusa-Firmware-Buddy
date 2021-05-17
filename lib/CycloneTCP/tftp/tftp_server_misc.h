/**
 * @file tftp_server_misc.h
 * @brief Helper functions for TFTP server
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

#ifndef _TFTP_SERVER_MISC_H
#define _TFTP_SERVER_MISC_H

//Dependencies
#include "core/net.h"
#include "tftp/tftp_server.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//TFTP server related functions
void tftpServerTick(TftpServerContext *context);

TftpClientConnection *tftpServerOpenConnection(TftpServerContext *context,
   const IpAddr *clientIpAddr, uint16_t clientPort);

void tftpServerCloseConnection(TftpClientConnection *connection);

void tftpServerAcceptRequest(TftpServerContext *context);

void tftpServerProcessPacket(TftpServerContext *context,
   TftpClientConnection *connection);

void tftpServerProcessRrqPacket(TftpServerContext *context, const IpAddr *clientIpAddr,
   uint16_t clientPort, const TftpRrqPacket *rrqPacket, size_t length);

void tftpServerProcessWrqPacket(TftpServerContext *context, const IpAddr *clientIpAddr,
   uint16_t clientPort, const TftpWrqPacket *wrqPacket, size_t length);

void tftpServerProcessDataPacket(TftpClientConnection *connection,
   const TftpDataPacket *dataPacket, size_t length);

void tftpServerProcessAckPacket(TftpClientConnection *connection,
   const TftpAckPacket *ackPacket, size_t length);

void tftpServerProcessErrorPacket(TftpClientConnection *connection,
   const TftpErrorPacket *errorPacket, size_t length);

error_t tftpServerSendDataPacket(TftpClientConnection *connection);
error_t tftpServerSendAckPacket(TftpClientConnection *connection);

error_t tftpServerSendErrorPacket(TftpClientConnection *connection,
   uint16_t errorCode, const char_t *errorMsg);

error_t tftpServerRetransmitPacket(TftpClientConnection *connection);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
