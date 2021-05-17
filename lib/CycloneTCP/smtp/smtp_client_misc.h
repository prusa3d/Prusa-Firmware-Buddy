/**
 * @file smtp_client_misc.h
 * @brief Helper functions for SMTP client
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

#ifndef _SMTP_CLIENT_MISC_H
#define _SMTP_CLIENT_MISC_H

//Dependencies
#include "core/net.h"
#include "smtp/smtp_client.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif

//SMTP client related functions
void smtpClientChangeState(SmtpClientContext *context,
   SmtpClientState newState);

error_t smtpClientSendCommand(SmtpClientContext *context,
   SmtpClientReplyCallback callback);

error_t smtpClientFormatCommand(SmtpClientContext *context,
   const char_t *command, const char_t *argument);

error_t smtpClientParseEhloReply(SmtpClientContext *context,
   char_t *replyLine);

error_t smtpClientFormatMailHeader(SmtpClientContext *context,
   const SmtpMailAddr *from, const SmtpMailAddr *recipients,
   uint_t numRecipients, const char_t *subject);

error_t smtpClientFormatMultipartHeader(SmtpClientContext *context,
   const char_t *filename, const char_t *contentType,
   const char_t *contentTransferEncoding, bool_t last);

error_t smtpClientCheckTimeout(SmtpClientContext *context);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
