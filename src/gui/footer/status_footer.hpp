// status_footer.hpp
#pragma once
#include "gui.hpp"

#ifdef USE_ST7789
    #include "footer_doubleline.hpp"
using StatusFooter = FooterDoubleLine;
#endif //USE_ST7789
