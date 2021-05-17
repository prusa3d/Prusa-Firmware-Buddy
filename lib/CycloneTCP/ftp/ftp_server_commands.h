/**
 * @file ftp_server_commands.h
 * @brief FTP server (command processing)
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

#ifndef _FTP_SERVER_COMMANDS_H
#define _FTP_SERVER_COMMANDS_H

//Dependencies
#include "core/net.h"
#include "ftp/ftp_server.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//FTP server related functions
void ftpServerProcessCommand(FtpClientConnection *connection);

void ftpServerProcessNoop(FtpClientConnection *connection, char_t *param);
void ftpServerProcessSyst(FtpClientConnection *connection, char_t *param);
void ftpServerProcessFeat(FtpClientConnection *connection, char_t *param);
void ftpServerProcessAuth(FtpClientConnection *connection, char_t *param);
void ftpServerProcessPbsz(FtpClientConnection *connection, char_t *param);
void ftpServerProcessProt(FtpClientConnection *connection, char_t *param);
void ftpServerProcessType(FtpClientConnection *connection, char_t *param);
void ftpServerProcessStru(FtpClientConnection *connection, char_t *param);
void ftpServerProcessMode(FtpClientConnection *connection, char_t *param);
void ftpServerProcessUser(FtpClientConnection *connection, char_t *param);
void ftpServerProcessPass(FtpClientConnection *connection, char_t *param);
void ftpServerProcessRein(FtpClientConnection *connection, char_t *param);
void ftpServerProcessQuit(FtpClientConnection *connection, char_t *param);
void ftpServerProcessPort(FtpClientConnection *connection, char_t *param);
void ftpServerProcessEprt(FtpClientConnection *connection, char_t *param);
void ftpServerProcessPasv(FtpClientConnection *connection, char_t *param);
void ftpServerProcessEpsv(FtpClientConnection *connection, char_t *param);
void ftpServerProcessAbor(FtpClientConnection *connection, char_t *param);
void ftpServerProcessPwd(FtpClientConnection *connection, char_t *param);
void ftpServerProcessCwd(FtpClientConnection *connection, char_t *param);
void ftpServerProcessCdup(FtpClientConnection *connection, char_t *param);
void ftpServerProcessList(FtpClientConnection *connection, char_t *param);
void ftpServerProcessNlst(FtpClientConnection *connection, char_t *param);
void ftpServerProcessMkd(FtpClientConnection *connection, char_t *param);
void ftpServerProcessRmd(FtpClientConnection *connection, char_t *param);
void ftpServerProcessSize(FtpClientConnection *connection, char_t *param);
void ftpServerProcessRetr(FtpClientConnection *connection, char_t *param);
void ftpServerProcessStor(FtpClientConnection *connection, char_t *param);
void ftpServerProcessAppe(FtpClientConnection *connection, char_t *param);
void ftpServerProcessRnfr(FtpClientConnection *connection, char_t *param);
void ftpServerProcessRnto(FtpClientConnection *connection, char_t *param);
void ftpServerProcessDele(FtpClientConnection *connection, char_t *param);

void ftpServerProcessUnknownCmd(FtpClientConnection *connection, char_t *param);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
