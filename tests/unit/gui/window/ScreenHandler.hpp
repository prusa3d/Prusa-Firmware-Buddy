//ScreenHandler.hpp

#pragma once

class Screens {

public:
    static Screens *Access() {
        static Screens ret;
        return &ret;
    }
    void Close() {}
};
