#include "translator.hpp"
#include "translation_provider_empty.hpp"

string_view_utf8 gettext(const char *src) {
    return Translations::Instance().CurrentProvider()->GetText(src);
}

/// Just to have a meaningfull translation provider if there are no real translations available
/// ... or at least until there some translations available
static const EmptyTranslationProvider emptyProvider;

namespace {
// register the emptyProvider as "translator" for EN language ... i.e. return the source string intact
ProviderRegistrator prEN("en", &emptyProvider);
}

// this explicit initialization of currentProvider is here to make sure the pointer is always initialized
// even if there were no registered languages at all
Translations::Translations()
    : currentProvider(&emptyProvider) {
}

bool Translations::RegisterProvider(uint16_t langCode, const ITranslationProvider *provider) {
    // find the lang code in the table, update the provider
    auto i = std::find_if(translations.begin(), translations.end(), [langCode](const TranRec &r) { return r.langCode == langCode; });
    if (i == translations.end()) {
        // could not find the lang code key - yet - find a free spot to place it into
        i = std::find_if(translations.begin(), translations.end(), [](const TranRec &r) { return r.langCode == 0; });
        if (i == translations.end()) {
            // now we are out of space - couldn't register the provider anywhere
            return false;
        }
        i->langCode = langCode;
        // fallthrough and save also the provider when going out of this block
    }
    i->provider = provider;
    return true;
}

bool Translations::ChangeLanguage(uint16_t langCode) {
    // set current provider pointer to the one specified by lang code
    auto i = std::find_if(translations.cbegin(), translations.cend(), [langCode](const TranRec &r) { return r.langCode == langCode; });
    if (i != translations.cend()) {
        currentProvider = i->provider;
        return true;
    }
    return false;
}

bool Translations::LangExists(uint16_t langCode) const {
    auto i = std::find_if(translations.cbegin(), translations.cend(), [langCode](const TranRec &r) { return r.langCode == langCode; });
    return i != translations.cend();
}
