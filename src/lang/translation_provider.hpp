#pragma once

#include "string_view_utf8.hpp"

/// A generic provider interface class for translations
/// Every language will have one (or more) providers.
/// Providers generally hide the internal implementation of getting a localized string for a given key
class ITranslationProvider {
public:
    /// This must be implemented by derived classes
    /// This is the actual search for localized texts
    virtual string_view_utf8 GetText(const char *) const = 0;
};
