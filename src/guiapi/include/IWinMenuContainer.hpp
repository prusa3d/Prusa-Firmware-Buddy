#pragma once

#include <stdint.h>

#pragma pack(push, 1)

class IWinMenuContainer {
public:
    virtual size_t GetCount() = 0;
    virtual IWindowMenuItem &GetItem(size_t pos) = 0;
    virtual ~IWinMenuContainer() {}
};

#pragma pack(pop)
