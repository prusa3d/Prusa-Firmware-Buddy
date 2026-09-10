/**
 * Marlin 3D Printer Firmware
 * Copyright (c) 2019 MarlinFirmware [https://github.com/MarlinFirmware/Marlin]
 *
 * Based on Sprinter and grbl.
 * Copyright (c) 2011 Camiel Gubbels / Erik van der Zalm
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * queue.cpp - The G-code command queue
 */

#include "queue.h"
GCodeQueue queue;

#include "gcode.h"

#include "../lcd/ultralcd.h"
#include "../module/planner.h"
#include "../module/temperature.h"
#include "../Marlin.h"
#include <cstring>
#include <iterator>
#include <option/has_serial_print.h>
#if HAS_SERIAL_PRINT()
    #include "serial_printing.hpp"
#endif
#include <gcode/inject_queue.hpp>
#include <feature/cork/tracker.hpp>
#include <marlin_server.hpp>

/**
 * GCode line number handling. Hosts may opt to include line numbers when
 * sending commands to Marlin, and lines will be checked for sequentiality.
 * M110 N<int> sets the current line number.
 */
long gcode_N, GCodeQueue::last_N, GCodeQueue::stopped_N = 0;

/**
 * GCode Command Queue
 * A simple ring buffer of BUFSIZE command strings.
 *
 * Commands are copied into this buffer by the command injectors
 * (immediate, serial, sd card) and they are processed sequentially by
 * the main loop. The gcode.process_next_command method parses the next
 * command and hands off execution to individual handler functions.
 */
uint8_t GCodeQueue::length = 0,  // Count of commands in the queue
        GCodeQueue::index_r = 0, // Ring buffer read position
        GCodeQueue::index_w = 0; // Ring buffer write position

char GCodeQueue::command_buffer[BUFSIZE][MAX_CMD_SIZE];

uint32_t GCodeQueue::sdpos = GCodeQueue::SDPOS_INVALID;
uint32_t GCodeQueue::last_executed_sdpos = GCodeQueue::SDPOS_INVALID;
uint32_t GCodeQueue::executed_commmand_count = 0;
uint32_t GCodeQueue::sdpos_buffer[BUFSIZE];
bool GCodeQueue::pause_serial_commands = false;

/*
 * The port that the command was received on
 */
#if NUM_SERIAL > 1
  // #error dead code found by automatic analyses (see BFW-5461)
  int16_t GCodeQueue::port[BUFSIZE];
#endif

/**
 * Serial command injection
 */

// Number of characters read in the current line of serial input
static int serial_count[NUM_SERIAL] = { 0 };

bool send_ok[BUFSIZE];

/**
 * Next Injected Command pointer. nullptr if no commands are being injected.
 * Used by Marlin internally to ensure that commands initiated from within
 * are enqueued ahead of any pending serial or sd card commands.
 */
static PGM_P injected_commands_P = nullptr;

GCodeQueue::GCodeQueue() {
  // Send "ok" after commands by default
  for (uint8_t i = 0; i < COUNT(send_ok); i++) send_ok[i] = true;
}

/**
 * Check whether there are any commands yet to be executed
 */
bool GCodeQueue::has_commands_queued() {
  return queue.length || injected_commands_P;
}

/**
 * Clear the Marlin command queue
 */
void GCodeQueue::clear() {
  sdpos = last_executed_sdpos;
  index_r = index_w = length = 0;
  buddy::cork::tracker.clear();
}

/**
 * Once a new command is in the ring buffer, call this to commit it
 */
void GCodeQueue::_commit_command(bool say_ok
  #if NUM_SERIAL > 1
    // #error dead code found by automatic analyses (see BFW-5461)
    , int16_t p/*=-1*/
  #endif
) {
  send_ok[index_w] = say_ok;
  #if NUM_SERIAL > 1
    // #error dead code found by automatic analyses (see BFW-5461)
    port[index_w] = p;
  #endif
  sdpos_buffer[index_w] = sdpos;

  if (++index_w >= BUFSIZE) index_w = 0;
  length++;
}

/**
 * Copy a command from RAM into the main command buffer.
 * Return true if the command was successfully added.
 * Return false for a full buffer, a comment, or a command that would not
 * fit a queue slot (>= MAX_CMD_SIZE).
 */
bool GCodeQueue::_enqueue(const char* cmd, bool say_ok/*=false*/
  #if NUM_SERIAL > 1
    // #error dead code found by automatic analyses (see BFW-5461)
    , int16_t pn/*=-1*/
  #endif
) {
  if (*cmd == ';' || length >= BUFSIZE) return false;
  char(&slot)[MAX_CMD_SIZE] = command_buffer[index_w];
  if (strlcpy(slot, cmd, std::size(slot)) >= std::size(slot)) return false;
  _commit_command(say_ok
    #if NUM_SERIAL > 1
      // #error dead code found by automatic analyses (see BFW-5461)
      , pn
    #endif
  );
  return true;
}

/**
 * Enqueue with Serial Echo
 * Return true if the command was consumed
 */
 [[nodiscard]] bool GCodeQueue::enqueue_one(const char* cmd, bool echo/*=true*/) {

  //SERIAL_ECHOPGM("enqueue_one(\"");
  //SERIAL_ECHO(cmd);
  //SERIAL_ECHOPGM("\") \n");

  if (*cmd == 0 || *cmd == '\n' || *cmd == '\r' || *cmd == ';') return true;

  if (_enqueue(cmd)) {
    if (echo) {
        SERIAL_ECHO_START();
        SERIAL_ECHOLNPAIR(MSG_ENQUEUEING, cmd, "\"");
    }
    return true;
  }
  return false;
}

/**
 * Process the next "immediate" command.
 * Return 'true' if any commands were processed,
 * or remain to process.
 */
bool GCodeQueue::process_injected_command() {
  if (injected_commands_P == nullptr) {
    const auto inject_gcode = inject_queue.get_gcode();
    if (inject_gcode.has_value()) {
      // successfully received G-Code stream [const char *]
      injected_commands_P = *inject_gcode;
    } else if (inject_gcode.error() == InjectQueue::GetGCodeError::empty) {
      // Empty inject_queue -> continue parsing standard G-Code queue
      return false;
    } else {
      // Inject G-Code is not ready yet (buffering from file)
      // or loading error occurred, in both cases skip standard G-Code queue
      return true;
    }
  }

  char c;
  size_t i = 0;
  while ((c = pgm_read_byte(&injected_commands_P[i])) && c != '\n') i++;

  // Extract current command and move pointer to next command
  char cmd[i + 1];
  memcpy_P(cmd, injected_commands_P, i);
  cmd[i] = '\0';
  injected_commands_P = c ? injected_commands_P + i + 1 : nullptr;

  // Execute command if non-blank
  if (i) {
    parser.parse(cmd);
    PORT_REDIRECT(SERIAL_PORT);
    gcode.process_parsed_command();
  }
  return true;
}

/**
 * Enqueue one or many commands to inject_queue, to run from program memory.
 * Do not inject a comment or use leading spaces!
 * G-Codes are enqueued only if inject_queue is not already full
 * Note: process_injected_command() will be called to drain any commands afterwards
 */
void GCodeQueue::inject_P(ConstexprString pgcode) { inject(GCodeLiteral(pgcode)); }

/**
 * Enqueue action to inject_queue, if inject_queue isn't already full
 */
bool GCodeQueue::inject(InjectQueueRecord request) { 
  return inject_queue.try_push(request);
}

/**
 * Enqueue and return only when commands are actually enqueued.
 * Never call this from a G-code handler!
 */
void GCodeQueue::enqueue_one_now(const char* cmd) { while (!enqueue_one(cmd)) idle(true); }

/**
 * Enqueue from program memory and return only when commands are actually enqueued
 * Never call this from a G-code handler!
 */
void GCodeQueue::enqueue_now_P(PGM_P const pgcode) {
  size_t i = 0;
  PGM_P p = pgcode;
  for (;;) {
    char c;
    while ((c = pgm_read_byte(&p[i])) && c != '\n') i++;
    char cmd[i + 1];
    memcpy_P(cmd, p, i);
    cmd[i] = '\0';
    enqueue_one_now(cmd);
    if (!c) break;
    p += i + 1;
  }
}

/**
 * Send an "ok" message to the host, indicating
 * that a command was successfully processed.
 *
 * If ADVANCED_OK is enabled also include:
 *   N<int>  Line number of the command, if any
 *   P<int>  Planner space remaining
 *   B<int>  Block queue space remaining
 */
void GCodeQueue::ok_to_send() {
  #if NUM_SERIAL > 1
    // #error dead code found by automatic analyses (see BFW-5461)
    const int16_t pn = port[index_r];
    if (pn < 0) return;
    PORT_REDIRECT(pn);
  #endif
  if (!send_ok[index_r]) return;
  SERIAL_ECHOPGM(MSG_OK);
  #if ENABLED(ADVANCED_OK)
    // #error dead code found by automatic analyses (see BFW-5461)
    char* p = command_buffer[index_r];
    if (*p == 'N') {
      SERIAL_ECHO(' ');
      SERIAL_ECHO(*p++);
      while (NUMERIC_SIGNED(*p))
        SERIAL_ECHO(*p++);
    }
    SERIAL_ECHOPGM(" P"); SERIAL_ECHO(int(BLOCK_BUFFER_SIZE - planner.movesplanned() - 1));
    SERIAL_ECHOPGM(" B"); SERIAL_ECHO(BUFSIZE - length);
  #endif
  SERIAL_EOL();
}

/**
 * Send a "Resend: nnn" message to the host to
 * indicate that a command needs to be re-sent.
 */
void GCodeQueue::flush_and_request_resend() {
  #if NUM_SERIAL > 1
    // #error dead code found by automatic analyses (see BFW-5461)
    const int16_t p = port[index_r];
    if (p < 0) return;
    PORT_REDIRECT(p);
  #endif
  SERIAL_FLUSH();
  SERIAL_ECHOPGM(MSG_RESEND);
  SERIAL_ECHOLN(last_N + 1);
  ok_to_send();
}

inline bool serial_data_available() {
  return false
    || MYSERIAL0.available()
    #if NUM_SERIAL > 1
      // #error dead code found by automatic analyses (see BFW-5461)
      || MYSERIAL1.available()
    #endif
  ;
}

inline int read_serial(const uint8_t index) {
  switch (index) {
    case 0: return MYSERIAL0.read();
    #if NUM_SERIAL > 1
      // #error dead code found by automatic analyses (see BFW-5461)
      case 1: return MYSERIAL1.read();
    #endif
    default: return -1;
  }
}

void GCodeQueue::gcode_line_error(PGM_P const err, const int8_t port) {
  PORT_REDIRECT(port);
  SERIAL_ERROR_START();
  serialprintPGM(err);
  SERIAL_ECHOLN(last_N);
  while (read_serial(port) != -1);           // clear out the RX buffer
  flush_and_request_resend();
  serial_count[port] = 0;
}

FORCE_INLINE bool is_M29(const char * const cmd) {  // matches "M29" & "M29 ", but not "M290", etc
  const char * const m29 = strstr_P(cmd, PSTR("M29"));
  return m29 && !NUMERIC(m29[3]);
}

/**
 * Get all commands waiting on the serial port and queue them.
 * Exit when the buffer is full or when no more characters are
 * left on the serial port.
 */
void GCodeQueue::get_serial_commands() {
  // Mutually exclusive: a comment is consumed before the overflow check, so a line
  // can never be both.
  enum class LineState : uint8_t { ok, too_long, comment };
  static_assert(LineState {} == LineState::ok, "zero-initialised state must mean ok");

  static char serial_line_buffer[NUM_SERIAL][MAX_CMD_SIZE];
  static LineState serial_line_state[NUM_SERIAL] = {};

  // If the command buffer is empty for too long,
  // send "wait" to indicate Marlin is still waiting.
  #if NO_TIMEOUTS > 0
    // #error dead code found by automatic analyses (see BFW-5461)
    static millis_t last_command_time = 0;
    const millis_t ms = millis();
    if (length == 0 && !serial_data_available() && ELAPSED(ms, last_command_time + NO_TIMEOUTS)) {
      SERIAL_ECHOLNPGM(MSG_WAIT);
      last_command_time = ms;
    }
  #endif

  /**
   * Loop while serial characters are incoming and the queue is not full
   */
  while (length < BUFSIZE && serial_data_available()) {
    for (uint8_t i = 0; i < NUM_SERIAL; ++i) {
      int c;
      if ((c = read_serial(i)) < 0) continue;

      char serial_char = c;

      /**
       * If the character ends the line
       */
      if (serial_char == '\n' || serial_char == '\r') {
        const bool line_too_long = (serial_line_state[i] == LineState::too_long);

        // Start the next line with a clean state
        serial_line_state[i] = LineState::ok;

        // Skip empty lines and comments
        if (!serial_count[i]) { thermalManager.manage_heater(); continue; }

        serial_line_buffer[i][serial_count[i]] = 0;       // Terminate string
        serial_count[i] = 0;                              // Reset buffer

        char* command = serial_line_buffer[i];

        while (*command == ' ') command++;                // Skip leading spaces
        char *npos = (*command == 'N') ? command : nullptr;  // Require the N parameter to start the line

        if (npos) {

          bool M110 = strstr_P(command, PSTR("M110")) != nullptr;

          if (M110) {
            char* n2pos = strchr(command + 4, 'N');
            if (n2pos) npos = n2pos;
          }

          gcode_N = strtol(npos + 1, nullptr, 10);

          if (gcode_N != last_N + 1 && !M110)
            return gcode_line_error(PSTR(MSG_ERR_LINE_NO), i);

          char *apos = strrchr(command, '*');
          if (apos) {
            uint8_t checksum = 0, count = uint8_t(apos - command);
            while (count) checksum ^= command[--count];
            if (strtol(apos + 1, nullptr, 10) != checksum)
              return gcode_line_error(PSTR(MSG_ERR_CHECKSUM_MISMATCH), i);
          }
          else
            return gcode_line_error(PSTR(MSG_ERR_NO_CHECKSUM), i);

          last_N = gcode_N;
        }

        #if DISABLED(EMERGENCY_PARSER)
          // Process critical commands early
          if (strcmp(command, "M108") == 0) {
            wait_for_heatup = false;
          }
          if (strcmp(command, "M112") == 0) kill(PSTR("Emergency stop (M112)"), nullptr, true);
          if (strcmp(command, "M410") == 0) quickstop_stepper();
        #endif

        #if defined(NO_TIMEOUTS) && NO_TIMEOUTS > 0
          // #error dead code found by automatic analyses (see BFW-5461)
          last_command_time = ms;
        #endif

#if HAS_SERIAL_PRINT()
        // notify serial printing about command
        SerialPrinting::serial_command_hook(command);
#endif

        if (line_too_long) {
          // Reported only once the cropped command is certain to run - a dialog raised
          // for a line rejected above could not be dismissed while the host resends it.
          // Echo rather than error: hosts match error text against a list of recoverable
          // conditions and abort the print on anything else.
          PORT_REDIRECT(i);
          SERIAL_ECHO_MSG(MSG_LINE_TOO_LONG);
          marlin_server::set_warning(WarningType::GcodeCropped);
        }

        // Add the command to the queue
        _enqueue(serial_line_buffer[i], true
          #if NUM_SERIAL > 1
            // #error dead code found by automatic analyses (see BFW-5461)
            , i
          #endif
        );
      }
      else if (serial_line_state[i] == LineState::comment) {
        // Drop the rest of the line, however long it grows
      }
      else if (serial_char == ';') {
        serial_line_state[i] = LineState::comment;
      }
      else if (serial_count[i] >= MAX_CMD_SIZE - 1) {
        // Keep fetching, but ignore normal characters beyond the max length
        // The cropped command will be injected when EOL is reached
        serial_line_state[i] = LineState::too_long;
      }
      else if (serial_char == '\\') {  // Handle escapes
        // if we have one more character, copy it over
        if ((c = read_serial(i)) >= 0) {
          serial_line_buffer[i][serial_count[i]++] = (char)c;
        }
      }
      else { // it's not a newline, carriage return, comment or escape char
        serial_line_buffer[i][serial_count[i]++] = serial_char;
      }
    } // for NUM_SERIAL
  } // queue has space, serial has data
}

/**
 * Add to the circular command queue the next command from:
 *  - The command-injection queue (injected_commands_P)
 *  - The active serial input (usually USB)
 *  - The SD card file being actively printed
 */
void GCodeQueue::get_available_commands() {

  if (!pause_serial_commands)
    get_serial_commands();

}

/**
 * Get the next command in the queue, optionally log it to SD, then dispatch it
 */
void GCodeQueue::advance() {

  // Process immediate commands
  if (process_injected_command()) {
    delay(1); // Safety measure to avoid locking async job in inject queue (buffering a file)
    return;
  }

  // Return if the G-code buffer is empty
  if (!length) {
    delay(1);
    return;
  }

  last_executed_sdpos = queue.get_current_sdpos();
  gcode.process_next_command();
  executed_commmand_count++;

  // The queue may be reset by a command handler or by code invoked by idle() within a handler
  if (length) {
    --length;
    if (++index_r >= BUFSIZE) index_r = 0;
  }

}
