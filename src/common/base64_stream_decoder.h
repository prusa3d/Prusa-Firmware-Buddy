#pragma once

#include <stdint.h>
#include <stddef.h>

// dekodovani 3 bajtu ze 4 base64 znaku - jelikoz mame 80 znaku na radek
// a 2 znaky jsou uvodni, tak 78/4 rozhodne neni cely cislo, takze to bude
// pretejkat Stavy funkce muzou bejt nasledujici: 0 podarilo se zdekodovat
// vsechny 3 bajty, tj. vstupem byl dost dlouhej retezec 1 podarilo se
// zdekodovat 2 bajty a nic nezbylo 2 podarilo se zdekodovat 2 bajty a neco
// zbylo, ceka se na dalsi radek (nejslozitejsi pripad) 3 podarilo se zdekodovat
// 1 bajt a nic nezbylo -1 jiny pruser, chyba dekodovani/dat Idealne tohle
// udelat jako objekt, ktery bude dostavat vstupni znaky a obcas z nej vypadne
// nejakej bajt proste aby si to pamatovalo svuj stav i mezi volanima... Pak by
// byla funkce ConsumeChar, ktera by vracela true/false jako mam bajt/nemam bajt
class Base64StreamDecoder {
public:
    constexpr inline Base64StreamDecoder()
        : state(States::AwaitingFirst) {
    }

    inline static uint8_t find64(uint8_t base64c) {
        return base64_inverse[base64c];
    }

    inline void Reset() {
        state = States::AwaitingFirst;
    }

    // @param c znak z base64 retezce
    // @param out sem se pripadne zapise zdekodovany bajt
    // @return 1 pokud je vystupni bajt hotovy a platny
    //         0 pokud na vystup nic nejde
    //         -1 pokud se posralo dekodovani vstupniho znaku
    int ConsumeChar(char c, uint8_t *out);

private:
    uint8_t lastbits = 0;
    enum class States : uint8_t {
        AwaitingFirst, // ocekavam prvni znak, nemam nic
        FirstByteOut, // mam prvni znak, a prisel mi 2. znak - vracim 1. bajt,
        // ukladam si zbyly 4 bity pro dalsi pouziti
        SecondByteOut, // mam 2. znak a prisel mi 3. znak, pouziju z nej 4 bity,
        // slozim s predchozimi 4 bity, vracim 1 bajt, zustavaji
        // mi 2 bity pro dalsi pouziti
        ThirdByteOut // mam 3. znak a prisel mi 4. znak, slozim s predchozimi 2
        // bity a vracim 1 bajt, prechazim do stavu 0
    };
    States state;

    // Inverse base64 lookup table
    // generated programmatically
    // for(size_t i = 0; i <= 255; ++i){
    //	 cout << hex << "0x" << (int)( is_base64(i) ? find64((uint8_t)i) : 0xff
    //) << ", ";
    // }
    static const uint8_t base64_inverse[256];
};
