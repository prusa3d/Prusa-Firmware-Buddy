#include <stdint.h>
#include "eeprom.h"

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
    uint16_t getLanguage();
    const char *getLanguageChar();

private:
    LangEEPROM();
    ~LangEEPROM() {};

    uint16_t _language;
};
