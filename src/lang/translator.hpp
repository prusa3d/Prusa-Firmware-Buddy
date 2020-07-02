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
        ITranslationProvider *provider;
        uint16_t langCode;
    };
    static const size_t maxTranslations = 6;
    std::array<TranRec, maxTranslations> translations; // record of translations
    Translations();
    const ITranslationProvider *currentProvider;

public:
    static Translations Instance() {
        static Translations t;
        return t;
    }
    bool RegisterProvider(uint16_t langCode, ITranslationProvider *provider);
    bool ChangeLanguage(uint16_t langCode);
    inline const ITranslationProvider *CurrentProvider() const {
        return currentProvider;
    }
};
