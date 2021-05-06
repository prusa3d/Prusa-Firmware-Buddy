/**
 * @file ppp_fsm.h
 * @brief PPP finite state machine
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

#ifndef _PPP_FSM_H
#define _PPP_FSM_H

//Dependencies
#include "core/net.h"
#include "ppp/ppp.h"

//C++ guard
#ifdef __cplusplus
extern "C" {
#endif


/**
 * @brief This-Layer-Up callback function
 **/

typedef void (*PppThisLayerUp)(PppContext *context);


/**
 * @brief This-Layer-Down callback function
 **/

typedef void (*PppThisLayerDown)(PppContext *context);


/**
 * @brief This-Layer-Started callback function
 **/

typedef void (*PppThisLayerStarted)(PppContext *context);


/**
 * @brief This-Layer-Finished callback function
 **/

typedef void (*PppThisLayerFinished)(PppContext *context);


/**
 * @brief Initialize-Restart-Count callback function
 **/

typedef void (*PppInitRestartCount)(PppContext *context, uint_t value);


/**
 * @brief Zero-Restart-Count callback function
 **/

typedef void (*PppZeroRestartCount)(PppContext *context);


/**
 * @brief Send-Configure-Request callback function
 **/

typedef error_t (*PppSendConfigureReq)(PppContext *context);


/**
 * @brief Send-Configure-Ack callback function
 **/

typedef error_t (*PppSendConfigureAck)(PppContext *context,
   const PppConfigurePacket *configureReqPacket);


/**
 * @brief Send-Configure-Nak callback function
 **/

typedef error_t (*PppSendConfigureNak)(PppContext *context,
   const PppConfigurePacket *configureReqPacket);


/**
 * @brief Send-Configure-Reject callback function
 **/

typedef error_t (*PppSendConfigureRej)(PppContext *context,
   const PppConfigurePacket *configureReqPacket);


/**
 * @brief Send-Terminate-Request callback function
 **/

typedef error_t (*PppSendTerminateReq)(PppContext *context);


/**
 * @brief Send-Terminate-Ack callback function
 **/

typedef error_t (*PppSendTerminateAck)(PppContext *context,
   const PppTerminatePacket *terminateReqPacket);


/**
 * @brief Send-Code-Reject callback function
 **/

typedef error_t (*PppSendCodeRej)(PppContext *context,
   const PppPacket *packet);


/**
 * @brief Send-Echo-Reply callback function
 **/

typedef error_t (*PppSendEchoRep)(PppContext *context,
   const PppEchoPacket *echoReqPacket);


/**
 *@brief PPP FSM actions
 **/

typedef struct
{
   PppThisLayerUp thisLayerUp;
   PppThisLayerDown thisLayerDown;
   PppThisLayerStarted thisLayerStarted;
   PppThisLayerFinished thisLayerFinished;
   PppInitRestartCount initRestartCount;
   PppZeroRestartCount zeroRestartCount;
   PppSendConfigureReq sendConfigureReq;
   PppSendConfigureAck sendConfigureAck;
   PppSendConfigureNak sendConfigureNak;
   PppSendConfigureRej sendConfigureRej;
   PppSendTerminateReq sendTerminateReq;
   PppSendTerminateAck sendTerminateAck;
   PppSendCodeRej sendCodeRej;
   PppSendEchoRep sendEchoRep;
} PppCallbacks;


//PPP FSM events
void pppUpEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks);

void pppDownEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks);

void pppOpenEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks);

void pppCloseEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks);

void pppTimeoutEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks);

void pppRcvConfigureReqEvent(PppContext *context, PppFsm *fsm, const PppCallbacks *callbacks,
   const PppConfigurePacket *configureReqPacket, PppCode code);

void pppRcvConfigureAckEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks);

void pppRcvConfigureNakEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks);

void pppRcvTerminateReqEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks, const PppTerminatePacket *terminateReqPacket);

void pppRcvTerminateAckEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks);

void pppRcvUnknownCodeEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks, const PppPacket *packet);

void pppRcvCodeRejEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks, bool_t acceptable);

void pppRcvEchoReqEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks, const PppEchoPacket *echoReqPacket);

void pppChangeState(PppFsm *fsm, PppState newState);

//C++ guard
#ifdef __cplusplus
}
#endif

#endif
