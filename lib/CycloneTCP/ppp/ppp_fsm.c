/**
 * @file ppp_fsm.c
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

//Switch to the appropriate trace level
#define TRACE_LEVEL PPP_TRACE_LEVEL

//Dependencies
#include "core/net.h"
#include "ppp/ppp_fsm.h"
#include "debug.h"

//Check TCP/IP stack configuration
#if (PPP_SUPPORT == ENABLED)


/**
 * @brief Process Up event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 **/

void pppUpEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks)
{
   //Check current state
   switch(fsm->state)
   {
   case PPP_STATE_0_INITIAL:
      //Switch to the Closed state
      pppChangeState(fsm, PPP_STATE_2_CLOSED);
      break;
   case PPP_STATE_1_STARTING:
      //Initialize restart counter
      callbacks->initRestartCount(context, PPP_MAX_CONFIGURE);
      //Send Configure-Request packet
      callbacks->sendConfigureReq(context);
      //Switch to the Req-Sent state
      pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
      break;
   default:
      //This event cannot occur in a properly implemented automaton.
      //No transition is taken, and the implementation should not
      //reset or freeze
      break;
   }
}


/**
 * @brief Process Down event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 **/

void pppDownEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks)
{
   //Check current state
   switch(fsm->state)
   {
   case PPP_STATE_2_CLOSED:
      //Switch to the Initial state
      pppChangeState(fsm, PPP_STATE_0_INITIAL);
      break;
   case PPP_STATE_3_STOPPED:
      //Switch to the Starting state
      pppChangeState(fsm, PPP_STATE_1_STARTING);
      //Indicate to the lower layers that the automaton is entering the
      //Starting state. The lower layer is needed for the link
      callbacks->thisLayerStarted(context);
      break;
   case PPP_STATE_4_CLOSING:
      //Switch to the Initial state
      pppChangeState(fsm, PPP_STATE_0_INITIAL);
      break;
   case PPP_STATE_5_STOPPING:
   case PPP_STATE_6_REQ_SENT:
   case PPP_STATE_7_ACK_RCVD:
   case PPP_STATE_8_ACK_SENT:
      //Switch to the Starting state
      pppChangeState(fsm, PPP_STATE_1_STARTING);
      break;
   case PPP_STATE_9_OPENED:
      //Switch to the Starting state
      pppChangeState(fsm, PPP_STATE_1_STARTING);
      //Indicate to the upper layers that the automaton is leaving the Opened
      //state. The link is no longer available for network traffic
      callbacks->thisLayerDown(context);
      break;
   default:
      //This event cannot occur in a properly implemented automaton.
      //No transition is taken, and the implementation should not
      //reset or freeze
      break;
   }
}


/**
 * @brief Process Open event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 **/

void pppOpenEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks)
{
   //Check current state
   switch(fsm->state)
   {
   case PPP_STATE_0_INITIAL:
      //Switch to the Starting state
      pppChangeState(fsm, PPP_STATE_1_STARTING);
      //Indicate to the lower layers that the automaton is entering the
      //Starting state. The lower layer is needed for the link
      callbacks->thisLayerStarted(context);
      break;
   case PPP_STATE_1_STARTING:
      //Stay in current state
      break;
   case PPP_STATE_2_CLOSED:
      //Initialize restart counter
      callbacks->initRestartCount(context, PPP_MAX_CONFIGURE);
      //Send Configure-Request packet
      callbacks->sendConfigureReq(context);
      //Switch to the Req-Sent state
      pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
      break;
   case PPP_STATE_3_STOPPED:
      //Stay in current state
      break;
   case PPP_STATE_4_CLOSING:
      //Switch to the Stopping state
      pppChangeState(fsm, PPP_STATE_5_STOPPING);
      break;
   case PPP_STATE_5_STOPPING:
   case PPP_STATE_6_REQ_SENT:
   case PPP_STATE_7_ACK_RCVD:
   case PPP_STATE_8_ACK_SENT:
   case PPP_STATE_9_OPENED:
      //Stay in current state
      break;
   default:
      //This event cannot occur in a properly implemented automaton.
      //No transition is taken, and the implementation should not
      //reset or freeze
      break;
   }
}


/**
 * @brief Process Close event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 **/

void pppCloseEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks)
{
   //Check current state
   switch(fsm->state)
   {
   case PPP_STATE_0_INITIAL:
      //Stay in current state
      break;
   case PPP_STATE_1_STARTING:
      //Switch to the Initial state
      pppChangeState(fsm, PPP_STATE_0_INITIAL);
      //Indicate to the lower layers that the automaton is entering the
      //Initial, Closed or Stopped states. The lower layer is no longer
      //needed for the link
      callbacks->thisLayerFinished(context);
      break;
   case PPP_STATE_2_CLOSED:
      //Stay in current state
      break;
   case PPP_STATE_3_STOPPED:
      //Switch to the Closed state
      pppChangeState(fsm, PPP_STATE_2_CLOSED);
      break;
   case PPP_STATE_4_CLOSING:
      //Stay in current state
      break;
   case PPP_STATE_5_STOPPING:
      //Switch to the Closing state
      pppChangeState(fsm, PPP_STATE_4_CLOSING);
      break;
   case PPP_STATE_6_REQ_SENT:
   case PPP_STATE_7_ACK_RCVD:
   case PPP_STATE_8_ACK_SENT:
      //Initialize restart counter
      callbacks->initRestartCount(context, PPP_MAX_TERMINATE);
      //Send Terminate-Request packet
      callbacks->sendTerminateReq(context);
      //Switch to the Closing state
      pppChangeState(fsm, PPP_STATE_4_CLOSING);
      break;
   case PPP_STATE_9_OPENED:
      //Initialize restart counter
      callbacks->initRestartCount(context, PPP_MAX_TERMINATE);
      //Send Terminate-Request packet
      callbacks->sendTerminateReq(context);
      //Switch to the Closing state
      pppChangeState(fsm, PPP_STATE_4_CLOSING);
      //Indicate to the upper layers that the automaton is leaving the Opened
      //state. The link is no longer available for network traffic
      callbacks->thisLayerDown(context);
      break;
   default:
      //This event cannot occur in a properly implemented automaton.
      //No transition is taken, and the implementation should not
      //reset or freeze
      break;
   }
}


/**
 * @brief Process Timeout event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 **/

void pppTimeoutEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks)
{
   //The restart counter is greater than zero (TO+ event)
   if(fsm->restartCounter > 0)
   {
      //Check current state
      switch(fsm->state)
      {
      case PPP_STATE_4_CLOSING:
      case PPP_STATE_5_STOPPING:
         //Send Terminate-Request packet
         callbacks->sendTerminateReq(context);
         //Stay in current state
         break;
      case PPP_STATE_6_REQ_SENT:
      case PPP_STATE_7_ACK_RCVD:
         //Send Configuration-Request packet
         callbacks->sendConfigureReq(context);
         //Switch to the Req-Sent state
         pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
         break;
      case PPP_STATE_8_ACK_SENT:
         //Send Configuration-Request packet
         callbacks->sendConfigureReq(context);
         //Stay in current state
         break;
      default:
         //This event cannot occur in a properly implemented automaton.
         //No transition is taken, and the implementation should not
         //reset or freeze
         break;
      }
   }
   //The restart counter is not greater than zero (TO- event)
   else
   {
      //Check current state
      switch(fsm->state)
      {
      case PPP_STATE_4_CLOSING:
         //Switch to the Closed state
         pppChangeState(fsm, PPP_STATE_2_CLOSED);
         //Indicate to the lower layers that the automaton is entering the
         //Initial, Closed or Stopped states. The lower layer is no longer
         //needed for the link
         callbacks->thisLayerFinished(context);
         break;
      case PPP_STATE_5_STOPPING:
      case PPP_STATE_6_REQ_SENT:
      case PPP_STATE_7_ACK_RCVD:
      case PPP_STATE_8_ACK_SENT:
         //Switch to the Stopped state
         pppChangeState(fsm, PPP_STATE_3_STOPPED);
         //Indicate to the lower layers that the automaton is entering the
         //Initial, Closed or Stopped states. The lower layer is no longer
         //needed for the link
         callbacks->thisLayerFinished(context);
         break;
      default:
         //This event cannot occur in a properly implemented automaton.
         //No transition is taken, and the implementation should not
         //reset or freeze
         break;
      }
   }
}


/**
 * @brief Process Receive-Configure-Request event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 * @param[in] configureReqPacket Configure-Request packet received from the peer
 * @param[in] code Tells whether the configuration options are acceptable
 **/

void pppRcvConfigureReqEvent(PppContext *context, PppFsm *fsm, const PppCallbacks *callbacks,
   const PppConfigurePacket *configureReqPacket, PppCode code)
{
   //Check whether the configuration options are acceptable
   if(code == PPP_CODE_CONFIGURE_ACK)
   {
      //If every configuration option received in the Configure-Request is
      //recognizable and all values are acceptable, then the implementation
      //must transmit a Configure-Ack
      switch(fsm->state)
      {
      case PPP_STATE_2_CLOSED:
         //Send Terminate-Ack packet
         callbacks->sendTerminateAck(context, NULL);
         //Stay in current state
         break;
      case PPP_STATE_3_STOPPED:
         //Initialize restart counter
         callbacks->initRestartCount(context, PPP_MAX_CONFIGURE);
         //Send Configure-Request packet
         callbacks->sendConfigureReq(context);
         //Send Configure-Ack packet
         callbacks->sendConfigureAck(context, configureReqPacket);
         //Switch to the Ack-Sent state
         pppChangeState(fsm, PPP_STATE_8_ACK_SENT);
         break;
      case PPP_STATE_4_CLOSING:
      case PPP_STATE_5_STOPPING:
         //Stay in current state
         break;
      case PPP_STATE_6_REQ_SENT:
         //Send Configure-Ack packet
         callbacks->sendConfigureAck(context, configureReqPacket);
         //Switch to the Ack-Sent state
         pppChangeState(fsm, PPP_STATE_8_ACK_SENT);
         break;
      case PPP_STATE_7_ACK_RCVD:
         //Send Configure-Ack packet
         callbacks->sendConfigureAck(context, configureReqPacket);
         //Switch to the Opened state
         pppChangeState(fsm, PPP_STATE_9_OPENED);
         //Indicate to the upper layers that the automaton is entering the
         //Opened state. The link is available for network traffic
         callbacks->thisLayerUp(context);
         break;
      case PPP_STATE_8_ACK_SENT:
         //Send Configure-Ack packet
         callbacks->sendConfigureAck(context, configureReqPacket);
         //Stay in current state
         break;
      case PPP_STATE_9_OPENED:
         //Send Configure-Request packet
         callbacks->sendConfigureReq(context);
         //Send Configure-Ack packet
         callbacks->sendConfigureAck(context, configureReqPacket);
         //Switch to the Ack-Sent state
         pppChangeState(fsm, PPP_STATE_8_ACK_SENT);
         //Indicate to the upper layers that the automaton is leaving the Opened
         //state. The link is no longer available for network traffic
         callbacks->thisLayerDown(context);
         break;
      default:
         //This event cannot occur in a properly implemented automaton.
         //No transition is taken, and the implementation should not
         //reset or freeze
         break;
      }
   }
   else if(code == PPP_CODE_CONFIGURE_NAK)
   {
      //If all configuration options are recognizable, but some values are not
      //acceptable, then the implementation must transmit a Configure-Nak
      switch(fsm->state)
      {
      case PPP_STATE_2_CLOSED:
         //Send Terminate-Ack packet
         callbacks->sendTerminateAck(context, NULL);
         //Stay in current state
         break;
      case PPP_STATE_3_STOPPED:
         //Initialize restart counter
         callbacks->initRestartCount(context, PPP_MAX_CONFIGURE);
         //Send Configure-Request packet
         callbacks->sendConfigureReq(context);
         //Send Configure-Nak packet
         callbacks->sendConfigureNak(context, configureReqPacket);
         //Switch to the Req-Sent state
         pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
         break;
      case PPP_STATE_4_CLOSING:
      case PPP_STATE_5_STOPPING:
         //Stay in current state
         break;
      case PPP_STATE_6_REQ_SENT:
      case PPP_STATE_7_ACK_RCVD:
         //Send Configure-Nak packet
         callbacks->sendConfigureNak(context, configureReqPacket);
         //Stay in current state
         break;
      case PPP_STATE_8_ACK_SENT:
         //Send Configure-Nak packet
         callbacks->sendConfigureNak(context, configureReqPacket);
         //Switch to the Req-Sent state
         pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
         break;
      case PPP_STATE_9_OPENED:
         //Send Configure-Request packet
         callbacks->sendConfigureReq(context);
         //Send Configure-Nak packet
         callbacks->sendConfigureNak(context, configureReqPacket);
         //Switch to the Req-Sent state
         pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
         //Indicate to the upper layers that the automaton is leaving the Opened
         //state. The link is no longer available for network traffic
         callbacks->thisLayerDown(context);
         break;
      default:
         //This event cannot occur in a properly implemented automaton.
         //No transition is taken, and the implementation should not
         //reset or freeze
         break;
      }
   }
   else if(code == PPP_CODE_CONFIGURE_REJ)
   {
      //If some configuration options received in the Configure-Request are not
      //recognizable or not acceptable for negotiation, then the implementation
      //must transmit a Configure-Reject
      switch(fsm->state)
      {
      case PPP_STATE_2_CLOSED:
         //Send Terminate-Ack packet
         callbacks->sendTerminateAck(context, NULL);
         //Stay in current state
         break;
      case PPP_STATE_3_STOPPED:
         //Initialize restart counter
         callbacks->initRestartCount(context, PPP_MAX_CONFIGURE);
         //Send Configure-Request packet
         callbacks->sendConfigureReq(context);
         //Send Configure-Reject packet
         callbacks->sendConfigureRej(context, configureReqPacket);
         //Switch to the Req-Sent state
         pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
         break;
      case PPP_STATE_4_CLOSING:
      case PPP_STATE_5_STOPPING:
         //Stay in current state
         break;
      case PPP_STATE_6_REQ_SENT:
      case PPP_STATE_7_ACK_RCVD:
         //Send Configure-Reject packet
         callbacks->sendConfigureRej(context, configureReqPacket);
         //Stay in current state
         break;
      case PPP_STATE_8_ACK_SENT:
         //Send Configure-Reject packet
         callbacks->sendConfigureRej(context, configureReqPacket);
         //Switch to the Req-Sent state
         pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
         break;
      case PPP_STATE_9_OPENED:
         //Send Configure-Request packet
         callbacks->sendConfigureReq(context);
         //Send Configure-Reject packet
         callbacks->sendConfigureRej(context, configureReqPacket);
         //Switch to the Req-Sent state
         pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
         //Indicate to the upper layers that the automaton is leaving the Opened
         //state. The link is no longer available for network traffic
         callbacks->thisLayerDown(context);
         break;
      default:
         //This event cannot occur in a properly implemented automaton.
         //No transition is taken, and the implementation should not
         //reset or freeze
         break;
      }
   }
}


/**
 * @brief Process Receive-Configure-Ack event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 **/

void pppRcvConfigureAckEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks)
{
   //Check current state
   switch(fsm->state)
   {
   case PPP_STATE_2_CLOSED:
   case PPP_STATE_3_STOPPED:
      //Send Terminate-Ack packet
      callbacks->sendTerminateAck(context, NULL);
      //Stay in current state
      break;
   case PPP_STATE_4_CLOSING:
   case PPP_STATE_5_STOPPING:
      //Stay in current state
      break;
   case PPP_STATE_6_REQ_SENT:
      //Initialize restart counter
      callbacks->initRestartCount(context, PPP_MAX_CONFIGURE);
      //Switch to the Ack-Rcvd state
      fsm->state = PPP_STATE_7_ACK_RCVD;
      break;
   case PPP_STATE_7_ACK_RCVD:
      //Send Configure-Request packet
      callbacks->sendConfigureReq(context);
      //Switch to the Req-Sent state
      pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
      break;
   case PPP_STATE_8_ACK_SENT:
      //Initialize restart counter
      callbacks->initRestartCount(context, PPP_MAX_CONFIGURE);
      //Switch to the Opened state
      pppChangeState(fsm, PPP_STATE_9_OPENED);
      //Indicate to the upper layers that the automaton is entering the
      //Opened state. The link is available for network traffic
      callbacks->thisLayerUp(context);
      break;
   case PPP_STATE_9_OPENED:
      //Send Configure-Request packet
      callbacks->sendConfigureReq(context);
      //Switch to the Req-Sent state
      pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
      //Indicate to the upper layers that the automaton is leaving the Opened
      //state. The link is no longer available for network traffic
      callbacks->thisLayerDown(context);
      break;
   default:
      //This event cannot occur in a properly implemented automaton.
      //No transition is taken, and the implementation should not
      //reset or freeze
      break;
   }
}


/**
 * @brief Process Receive-Configure-Nak event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 **/

void pppRcvConfigureNakEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks)
{
   //Check current state
   switch(fsm->state)
   {
   case PPP_STATE_2_CLOSED:
   case PPP_STATE_3_STOPPED:
      //Send Terminate-Ack packet
      callbacks->sendTerminateAck(context, NULL);
      //Stay in current state
      break;
   case PPP_STATE_4_CLOSING:
   case PPP_STATE_5_STOPPING:
      //Stay in current state
      break;
   case PPP_STATE_6_REQ_SENT:
      //Initialize restart counter
      callbacks->initRestartCount(context, PPP_MAX_CONFIGURE);
      //Send Configure-Request packet
      callbacks->sendConfigureReq(context);
      //Stay in current state
      break;
   case PPP_STATE_7_ACK_RCVD:
      //Send Configure-Request packet
      callbacks->sendConfigureReq(context);
      //Switch to the Req-Sent state
      pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
      break;
   case PPP_STATE_8_ACK_SENT:
      //Initialize restart counter
      callbacks->initRestartCount(context, PPP_MAX_CONFIGURE);
      //Send Configure-Request packet
      callbacks->sendConfigureReq(context);
      //Stay in current state
      break;
   case PPP_STATE_9_OPENED:
      //Send Configure-Request packet
      callbacks->sendConfigureReq(context);
      //Switch to the Req-Sent state
      pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
      //Indicate to the upper layers that the automaton is leaving the Opened
      //state. The link is no longer available for network traffic
      callbacks->thisLayerDown(context);
      break;
   default:
      //This event cannot occur in a properly implemented automaton.
      //No transition is taken, and the implementation should not
      //reset or freeze
      break;
   }
}


/**
 * @brief Process Receive-Terminate-Req event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 * @param[in] terminateReqPacket Terminate-Request packet received from the peer
 **/

void pppRcvTerminateReqEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks, const PppTerminatePacket *terminateReqPacket)
{
   //Check current state
   switch(fsm->state)
   {
   case PPP_STATE_2_CLOSED:
   case PPP_STATE_3_STOPPED:
   case PPP_STATE_4_CLOSING:
   case PPP_STATE_5_STOPPING:
      //Send Terminate-Ack packet
      callbacks->sendTerminateAck(context, terminateReqPacket);
      //Stay in current state
      break;
   case PPP_STATE_6_REQ_SENT:
   case PPP_STATE_7_ACK_RCVD:
   case PPP_STATE_8_ACK_SENT:
      //Send Terminate-Ack packet
      callbacks->sendTerminateAck(context, terminateReqPacket);
      //Switch to the Req-Sent state
      pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
      break;
   case PPP_STATE_9_OPENED:
      //Zero restart counter
      callbacks->zeroRestartCount(context);
      //Send Terminate-Ack packet
      callbacks->sendTerminateAck(context, terminateReqPacket);
      //Switch to the Stopping state
      pppChangeState(fsm, PPP_STATE_5_STOPPING);
      //Indicate to the upper layers that the automaton is leaving the Opened
      //state. The link is no longer available for network traffic
      callbacks->thisLayerDown(context);
      break;
   default:
      //This event cannot occur in a properly implemented automaton.
      //No transition is taken, and the implementation should not
      //reset or freeze
      break;
   }
}


/**
 * @brief Process Receive-Terminate-Ack event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 **/

void pppRcvTerminateAckEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks)
{
   //Check current state
   switch(fsm->state)
   {
   case PPP_STATE_2_CLOSED:
   case PPP_STATE_3_STOPPED:
      //Stay in current state
      break;
   case PPP_STATE_4_CLOSING:
      //Switch to the Closed state
      pppChangeState(fsm, PPP_STATE_2_CLOSED);
      //Indicate to the lower layers that the automaton is entering the
      //Initial, Closed or Stopped states. The lower layer is no longer
      //needed for the link
      callbacks->thisLayerFinished(context);
      break;
   case PPP_STATE_5_STOPPING:
      //Switch to the Stopped state
      pppChangeState(fsm, PPP_STATE_3_STOPPED);
      //Indicate to the lower layers that the automaton is entering the
      //Initial, Closed or Stopped states. The lower layer is no longer
      //needed for the link
      callbacks->thisLayerFinished(context);
      break;
   case PPP_STATE_6_REQ_SENT:
   case PPP_STATE_7_ACK_RCVD:
      //Switch to the Req-Sent state
      pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
      break;
   case PPP_STATE_8_ACK_SENT:
      //Stay in current state
      break;
   case PPP_STATE_9_OPENED:
      //Send Configure-Req packet
      callbacks->sendConfigureReq(context);
      //Switch to the Req-Sent state
      pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
      //Indicate to the upper layers that the automaton is leaving the Opened
      //state. The link is no longer available for network traffic
      callbacks->thisLayerDown(context);
      break;
   default:
      //This event cannot occur in a properly implemented automaton.
      //No transition is taken, and the implementation should not
      //reset or freeze
      break;
   }
}


/**
 * @brief Process Receive-Unknown-Code event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 * @param[in] packet Un-interpretable packet received from the peer
 **/

void pppRcvUnknownCodeEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks, const PppPacket *packet)
{
   //Check current state
   switch(fsm->state)
   {
   case PPP_STATE_2_CLOSED:
   case PPP_STATE_3_STOPPED:
   case PPP_STATE_4_CLOSING:
   case PPP_STATE_5_STOPPING:
   case PPP_STATE_6_REQ_SENT:
   case PPP_STATE_7_ACK_RCVD:
   case PPP_STATE_8_ACK_SENT:
   case PPP_STATE_9_OPENED:
      //Send Reject-Code packet
      callbacks->sendCodeRej(context, packet);
      //Stay in current state
      break;
   default:
      //This event cannot occur in a properly implemented automaton.
      //No transition is taken, and the implementation should not
      //reset or freeze
      break;
   }
}

/**
 * @brief Process Receive-Code-Reject or Receive-Protocol-Reject event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 * @param[in] acceptable This parameter tells whether the rejected value
 *   is acceptable or catastrophic
 **/

void pppRcvCodeRejEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks, bool_t acceptable)
{
   //Check whether the rejected value is acceptable or catastrophic
   if(acceptable)
   {
      //The RXJ+ event arises when the rejected value is acceptable, such
      //as a Code-Reject of an extended code, or a Protocol-Reject of a
      //NCP. These are within the scope of normal operation
      switch(fsm->state)
      {
      case PPP_STATE_2_CLOSED:
      case PPP_STATE_3_STOPPED:
      case PPP_STATE_4_CLOSING:
      case PPP_STATE_5_STOPPING:
      case PPP_STATE_6_REQ_SENT:
         //Stay in current state
         break;
      case PPP_STATE_7_ACK_RCVD:
         //Switch to the Req-Sent state
         pppChangeState(fsm, PPP_STATE_6_REQ_SENT);
         break;
      case PPP_STATE_8_ACK_SENT:
      case PPP_STATE_9_OPENED:
         //Stay in current state
         break;
      default:
         //This event cannot occur in a properly implemented automaton.
         //No transition is taken, and the implementation should not
         //reset or freeze
         break;
      }
   }
   else
   {
      //The RXJ- event arises when the rejected value is catastrophic,
      //such as a Code-Reject of Configure-Request, or a Protocol-Reject
      //of LCP! This event communicates an unrecoverable error that
      //terminates the connection
      switch(fsm->state)
      {
      case PPP_STATE_2_CLOSED:
      case PPP_STATE_3_STOPPED:
         //Indicate to the lower layers that the automaton is entering the
         //Initial, Closed or Stopped states. The lower layer is no longer
         //needed for the link
         callbacks->thisLayerFinished(context);
         //Stay in current state
         break;
      case PPP_STATE_4_CLOSING:
         //Switch to the Closed state
         pppChangeState(fsm, PPP_STATE_2_CLOSED);
         //Indicate to the lower layers that the automaton is entering the
         //Initial, Closed or Stopped states. The lower layer is no longer
         //needed for the link
         callbacks->thisLayerFinished(context);
         break;
      case PPP_STATE_5_STOPPING:
      case PPP_STATE_6_REQ_SENT:
      case PPP_STATE_7_ACK_RCVD:
      case PPP_STATE_8_ACK_SENT:
         //Switch to the Stopped state
         pppChangeState(fsm, PPP_STATE_3_STOPPED);
         //Indicate to the lower layers that the automaton is entering the
         //Initial, Closed or Stopped states. The lower layer is no longer
         //needed for the link
         callbacks->thisLayerFinished(context);
         break;
      case PPP_STATE_9_OPENED:
         //Initialize restart counter
         callbacks->initRestartCount(context, PPP_MAX_TERMINATE);
         //Send Terminate-Req packet
         callbacks->sendTerminateReq(context);
         //Switch to the Stopping state
         pppChangeState(fsm, PPP_STATE_5_STOPPING);
         //Indicate to the upper layers that the automaton is leaving the Opened
         //state. The link is no longer available for network traffic
         callbacks->thisLayerDown(context);
         break;
      default:
         //This event cannot occur in a properly implemented automaton.
         //No transition is taken, and the implementation should not
         //reset or freeze
         break;
      }
   }
}


/**
 * @brief Process Receive-Echo-Request event
 * @param[in] context PPP context
 * @param[in,out] fsm Finite state machine
 * @param[in] callbacks FSM actions
 * @param[in] echoReqPacket Echo-Request packet received from the peer
 **/

void pppRcvEchoReqEvent(PppContext *context, PppFsm *fsm,
   const PppCallbacks *callbacks, const PppEchoPacket *echoReqPacket)
{
   //Check current state
   switch(fsm->state)
   {
   case PPP_STATE_2_CLOSED:
   case PPP_STATE_3_STOPPED:
   case PPP_STATE_4_CLOSING:
   case PPP_STATE_5_STOPPING:
   case PPP_STATE_6_REQ_SENT:
   case PPP_STATE_7_ACK_RCVD:
   case PPP_STATE_8_ACK_SENT:
      //Stay in current state
      break;
   case PPP_STATE_9_OPENED:
      //Send Echo-Reply packet
      callbacks->sendEchoRep(context, echoReqPacket);
      //Stay in current state
      break;
   default:
      //This event cannot occur in a properly implemented automaton.
      //No transition is taken, and the implementation should not
      //reset or freeze
      break;
   }
}


/**
 * @brief Update PPP FSM state
 * @param[in,out] fsm Finite state machine
 * @param[in] newState New PPP state to switch to
 **/

void pppChangeState(PppFsm *fsm, PppState newState)
{
#if (PPP_TRACE_LEVEL >= TRACE_LEVEL_INFO)
   //PPP FSM states
   static const char_t *stateLabel[] =
   {
      "INITIAL",  //0
      "STARTING", //1
      "CLOSED",   //2
      "STOPPED",  //3
      "CLOSING",  //4
      "STOPPING", //5
      "REQ_SENT", //6
      "ACK_RCVD", //7
      "ACK_SENT", //8
      "OPENED"    //9
   };

   //Sanity check
   if(fsm->state < arraysize(stateLabel) && newState < arraysize(stateLabel))
   {
      //Debug message
      TRACE_INFO("PPP FSM: %s (%u) -> %s (%u)\r\n", stateLabel[fsm->state],
         fsm->state, stateLabel[newState], newState);
   }
#endif

   //Switch to the new state
   fsm->state = newState;
}

#endif
