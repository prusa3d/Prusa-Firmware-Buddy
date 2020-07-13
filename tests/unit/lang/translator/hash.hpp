#pragma once

#include "translation_provider_CPUFLASH.hpp"
#include <deque>
#include <string>

bool FillHashTableCPUFLASHProvider(CPUFLASHTranslationProviderBase::SHashTable &ht, const char *fname, std::deque<std::string> &rawStrings);
