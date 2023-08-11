#pragma once

#pragma pack(push, 1)

struct DockPosition {
    float x;
    float y;
};

inline bool operator==(DockPosition lhs, DockPosition rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

#pragma pack(pop)
