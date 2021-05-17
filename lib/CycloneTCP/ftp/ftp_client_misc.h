/**
 * @file ftp_client_misc.h
 * @brief Helper functions for FTP client
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

#ifndef _FTP_CLIENT_MISC_H
#define _FTP_CLIENT_MISC_H

//Dependencies
#include "core/net.h"
#include "ftp_client.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//FTP client related functions
void ftpClientChangeState(FtpClientContext *context, FtpClientState newState);

error_t ftpClientSendCommand(FtpClientContext *context);

error_t ftpClientFormatCommand(FtpClientContext *context,
   const char_t *command, const char_t *argument);

error_t ftpClientFormatPortCommand(FtpClientContext *context,
   const IpAddr *ipAddr, uint16_t port);

error_t ftpClientFormatPasvCommand(FtpClientContext *context);
error_t ftpClientParsePasvReply(FtpClientContext *context, uint16_t *port);

error_t ftpClientParsePwdReply(FtpClientContext *context, char_t *path,
   size_t maxLen);

error_t ftpClientParseDirEntry(char_t *line, FtpDirEntry *dirEntry);

error_t ftpClientInitDataTransfer(FtpClientContext *context, bool_t direction);
error_t ftpClientTerminateDataTransfer(FtpClientContext *context);

error_t ftpClientCheckTimeout(FtpClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
