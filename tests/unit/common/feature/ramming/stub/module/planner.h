#pragma once

struct Planner {
    void synchronize() {}
    bool draining() { return false; }
};
inline Planner planner;
