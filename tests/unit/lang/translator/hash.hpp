#pragma once

#include "translation_provider_CPUFLASH.hpp"
#include <deque>
#include <string>

bool FillHashTableCPUFLASHProvider(CPUFLASHTranslationProviderBase::SHashTable &ht, const char *fname, std::deque<std::string> &rawStrings);

// I'd love to use boost::replace_all instead of this :(
void FindAndReplaceAll(std::string &data, std::string toSearch, std::string replaceStr);

void PreprocessRawLineStrings(std::string &l);
