#pragma once

namespace transfers {

class NotifyFilechange {
public:
    virtual ~NotifyFilechange();
    virtual void notify_filechange(const char *filename) = 0;
};

}
