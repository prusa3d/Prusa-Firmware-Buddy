//ScreenFirstLayer.hpp
#pragma once
#include "IScreenPrinting.hpp"
#include "gui.hpp"
#include "window_header.hpp"
#include "status_footer.h"
#include "resource.h"

class ScreenFirstLayer : public IScreenPrinting {
protected:
    window_header_t header;
    status_footer_t footer;

public:
    ScreenFirstLayer(string_view_utf8 caption);
    ~ScreenFirstLayer();
};
