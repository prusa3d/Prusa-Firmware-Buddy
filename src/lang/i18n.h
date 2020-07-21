#pragma once

#include "string_view_utf8.hpp"
#include "translator.hpp"

// Based on recommendation from GNU/gettext
// prepare the sources for localization/internationalization (aka l10n/i18n)
#define _(String)  gettext(String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
