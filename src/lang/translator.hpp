#pragma once

/// Translator classes and interfaces
/// Basically we need something, that converts an input string into some other (translated) string
/// There may be several types of translators, mostly based on where the sources are.

#include "string_view_utf8.hpp"

extern string_view_utf8 gettext(const char *src);
