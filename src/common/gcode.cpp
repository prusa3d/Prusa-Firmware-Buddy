/// gcode.cpp

#include <stdio.h>
#include <string.h>
#include <algorithm>
#include <math.h>

#include "cmath_ext.h"
#include "marlin_client.h"

constexpr uint16_t bufferSize = 1024;

constexpr float layerHeight = 0.2f;
constexpr float threadWidth = 0.5f;
constexpr float filamentD = 1.75f;
constexpr float pi = 3.1415926535897932384626433832795;
constexpr float extrudeCoef = layerHeight * threadWidth / (pi * SQR(filamentD / 2));

/// Tool to generate string from G codes.
/// Most of the methods can be chained (gc.G(28).G(29).G1(0,0,0,0,0))
class gCode {
private:
    /// G code string; stores whole G code
    std::array<char, bufferSize> code;

    /// Cursor position in code.
    /// This defines end of the string.
    /// There's no need for trailing 0.
    uint16_t pos = 0;
    uint8_t error_ = 0;
    float x_ = NAN;
    float y_ = NAN;

    void updatePosOrError(const int chars, const uint8_t errorNum = 1) {
        if (chars < 0) {
            error_ = errorNum;
        } else {
            pos += chars;
        }
    }

    void addNewLineIfMissing() {
        if (pos > 0 && !isFull() && code[pos - 1] != '\n')
            code[pos++] = '\n';
    }

public:
    gCode() {
        code[0] = 0;
    }

    bool isFull() {
        if (pos >= bufferSize)
            return true;
        return false;
    }

    uint8_t error() {
        return error_;
    }

    inline bool isError() {
        return error_ > 0;
    }

    void resetError() {
        error_ = 0;
    }

    void clear() {
        pos = 0;
    }

    /// Writes a string.
    gCode &write(const char *string) {
        if (isError())
            return *this;

        const int chars = snprintf(&code[pos], bufferSize - pos, string);
        updatePosOrError(chars);
        return *this;
    }

    /// Writes a float.
    gCode &write(const float value) {
        if (isError())
            return *this;

        const int chars = snprintf(&code[pos], bufferSize - pos, "%f", (double)value);
        updatePosOrError(chars);
        return *this;
    }

    /// Writes a parameter e.g. " S120"
    gCode &param(const char parameter, const float value) {
        if (isError())
            return *this;

        const int chars = snprintf(&code[pos], bufferSize - pos, " %c%f", parameter, (double)value);
        updatePosOrError(chars);
        return *this;
    }

    /// Writes e.g. "G28\n" or "G28 ".
    gCode &G(const uint8_t value) {
        if (isError())
            return *this;

        addNewLineIfMissing();
        const int chars = snprintf(&code[pos], bufferSize - pos, "G%d", value);

        if (chars < 0) {
            error_ = 1;
        } else {
            pos += chars;
        }
        return *this;
    }

    /// Writes e.g. "M83\n" or "M83 ".
    gCode &M(const uint8_t value) {
        if (isError())
            return *this;

        addNewLineIfMissing();
        const int chars = snprintf(&code[pos], bufferSize - pos, "M%d", value);

        if (chars < 0) {
            error_ = 1;
        } else {
            pos += chars;
        }
        return *this;
    }

    /// Add space.
    gCode &space() {
        if (isFull() || isError()) {
            error_ = 2;
        } else {
            code[pos++] = ' ';
        }
        return *this;
    }

    /// Add new line.
    gCode &newLine() {
        if (isFull() || isError()) {
            error_ = 2;
        } else {
            code[pos++] = '\n';
        }
        return *this;
    }

    /// Writes e.g. "G1 X10 Y20 Z30 E0.5 F1500\n".
    gCode &G1(const float x, const float y, const float z, const float e, const float f) {
        if (isError())
            return *this;

        G(1);

        int chars = 0;
        if (isfinite(x)) {
            x_ = x;
            chars = snprintf(&code[pos], bufferSize - pos, " X%f", (double)x);
            if (chars < 0) {
                error_ = 3;
                return *this;
            }
            pos += chars;
        }

        if (isfinite(y)) {
            y_ = y;
            chars = snprintf(&code[pos], bufferSize - pos, " Y%f", (double)y);
            if (chars < 0) {
                error_ = 3;
                return *this;
            }
            pos += chars;
        }

        if (isfinite(z)) {
            chars = snprintf(&code[pos], bufferSize - pos, " Z%f", (double)z);
            if (chars < 0) {
                error_ = 3;
                return *this;
            }
            pos += chars;
        }

        if (isfinite(e)) {
            chars = snprintf(&code[pos], bufferSize - pos, " E%f", (double)e);
            if (chars < 0) {
                error_ = 3;
                return *this;
            }
            pos += chars;
        }

        if (isfinite(f)) {
            chars = snprintf(&code[pos], bufferSize - pos, " F%f", (double)f);
            if (chars < 0) {
                error_ = 3;
                return *this;
            }
            pos += chars;
        }

        chars = snprintf(&code[pos], bufferSize - pos, "\n");
        updatePosOrError(chars, 3);

        return *this;
    }

    /// Writes e.g. "G1 X10 Y20 E0.5".
    gCode &G1(const float x, const float y, const float e) {
        return G1(x, y, NAN, e, NAN);
    }

    /// Sends G codes one by one
    /// \returns false if error occurs
    bool send() {
        if (pos >= bufferSize) {
            error_ = 4;
            return false;
        }

        uint16_t eol = 0;
        uint16_t start = 0;
        while (eol < pos) {
            /// find end of line
            while (eol < pos && code[eol] != '\n')
                ++eol;
            code[eol] = '\0'; /// switch \n to \0
            marlin_gcode(&code[start]);
            start = ++eol;
        }
        clear(); /// remove all G codes
        return true;
    }

    /// Extrude from the last point to the specified one
    /// Extrusion uses pre-defined height and width
    gCode &ex(const float x, const float y) {
        if (!isfinite(x_) || !isfinite(y_))
            return *this;
        const float length = sqrt(SQR(x - x_) + SQR(y - y_));
        G1(x, y, length * extrudeCoef);
    }

    /// Set the last point of extrusion
    /// Does not add anything to the G codes
    gCode &lastExtrusion(const float x, const float y) {
        x_ = x;
        y_ = y;
        return *this;
    }

    const std::array<char, bufferSize> &read() {
        return code;
    }
};
