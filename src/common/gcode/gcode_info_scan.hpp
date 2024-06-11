#pragma once

namespace gcode_info_scan {

enum class ScanStartResult {
    not_started,
    started,
    failed,
};

ScanStartResult scan_start_result();

/// Asynchronously scanning the file specified in GCodeInfo::instance()->gcode_file_path and filling in the GcodeInfo fields to match it.
/// Yes, this is utterly wrong. However, it was wrong like this before, I'm just concentrating the utter-wrongness to this file, so that we can more easily eradicate it.
void start_scan();

/// Cancels the currently running gcode info scan, to save resources.
void cancel_scan();

} // namespace gcode_info_scan
