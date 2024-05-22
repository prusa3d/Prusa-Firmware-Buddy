// Ugly hack to remove the host "gettext" declaration from view
#pragma once
#define gettext _gettext
#include_next "libintl.h"
#undef gettext
