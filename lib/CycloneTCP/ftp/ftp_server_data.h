/**
 * @file ftp_server_data.h
 * @brief FTP data connection
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

#ifndef _FTP_SERVER_DATA_H
#define _FTP_SERVER_DATA_H

//Dependencies
#include "core/net.h"
#include "ftp/ftp_server.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//FTP server related functions
void ftpServerRegisterDataChannelEvents(FtpClientConnection *connection,
   SocketEventDesc *eventDesc);

void ftpServerProcessDataChannelEvents(FtpClientConnection *connection,
   uint_t eventFlags);

error_t ftpServerOpenDataChannel(FtpClientConnection *connection);
void ftpServerAcceptDataChannel(FtpClientConnection *connection);
void ftpServerWriteDataChannel(FtpClientConnection *connection);
void ftpServerReadDataChannel(FtpClientConnection *connection);
void ftpServerCloseDataChannel(FtpClientConnection *connection);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
