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
#pragma once

/**
 * queue.h - The G-code command queue, which holds commands before they
 *           go to the parser and dispatcher.
 */

#include "../inc/MarlinConfig.h"
#include <limits>
#include <gcode/inject_queue_actions.hpp>

class GCodeQueue {
public:
  /**
   * GCode line number handling. Hosts may include line numbers when sending
   * commands to Marlin, and lines will be checked for sequentiality.
   * M110 N<int> sets the current line number.
   */
  static long last_N, stopped_N;

  static inline void stop() { stopped_N = last_N; }

  /**
   * GCode Command Queue
   * A simple ring buffer of BUFSIZE command strings.
   *
   * Commands are copied into this buffer by the command injectors
   * (immediate, serial, sd card) and they are processed sequentially by
   * the main loop. The gcode.process_next_command method parses the next
   * command and hands off execution to individual handler functions.
   */
  static uint8_t length,  // Count of commands in the queue
                 index_r; // Ring buffer read position

  static char command_buffer[BUFSIZE][MAX_CMD_SIZE];

  static constexpr uint32_t SDPOS_INVALID = std::numeric_limits<uint32_t>::max(); // When sdpos doesn't have valid value

  static uint32_t sdpos;                 // Position in file for the latest instruction (behind the end of the queue)
  static uint32_t last_executed_sdpos;      // (replay) Position of the last executed gcode
  static uint32_t executed_commmand_count; ///< Increased every time a command is executed
  static uint32_t sdpos_buffer[BUFSIZE]; // Ring buffer of (replay) positions (synced with command_buffer)

  /// True pauses processing of serial commands.
  static bool pause_serial_commands;

  /// Return the file position of the _current_ instruction
  /// Red note: right after executing the gcode, the queue is advanced to the next one, so this actually returns the next gcode
  static uint32_t get_current_sdpos() {
    return length ? sdpos_buffer[index_r] : sdpos;
  }

  /*
   * The port that the command was received on
   */
  #if NUM_SERIAL > 1
    static int16_t port[BUFSIZE];
  #endif

  GCodeQueue();

  /**
   * Clear the Marlin command queue and return reading index.
   */
  static void clear();

  /**
   * Enqueue one or many commands to run from program memory.
   * Aborts the current queue, if any.
   * Note: process_injected_command() will process them.
   */
  static void inject_P(ConstexprString pgcode);

  /**
   * Enqueue action to inject_queue, if inject_queue isn't already full
   * @param record [in] record variant
   */
  static bool inject(InjectQueueRecord record);

  /**
   * Enqueue and return only when commands are actually enqueued
   */
  static void enqueue_one_now(const char* cmd);

  /**
   * Enqueue from program memory and return only when commands are actually enqueued
   */
  static void enqueue_now_P(PGM_P const cmd);

  /**
   * Check whether there are any commands yet to be executed
   * 
   * !!! Consider using marlin_server::is_processing() instead, it's usually the right choice
   */
  static bool has_commands_queued();

  /**
   * Get the next command in the queue, optionally log it to SD, then dispatch it
   */
  static void advance();

  /**
   * Add to the circular command queue the next command from:
   *  - The command-injection queue (injected_commands_P)
   *  - The active serial input (usually USB)
   *  - The SD card file being actively printed
   */
  static void get_available_commands();

  /**
   * Send an "ok" message to the host, indicating
   * that a command was successfully processed.
   *
   * If ADVANCED_OK is enabled also include:
   *   N<int>  Line number of the command, if any
   *   P<int>  Planner space remaining
   *   B<int>  Block queue space remaining
   */
  static void ok_to_send();

  /**
   * Clear the serial line and request a resend of
   * the next expected line number.
   */
  static void flush_and_request_resend();

private:

  static uint8_t index_w;  // Ring buffer write position

  static void get_serial_commands();

  #if ENABLED(SDSUPPORT)
    static void get_sdcard_commands();
  #endif

  static void _commit_command(bool say_ok
    #if NUM_SERIAL > 1
      , int16_t p=-1
    #endif
  );

  static bool _enqueue(const char* cmd, bool say_ok=false
    #if NUM_SERIAL > 1
      , int16_t p=-1
    #endif
  );

  // Process the next "immediate" command
  static bool process_injected_command();

public:
  /**
   * Enqueue with Serial Echo (optionally without)
   * Return true on success
   */
  static bool enqueue_one(const char* cmd, bool echo=true);

private:
  static void gcode_line_error(PGM_P const err, const int8_t port);

};

extern GCodeQueue queue;
