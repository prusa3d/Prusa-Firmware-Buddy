#include "translator.hpp"
#include "translation_provider_empty.hpp"

string_view_utf8 gettext(const char *src) {
    if (auto f = Translations::Instance().gettext_hook) {
        f(src);
    }
    return Translations::Instance().CurrentProvider()->GetText(src);
}

namespace {
// register the emptyProvider as "translator" for EN language ... i.e. return the source string intact
ProviderRegistrator prEN("en", EmptyTranslationProvider::Instance());
} // namespace

// this explicit initialization of currentProvider is here to make sure the pointer is always initialized
// even if there were no registered languages at all
Translations::Translations()
    : currentProvider(EmptyTranslationProvider::Instance()) {
    // this is a safety precaution - initialize the array with valid pointers even if the cells are empty
    translations.fill(TranRec({ currentProvider, 0U }));
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
    if (langCode == 0) {
        return false; // requesting an invalid langCode to change to
    }
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
