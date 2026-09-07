/// @file i18n.h
/// stub for tests
/// all texts are translated as nullstr

#pragma once
#include "../../../../src/lang/string_view_utf8.hpp"

#define _(String)  string_view_utf8::MakeNULLSTR()
#define N_(String) String
