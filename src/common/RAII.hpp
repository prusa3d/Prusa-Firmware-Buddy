#pragma once

template <class T>
class AutoRestore {
    T &ref;
    T val;

public:
    AutoRestore(T &ref, T val)
        : ref(ref)
        , val(val) {}
    AutoRestore(T &ref)
        : ref(ref)
        , val(ref) {}
    ~AutoRestore() { ref = val; }
};
