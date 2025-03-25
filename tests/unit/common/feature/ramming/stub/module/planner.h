#pragma once

struct Planner {
    void synchronize() {}
    bool draining() { return false; }
};
inline Planner planner;

struct Temporary_Reset_Motion_Parameters {};
