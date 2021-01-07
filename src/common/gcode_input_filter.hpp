/// @brief G-code input filtering
///
/// Contains a generic and simple read buffering (RDbuf) and a generic filtering class (GCodeInputFilter).
/// Both classes need parametrization of important functionality:
/// - the instance of RDbuf needs to know how to fetch input data
/// - the instance of GCodeInputFilter needs to know how enqueue filtered G-code commands
///
/// Design concerns (over previous implementation of media_loop() in media.cpp):
/// - handle long comment lines correctly, even if they contain G-code commands -> filter these lines out, avoid sending them into Marlin
/// - detect long G-code lines and stop printing in such a case avoiding damage to the machine
/// - avoid FS operations for short chunks of data - ideally read the whole block/sector into RAM -> buffering, avoid f_gets()
/// - make the processing/filtering generic to enable unit testing

#pragma once

#include <stdint.h>
#include <limits>

/// RDbuf handles buffering of input data stream - for now it shall sit on top of raw FATfs's f_read and f_eof functions.
/// It may get replaced in the future when we have "normal" libc file interfaces
///
/// Memory requirements:
/// RDBUFFMAX + some state vars
/// basically any caching size should work, ideally the same size as usb drive sector (but 512 is may be too much for our STM32)
template <uint32_t RDBUFFMAX = 64>
class RDbuf {
    char rdbuf[RDBUFFMAX];
    uint32_t rdbufSize;
    char *r = rdbuf;
    uint32_t startInputOffset; ///< where the buffer starts in the input stream/file
public:
    RDbuf() {
        reset();
    }

    enum EFill { OK,
        ERROR,
        END };

    /// Performs filling the buffer from input stream
    /// How to read the input stream and how to check for its end of input is parametrized externally via:
    /// FREAD:
    /// bool fread(char *rdbuf, uint32_t rdbufsize, uint32_t *bytes_read)
    /// returns false in case of any read error, otherwise true
    ///
    /// FEOF:
    /// bool feof()
    /// returns true in case of input file/stream end
    ///
    /// @returns EFill::OK - the buffer read at least one byte from the input
    ///          EFill::ERROR - a read error occurred, the rdbuf's content is unspecified in this case
    ///          EFill::END - end of input reached
    template <typename FREAD, typename FEOF>
    inline EFill Fill(FREAD fread, FEOF feof) {
        if (feof()) {
            return END;
        }
        // advance "file position" by the last size of buffer read
        startInputOffset += rdbufSize;
        return fread(rdbuf, sizeof(rdbuf), &rdbufSize) ? OK : ERROR;
    }

    // STL-like interface
    inline const char *begin() const { return rdbuf; }
    inline const char *end() const { return begin() + size(); }
    inline uint32_t size() const { return rdbufSize; }
    inline void reset(uint32_t fileRestartOffset = 0) {
        rdbufSize = 0;
        r = rdbuf;
        startInputOffset = fileRestartOffset;
    }
    inline uint32_t startOfs() const { return startInputOffset; }
};

/// GCodeInputFilter filters an input buffer of raw G-code text by stripping all comments
/// @returns processed bytes
///
/// Sadly, the operation is not zero-copy since the G-code line must be in one continuous block to be handed over to Marlin
/// thus crossing borders of the rdbuf is not easy to do now.
///
/// Memory requirements:
/// GCODEBUFFSIZE + some state vars
template <uint32_t GCODEBUFFSIZE = 97>
class GCodeInputFilter {
    char *g;
    const char *r;
    enum States { COMMENT,
        COPY,
        ONHOLD };
    States state = COPY;
    char gcbuf[GCODEBUFFSIZE];

public:
    GCodeInputFilter() {
        reset();
    }

    /// resets the filter to initial state
    inline void reset() {
        g = gcbuf;
        r = nullptr;
        state = COPY;
    }

    /// Perform filtering of the input data
    ///
    /// RDBUF: basically anything that provides the following methods:
    /// const char *begin()const
    /// const char *end()const
    /// uint32_t size()const
    /// and a few others
    ///
    /// ENQUEUE_GCODE:
    /// void enqueue_gcode(const char *gcodebuff, uint32_t new_file_offset)
    /// shall take the content of gcodebuff (zero terminated) and push it into Marlin (or any other processing engine)
    /// new_file_offset represents the offset in the input file/stream AFTER the gcode is pushed into Marlin
    /// - that's because we want to track which commands were successfully pushed into Marlin and processed
    ///   to know exactly where to continue in case of an input file/stream removal/error
    ///
    /// @returns -1 in case of being "on hold" - i.e. was unable to enqueue the G-code into Marlin (can be called again to retry).
    ///          Otherwise it returns a positive number of input bytes processed.
    ///          Please note - if the number != rdbuf.size(), the filter has reached a line which couldn't fit into Marlin's internal line buffer
    ///             which implies the line would have been truncated and could possibly damage the machine being misinterpreted!
    template <typename RDBUF, typename ENQUEUE_GCODE>
    int Filter(const RDBUF &rdbuf, ENQUEUE_GCODE enqueue_gcode) {
        if (OnHold()) {                                                              // try to enqueue the data again if any
            if (!enqueue_gcode(gcbuf, rdbuf.startOfs() + (r - rdbuf.begin()) + 1)) { // +1 is to skip the '\n'
                state = ONHOLD;
                return -1; // we haven't been able to enqueue the data again
            }
            g = gcbuf; // point to the beginning of the buffer again
            state = COPY;
        } else {
            r = rdbuf.begin(); // otherwise restart reading from the start of the input buffer
        }
        for (/* nothing */; r < rdbuf.end(); ++r) {
            switch (*r) {
            case ';':
                state = COMMENT;
                break;
            case '\n':
                if (g != gcbuf) {                                                            // if the line was not empty
                    *g = 0;                                                                  // terminating a copied G-code command
                    if (!enqueue_gcode(gcbuf, rdbuf.startOfs() + (r - rdbuf.begin()) + 1)) { // +1 is to skip the '\n'
                        state = ONHOLD;
                        return -1; // we haven't been able to enqueue the data
                    }
                    g = gcbuf; // point to the beginning of the buffer again
                }
                state = COPY; // anyway, we are switching to the copy state
                break;
            default:
                if (state == COPY) {
                    // Verify, we are not writing beyond the buffer
                    // "normal" g-codes are safe (within 96 bytes), but deliberately malformed lines may overflow if not checked
                    if (g < gcbuf + GCODEBUFFSIZE - 1) {
                        *g = *r;
                        ++g;
                    } else {
                        // @@TODO discuss when doing code review
                        // May be emit a runtime error or a red screen - such a "bad" line may be misinterpreted and possibly damage the machine
                        // Ideally this situation should get caught by G-code checking before starting a print
                        return r - rdbuf.begin();
                    }
                }
                break;
            }
        }
        // silently avoiding signed/unsigned warning - rdbuf's size is max a few hundred bytes long on our platform
        static_assert(sizeof(rdbuf) < std::numeric_limits<int>::max());
        return (int)rdbuf.size();
    }

    /// @returns true if the filter was blocked when trying to enqueue a gcode command
    /// - i.e. the rdbuf has not been processed completely yet
    bool OnHold() const { return state == ONHOLD; }
};
