// status_footer.hpp
#pragma once
#include "gui.hpp"

#ifdef USE_ST7789
    #include "footer_mini.hpp"
using StatusFooter = FooterMini;
#endif //USE_ST7789
