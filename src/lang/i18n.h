#pragma once

#include "string_view_utf8.hpp"
#include "translator.hpp"

/// Based on recommendation from GNU/gettext
/// prepare the sources for localization/internationalization (aka l10n/i18n)

/// Does not alter the content. Provides proper translation at the output
#define _(String) gettext(String)
/// Does not alter the content. This marks texts to be translated
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
