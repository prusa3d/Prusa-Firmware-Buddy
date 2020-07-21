#pragma once

#include "translation_provider.hpp"

/// The simplest usable translation provider - only takes the input text
/// and returns it in a form of string_view_utf8
class EmptyTranslationProvider : public ITranslationProvider {
public:
    virtual string_view_utf8 GetText(const char *src) const {
        return string_view_utf8::MakeCPUFLASH((const uint8_t *)src);
    }
};
