#pragma once

/// Translator classes and interfaces
/// Basically we need something, that converts an input string into some other (translated) string
/// There may be several types of translators, mostly based on where the sources are.

#include "string_view_utf8.hpp"
#include "translation_provider.hpp"
#include <algorithm>
#include <array>

extern string_view_utf8 gettext(const char *src);

class Translations {
    struct TranRec {
        const ITranslationProvider *provider;
        uint16_t langCode;
    };
    static const size_t maxTranslations = 8;
    std::array<TranRec, maxTranslations> translations; // record of translations
    Translations();
    const ITranslationProvider *currentProvider;

public:
    static Translations &Instance() {
        static Translations t;
        return t;
    }
    bool RegisterProvider(uint16_t langCode, const ITranslationProvider *provider);
    bool ChangeLanguage(uint16_t langCode);
    inline const ITranslationProvider *CurrentProvider() const {
        return currentProvider;
    }
    /// @returns true if the Translations engine knows about the langCode translation
    /// (i.e. a provider is registered for that language)
    bool LangExists(uint16_t langCode) const;

    /// takes input 2-character string (e.g. "cs") and transforms it into numerical lang code
    /// ... basically copies the two chars into an uint16_t, but this function is here to make
    /// sure this operation is done in a same manner at all places necessary.
    /// Basically the compiler should emit at most the one instruction necessary for this function
    inline static uint16_t MakeLangCode(const char *langCode) {
        const uint8_t *lcu = reinterpret_cast<const uint8_t *>(langCode); // make sure it is unsigned so that shifting the bits below
        // does not cause signed bit extension
        return uint16_t(lcu[1] << 8) + lcu[0];
    }

    using GetTextHook = void (*)(const char *);
    GetTextHook gettext_hook = nullptr;
};

struct ProviderRegistrator {
    ProviderRegistrator(const char *langCode, const ITranslationProvider *provider) {
        Translations::Instance().RegisterProvider(Translations::MakeLangCode(langCode), provider);
    }
};
