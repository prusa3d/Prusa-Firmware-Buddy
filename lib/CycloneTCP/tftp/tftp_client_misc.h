/**
 * @file tftp_client_misc.h
 * @brief Helper functions for TFTP client
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

#ifndef _TFTP_CLIENT_MISC_H
#define _TFTP_CLIENT_MISC_H

//Dependencies
#include "core/net.h"
#include "tftp/tftp_client.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//TFTP client related functions
error_t tftpClientOpenConnection(TftpClientContext *context);
void tftpClientCloseConnection(TftpClientContext *context);

error_t tftpClientProcessEvents(TftpClientContext *context);

void tftpClientProcessPacket(TftpClientContext *context,
   const IpAddr *srcIpAddr, uint16_t srcPort);

void tftpClientProcessDataPacket(TftpClientContext *context,
   uint16_t srcPort, const TftpDataPacket *dataPacket, size_t length);

void tftpClientProcessAckPacket(TftpClientContext *context,
   uint16_t srcPort, const TftpAckPacket *ackPacket, size_t length);

void tftpClientProcessErrorPacket(TftpClientContext *context,
   uint16_t srcPort, const TftpErrorPacket *errorPacket, size_t length);

error_t tftpClientSendRrqPacket(TftpClientContext *context,
   const char_t *filename, const char_t *mode);

error_t tftpClientSendWrqPacket(TftpClientContext *context,
   const char_t *filename, const char_t *mode);

error_t tftpClientSendDataPacket(TftpClientContext *context);
error_t tftpClientSendAckPacket(TftpClientContext *context);

error_t tftpClientSendErrorPacket(TftpClientContext *context,
   uint16_t errorCode, const char_t *errorMsg);

error_t tftpClientRetransmitPacket(TftpClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
