#pragma once

#pragma pack(push, 1)

struct ToolOffset {
    float x;
    float y;
    float z;
};

inline bool operator==(ToolOffset lhs, ToolOffset rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y && lhs.z == rhs.z;
}

#pragma pack(pop)
