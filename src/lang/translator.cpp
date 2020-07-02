#include "translator.hpp"

string_view_utf8 gettext(const char *src) {
    // the most simple implementation - return a string view over the source string
    //    return string_view_utf8::MakeCPUFLASH((const uint8_t *)src);
    return Translations::Instance().CurrentProvider()->GetText(src);
}

Translations::Translations() {
}

bool Translations::RegisterProvider(uint16_t langCode, ITranslationProvider *provider) {
    // find the lang code in the table, update the provider
    auto i = std::find_if(translations.begin(), translations.end(), [langCode](const TranRec &r) { return r.langCode == langCode; });
    if (i == translations.end()) {
        return false; // could not find the lang code key
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
