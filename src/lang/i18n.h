/// @file i18n.h
///
/// @brief Internationalization (i18n) support for Buddy FW
///
/// Based on recommendation from GNU/gettext
/// prepare the sources for localization/internationalization (aka l10n/i18n)
///
/// Typical usage scenarios:
///
/// Translated text:
///
///     static const char some_EN_text[] = N_("this is a text"); // this text is marked for extraction with GNU/gettext
///     string_view_utf8 translatedText = _(some_EN_text); // this text will be translated
///
/// alternative:
///
///     string_view_utf8 translatedText = _("this is a text"); // this text will be translated (also marked for extraction with GNU/gettext)
///
/// It is recommended to define the source texts as static const char[] - they will not be copied to RAM at runtime
/// It is perfectly safe to store string_view_utf8 in classes and/or initialize it in a constructor.
/// One only has to consider, what the string_view_utf8 and translation process do in reality:
/// - the translated text is computed in the function of _()
/// - string_view_utf8 only holds the "pointer" to the translated text
///
/// Untranslated text (wrapping normal const char * into a string view):
///
///     static const char some_EN_text[] = "this is a text";
///     string_view_utf8 untranslatedText = string_view_utf8::MakeCPUFLASH((const uint8_t *)some_EN_text); // this text will NOT be translated
///
/// Using translated text in snprintf:
///
///     char fmt[30]; // long enough to hold every translated variant
///     static const char some_EN_text[] = N_("this is a text %f %d");
///     _(some_EN_text).copyToRAM(fmt, sizeof(fmt)); // note the underscore at the beginning of this line
///     snprintf(out, sizeof(out) / sizeof(char), fmt, 0.1, 234);
///
/// Passing a text buffer to functions requiring a string view:
///
///     char buf[256];
///     // ... write something into the buffer
///     string_view_utf8 sv = string_view_utf8::MakeRAM((const uint8_t *)buf); // one must MAKE SURE, that buf exists as long as the string view does!
///     function(sv);
///
/// Using string view to hold empty strings or nullptr:
///
///     string_view_utf8 empty = string_view_utf8::MakeNULLSTR();
///
/// Computing total number of characters (glyphs) in a string view:
///
///     string_view_utf8 s;
///     size_t glyphs = s.computeNumUtf8CharsAndRewind(); // beware - this modifies the internal read "pointer" of the string view

#pragma once

#include "string_view_utf8.hpp"
#include "translator.hpp"

/// Translate the surrounded text. Does not alter the content. Provides proper translation at the output.
/// If translation not available, returns the same text.
#define _(String) gettext(String)

template <char... chars>
struct TranslatableStringT {
    static constexpr inline const char str[] = { chars..., '\0' };

    constexpr inline operator const char *() const {
        return str;
    }
};

template <typename T, T... chars>
constexpr TranslatableStringT<chars...> operator""_ntr() { return {}; }

/// This just marks the text to be translated (extracted by gettext tools). Does not alter its content.
#define N_(String) String##_ntr

#define textdomain(Domain)
#define bindtextdomain(Package, Directory)
