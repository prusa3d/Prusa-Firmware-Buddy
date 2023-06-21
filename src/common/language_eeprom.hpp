#pragma once
#include <stdint.h>
#include <array>

/* class Lang {
private:
    std::array<char, 2> data;

public:
    Lang(uint16_t encoded);
    uint16_t Encode() const;
    bool operator==(const Lang &rhs) const;
    bool operator!=(const Lang &rhs) const;
}; */

class LangEEPROM {
public:
    inline static LangEEPROM &getInstance() {
        static LangEEPROM lang_prom;
        return lang_prom;
    }
    LangEEPROM(const LangEEPROM &) = delete;
    LangEEPROM &operator=(const LangEEPROM &) = delete;

    void setLanguage(uint16_t lang);
    void saveLanguage();
    uint16_t getLanguage() const;
    std::array<char, 2> getLanguageChar();
    bool IsValid() const;

private:
    LangEEPROM();
    ~LangEEPROM() {};

    uint16_t _language;
};
