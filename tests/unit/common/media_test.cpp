/// media tests

#include <string.h>
#include <iostream>
#include <cstdint>
#include <vector>
#include <deque>

//#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

#include "gcode_input_filter.hpp"

using Catch::Matchers::Equals;

struct GCRec {
    std::string line;
    uint32_t lastSuccessfulFileOffset;
    GCRec(const char *gc, uint32_t o)
        : line(gc)
        , lastSuccessfulFileOffset(o) {}
};

using GCodesDeq = std::deque<GCRec>;

// synthetic tests on memory buffers
static const char gcodeTooLong[] = "G1\n"
                                   "G2 ; with comment\n"
                                   "; G3 commented completely\n"
                                   "G4 with deliberately insane string behind it to exceed the 96 bytes of input buffer 12345678901234567890\n"
                                   "; if(condition slicer profile){ G1 G2 G3 G4 G5 G6 G7 G8 G9 G10 }else{ G1 G2 G3 G4 G5 G6 G7 G8 G9 G10 } G1 G2 G3 G4 G5 G6 G7 G8 G9 G10 G1 G2 G3 G4 G5 G6 G7 G8 G9 G10 \n";

static const char gcode0[] = "G1\n"
                             "G2 ; with comment\n"
                             "; G3 commented completely\n"
                             "G4 with normal string behind within 96 bytes of input buffer\n"
                             "; if(condition slicer profile){ G1 G2 G3 G4 G5 G6 G7 G8 G9 G10 }else{ G1 G2 G3 G4 G5 G6 G7 G8 G9 G10 } G1 G2 G3 G4 G5 G6 G7 G8 G9 G10 G1 G2 G3 G4 G5 G6 G7 G8 G9 G10 \n";

constexpr uint32_t gcBuffSize = 97;

enum EMediaLoop { OK,
    ERROR,
    ONHOLD };

template <
    typename ENQUEUEGCODE,
    uint32_t RDBUFSIZE = 64,
    uint32_t GCBUFSIZE = gcBuffSize>
EMediaLoop media_loop_inner(ENQUEUEGCODE enq) {
    using R = RDbuf<RDBUFSIZE>;
    R rdbuf;
    using G = GCodeInputFilter<GCBUFSIZE>;
    G g;
    const char *g1 = gcode0;

    for (;;) {
        switch (rdbuf.Fill(
            [&](char *rdbuf, uint32_t rdbufsize, uint32_t *bytes_read) -> bool {
                uint32_t remainingBytes = (uint32_t)((gcode0 + sizeof(gcode0) - 1) - g1);
                *bytes_read = std::min(rdbufsize, remainingBytes);
                std::copy(g1, g1 + *bytes_read, rdbuf);
                g1 += *bytes_read;
                return true;
            },
            [&]() -> bool {
                return g1 == gcode0 + sizeof(gcode0) - 1; // skip the terminal '\0'
            })) {
        case R::END:
            return EMediaLoop::OK;
        case R::OK: {
            int frv = g.Filter(rdbuf, enq);
            if (frv == (int)rdbuf.size()) {
                break; // all data sucessfully processed
            }
            if (frv >= 0) {
                // a serious problem - not all data could have been pushed into Marlin - usually caused by some malformed G-code line
                return EMediaLoop::ERROR;
            } else {
                // on hold with enqueuing data into Marlin - return from this method and wait outside
                return EMediaLoop::ONHOLD;
            }
        } break;
        case R::ERROR:
            return EMediaLoop::ERROR;
        }
    }
    return EMediaLoop::ERROR;
}

void TEST_CASE_preparse_file_basic() {
    GCodesDeq enqueued_gcodes;

    REQUIRE(media_loop_inner(
                [&](const char *gcodebuff, uint32_t new_file_offset) -> bool {
                    enqueued_gcodes.emplace_back(GCRec(gcodebuff, new_file_offset));
                    return true;
                })
        == EMediaLoop::OK);

    // check, that we have in enqueued gcodes:
    REQUIRE(enqueued_gcodes.size() == 3);
    REQUIRE(enqueued_gcodes[0].line == "G1");
    REQUIRE(enqueued_gcodes[0].lastSuccessfulFileOffset == 3);
    REQUIRE(enqueued_gcodes[1].line == "G2 ");
    REQUIRE(enqueued_gcodes[1].lastSuccessfulFileOffset == 21);
    REQUIRE(enqueued_gcodes[2].line == "G4 with normal string behind within 96 bytes of input buffer");
    REQUIRE(enqueued_gcodes[2].lastSuccessfulFileOffset == 108);
}

TEST_CASE("preparse_file basic", "") {
    TEST_CASE_preparse_file_basic();
}

template <
    uint32_t RDBUFSIZE = 64,
    uint32_t GCBUFSIZE = gcBuffSize>
bool media_loop_inner_cause_error(GCodesDeq &enqueued_gcodes, uint32_t errorOffset, uint32_t faultBytes) {
    using R = RDbuf<RDBUFSIZE>;
    R rdbuf;
    using G = GCodeInputFilter<GCBUFSIZE>;
    G g;
    // uint32_t fileRestartOffset = 0; // where the file read was started or restarted after an error
    const char *g1 = gcode0;
    uint32_t lastSuccessfulGcodeOffset = 0;
    bool errorEmitted = false;

    for (;;) {
        switch (rdbuf.Fill(
            [&](char *rdbuf, uint32_t rdbufsize, uint32_t *bytes_read) -> bool {
                uint32_t remainingBytes = (uint32_t)((gcode0 + sizeof(gcode0)) - g1);
                uint32_t bytesNormallyRead = std::min(rdbufsize, remainingBytes);

                if ((uint32_t)((g1 + bytesNormallyRead) - gcode0) > errorOffset && (!errorEmitted)) { // cause an error
                    *bytes_read = faultBytes;                                                         // some random number - simulating a read failure inside a block
                    g1 += *bytes_read;
                    errorEmitted = true;
                    return false;
                }

                *bytes_read = bytesNormallyRead;
                std::copy(g1, g1 + *bytes_read, rdbuf);
                g1 += *bytes_read;
                return true;
            },
            [&]() -> bool {
                return g1 == gcode0 + sizeof(gcode0);
            })) {
        case R::END:
            return true;
        case R::OK:
            if (g.Filter(rdbuf,
                    [&](const char *gcodebuff, uint32_t new_file_offset) -> bool {
                        enqueued_gcodes.emplace_back(GCRec(gcodebuff, new_file_offset));
                        lastSuccessfulGcodeOffset = new_file_offset;
                        return true;
                    })
                != (int)rdbuf.size()) {
                return false;
            }
            break;
        case R::ERROR:
            // there was an (intentional) error reading the input stream, handle it by recovering reading
            // - do a "seek" in the input stream from the last successful position "enqueued into Marlin":
            // that's something media_print_resume() does by calling fseek()
            g1 = gcode0 + lastSuccessfulGcodeOffset;
            rdbuf.reset(lastSuccessfulGcodeOffset); // buffer will be filled newly
            g.reset();                              // filter will also start newly, all the already processed (but failed) content must be scrapped
            // and leave the cycle run again
            break;
        }
    }
    return false;
}

void TEST_CASE_preparse_file_read_error(uint32_t errorOffset, uint32_t faultBytes) {
    GCodesDeq enqueued_gcodes;
    REQUIRE(media_loop_inner_cause_error(
        enqueued_gcodes,
        errorOffset,
        faultBytes));

    // check, that we have in enqueued gcodes:
    REQUIRE(enqueued_gcodes.size() == 3);
    REQUIRE(enqueued_gcodes[0].line == "G1");
    REQUIRE(enqueued_gcodes[0].lastSuccessfulFileOffset == 3);
    REQUIRE(enqueued_gcodes[1].line == "G2 ");
    REQUIRE(enqueued_gcodes[1].lastSuccessfulFileOffset == 21);
    REQUIRE(enqueued_gcodes[2].line == "G4 with normal string behind within 96 bytes of input buffer");
    REQUIRE(enqueued_gcodes[2].lastSuccessfulFileOffset == 108);
}

TEST_CASE("preparse_file read_errors", "") {
    for (uint32_t errorOffset = 0; errorOffset < sizeof(gcode0); ++errorOffset) {
        for (uint32_t faultBytes = 0; faultBytes < 64; ++faultBytes) {
            TEST_CASE_preparse_file_read_error(errorOffset, faultBytes);
        }
    }
}

EMediaLoop TEST_CASE_preparse_file_on_hold(GCodesDeq &enqueued_gcodes) {
    // simulate waiting for emptying of the Marlin queue
    // - just a simple hack - every 3rd call will work/return true
    uint32_t enqueueCalls = 0;

    using R = RDbuf<64>;
    R rdbuf;
    using G = GCodeInputFilter<97>;
    G g;
    const char *g1 = gcode0;

    for (;;) {
        auto rdf = R::OK;

        // avoid filling the buffer again if the filter hasn't finished processing
        if (!g.OnHold()) {
            rdf = rdbuf.Fill(
                [&](char *rdbuf, uint32_t rdbufsize, uint32_t *bytes_read) -> bool {
                    uint32_t remainingBytes = (uint32_t)((gcode0 + sizeof(gcode0) - 1) - g1);
                    *bytes_read = std::min(rdbufsize, remainingBytes);
                    std::copy(g1, g1 + *bytes_read, rdbuf);
                    g1 += *bytes_read;
                    return true;
                },
                [&]() -> bool {
                    return g1 == gcode0 + sizeof(gcode0) - 1; // skip the terminal '\0'
                });
        }

        switch (rdf) {
        case R::END:
            return EMediaLoop::OK;
        case R::OK: {
            int frv = g.Filter(rdbuf,
                [&](const char *gcodebuff, uint32_t new_file_offset) -> bool {
                    ++enqueueCalls;
                    if (enqueueCalls % 3 == 0) {
                        enqueued_gcodes.emplace_back(GCRec(gcodebuff, new_file_offset));
                        return true;
                    } else {
                        return false;
                    }
                });
            if (frv == (int)rdbuf.size()) {
                break; // all data sucessfully processed
            }
            if (frv >= 0) {
                // a serious problem - not all data could have been pushed into Marlin - usually caused by some malformed G-code line
                return EMediaLoop::ERROR;
            } else {
                // on hold with enqueuing data into Marlin - just break and let the cycle run again
                break;
            }
        } break;
        case R::ERROR:
            return EMediaLoop::ERROR;
        }
    }
    return EMediaLoop::ERROR;
}

TEST_CASE("preparse_file on_hold", "") {
    GCodesDeq enqueued_gcodes;

    REQUIRE(TEST_CASE_preparse_file_on_hold(enqueued_gcodes) == EMediaLoop::OK);

    // check, that we have in enqueued gcodes:
    REQUIRE(enqueued_gcodes.size() == 3);
    REQUIRE(enqueued_gcodes[0].line == "G1");
    REQUIRE(enqueued_gcodes[0].lastSuccessfulFileOffset == 3);
    REQUIRE(enqueued_gcodes[1].line == "G2 ");
    REQUIRE(enqueued_gcodes[1].lastSuccessfulFileOffset == 21);
    REQUIRE(enqueued_gcodes[2].line == "G4 with normal string behind within 96 bytes of input buffer");
    REQUIRE(enqueued_gcodes[2].lastSuccessfulFileOffset == 108);
}

EMediaLoop TEST_CASE_preparse_line_too_long(GCodesDeq &enqueued_gcodes) {
    using R = RDbuf<64>;
    R rdbuf;
    using G = GCodeInputFilter<97>;
    G g;
    const char *g1 = gcodeTooLong;

    for (;;) {
        switch (rdbuf.Fill(
            [&](char *rdbuf, uint32_t rdbufsize, uint32_t *bytes_read) -> bool {
                uint32_t remainingBytes = (uint32_t)((gcodeTooLong + sizeof(gcodeTooLong) - 1) - g1);
                *bytes_read = std::min(rdbufsize, remainingBytes);
                std::copy(g1, g1 + *bytes_read, rdbuf);
                g1 += *bytes_read;
                return true;
            },
            [&]() -> bool {
                return g1 == gcodeTooLong + sizeof(gcode0) - 1; // skip the terminal '\0'
            })) {
        case R::END:
            return EMediaLoop::OK;
        case R::OK: {
            int frv = g.Filter(rdbuf, [&](const char *gcodebuff, uint32_t new_file_offset) -> bool {
                enqueued_gcodes.emplace_back(GCRec(gcodebuff, new_file_offset));
                return true;
            });
            if (frv == (int)rdbuf.size()) {
                break; // all data sucessfully processed
            }
            if (frv >= 0) {
                // a serious problem - not all data could have been pushed into Marlin - usually caused by some malformed G-code line
                return EMediaLoop::ERROR;
            } else {
                // on hold with enqueuing data into Marlin - return from this method and wait outside
                return EMediaLoop::ONHOLD;
            }
        } break;
        case R::ERROR:
            return EMediaLoop::ERROR;
        }
    }
    return EMediaLoop::ERROR;
}

TEST_CASE("preparse_file line_too_long", "") {
    GCodesDeq enqueued_gcodes;
    REQUIRE(TEST_CASE_preparse_line_too_long(enqueued_gcodes) == EMediaLoop::ERROR);

    REQUIRE(enqueued_gcodes.size() == 2);
    REQUIRE(enqueued_gcodes[0].line == "G1");
    REQUIRE(enqueued_gcodes[0].lastSuccessfulFileOffset == 3);
    REQUIRE(enqueued_gcodes[1].line == "G2 ");
    REQUIRE(enqueued_gcodes[1].lastSuccessfulFileOffset == 21);
}
