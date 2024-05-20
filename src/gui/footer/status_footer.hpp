// status_footer.hpp
#pragma once
#include "gui.hpp"
#include <guiconfig/guiconfig.h>

#if HAS_MINI_DISPLAY()
    #include "footer_doubleline.hpp"
using StatusFooter = FooterDoubleLine;
#else
    #include "footer_singleline.hpp"
using StatusFooter = FooterSingleline;
#endif
