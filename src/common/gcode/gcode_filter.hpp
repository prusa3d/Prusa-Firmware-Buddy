#pragma once
#include <cstddef>

/// Filtering input data to get a clear G-Code.
/// Removes whitespaces and comments.
///
/// In case of a too long line with a G-Code it still process the G-Code,
/// but ignores the part that doesn't fit into the buffer.
///
/// Requires `getByte` function to provide next byte from a file.
/// The function has to set corresponding result into provided `State` pointer.
/// If the mandatory function pointer is not set, it will cause a crash.
///
/// max_cmd_size should be set to MAX_CMD_SIZE macro from Marlin config.
class GCodeFilter {
public:
    enum State {
        Ok,
        Eof, // End of file
        Error,
        Timeout,
        NotDownloaded,
    };

    GCodeFilter(char (*getByte)(State *), char *buffer, size_t buffer_size)
        : getByte(getByte)
        , buffer(buffer)
        , buffer_size(buffer_size) {}

    /// Returns next gcode with a corresponding state.
    ///
    /// In case of Error or Skip, returned G-Code will be NULL.
    /// With returned valid G-Code, the state can be Ok or Eof.
    char *nextGcode(State *);

    /// Resets the current state of the G-Code processing.
    ///
    /// Call before processing another file.
    void reset();

private:
    /// Return next byte from a G-Code file.
    ///
    /// Provided `State` has to be set to the corresponding result.
    /// If the state is not Ok, returned value will be ignored.
    char (*getByte)(State *);

    /// Prepares G-Code in the buffer for returning to the client.
    char *prepareGcode();

    char *buffer;
    const size_t buffer_size;
    size_t offset = 0;
    bool wait_new_line = false;
};
