#include "gcode_thumb_decoder.h"

bool SLine::IsBeginThumbnail() const {
    static const char thumbnailBegin[] = "; thumbnail begin "; // pozor na tu mezeru na konci
    // pokud zacina radka na ; thumbnail, lze se tim zacit zabyvat
    // nemuzu pouzivat zadne pokrocile algoritmy, musim vystacit se strcmp
    const char *lc = (const char *)l; // jen quli debuggeru, abych do toho videl...
    // ta -1 na size ma svuj vyznam - chci, aby strncmp NEporovnavalo ten null
    // znak na konci, cili abych se nemusel srat s tim, ze vstupni string je
    // delsi, cili aby to emulovalo chovani boost::starts_with()
    if (!strncmp(lc, thumbnailBegin,
            std::min(sizeof(l), sizeof(thumbnailBegin)) - 1)) {
        // zacatek thumbnailu
        unsigned int x, y;
        unsigned long bytes;
        lc = lc + sizeof(thumbnailBegin) - 1;
        int ss = sscanf(lc, "%ux%u %lu", &x, &y, &bytes);
        if (ss == 3) { // 3 uspesne prectene itemy - rozliseni
            // je to platny zacatek thumbnailu, je to ten muj?
            if (x == 220 && y == 124) {
                // je to ten muj, ktery chci
                return true;
            }
        }
    }
    return false;
}

bool SLine::IsEndThumbnail() const {
    static const char thumbnailEnd[] = "; thumbnail end";
    // proc -1 viz. vysvetleni v LineIsBeginThumbnail
    return !strncmp((const char *)l, thumbnailEnd,
        std::min(sizeof(l), sizeof(thumbnailEnd)) - 1);
}

bool GCodeThumbDecoder::ReadByte(FILE *f, uint8_t &byte) {
    return fread(&byte, 1, 1, f) == 1;
}

bool GCodeThumbDecoder::ReadLine(FILE *f, SLine &line) {
    uint8_t byte;
    for (;;) {
        if (feof(f) || !ReadByte(f, byte))
            return false;
        if (byte == '\n')
            break;
        line.AppendByte(byte);
    }
    return true;
}

bool GCodeThumbDecoder::AppendBase64Chars(
    const char *src, GCodeThumbDecoder::TBytesQueue &bytesQ) {
    // a tohle uz je i s dekodovanim
    while (*src) {
        uint8_t bajt;
        switch (base64SD.ConsumeChar(*src++, &bajt)) {
        case 1:
            bytesQ.enqueue(bajt);
            break;
        case -1:
            return false; // chyba v dekodovani
        default:
            break; // zadny bajt na vystup
        }
    }
    return true;
}

int GCodeThumbDecoder::Read(char *pc, int n) {
    static const size_t MAX_READ_LINES = 2048; // treba 2K radek
    switch (state) {
    case States::Searching: {
        // ctu radky, dokud nenarazim na begin thumbnail nebo neprelezu max
        // ctenou velikost (radove asi 64KB) nebo neskonci fajl
        for (size_t lines = 0; lines < MAX_READ_LINES; ++lines) {
            SLine l;
            if (!ReadLine(f, l)) {
                // tahle podminka nevypada na prvni pohled uplne spravne, ale je
                // treba si uvedomit, ze hledany thumbnail rozhodne neni na
                // konci fajlu, takze lze s klidem skoncit bez ocheckovani
                // posledni nactene radky z fajlu Podobna domain-specific
                // prasecina je tu vickrat
                state = States::Error;
                break; // konec souboru
            }
            if (l.IsBeginThumbnail()) {
                state = States::Base64;
                break; // nalezen png meho rozmeru, budu cist jeho data
            }
        }
        if (state != States::Base64) {
            // nenalezen png, konec fajlu nebo hloubky hledani
            state = States::Error;
            return -1;
        }
    }
    // [[fallthrough]];  // here is no break intentionally
    case States::Base64: {
        int i = 0; // spravne by to melo byt unsigned, ale chci eliminovat
                   // warning i == n, pricemz n je definitoricky int
        for (;;) {
            // tady nemusim cist po radkach, akorat pak bych musel naprasit
            // automat na hledani ; thumbnail end, coz se mi nechce nicmene ten
            // vysledek musim prehodit do nejakyho zpracovanyho pole
            if (bytesQ.isEmpty()) {
                SLine l;
                // nutno nacist dalsi line
                if (!ReadLine(f, l)) {
                    state = States::Error; // rozbite, predcasny konec fajlu
                    return -1;
                }
                // kontrola, ze se precetla cela radka a nic se z toho
                // nezahodilo v rezimu cteni base64 je to dulezite, nemuzu nic
                // zahodit, rozbil bych si data
                if (l.expectedLineSize > l.size) {
                    state = States::Error; // rozbite, moc dlouha radka
                    return -1;
                }
                if (l.IsEndThumbnail()) {
                    state = States::End; // platny konec thumbnailu
                    return i;            // uz je EOF, nemam dalsi data, ale automat konci
                                         // spravnym koncem
                                         // vracim, kolik jsem dosud nactetl
                } else if (LineIsBase64(l, bytesQ)) {
                    // line nactena v poradku a zdekodovana, stav base64
                    // zustava, ocekava se dalsi takova radka
                } else {
                    state = States::Error;
                    return -1;
                }
            }
            // tady zamerne NENI else
            if (!bytesQ.isEmpty()) {
                *(pc + i) = bytesQ.dequeue();
                ++i;
                if (i == n)
                    break;
            }
        }
        return i;
    }
    default: // ve vsech ostatnich stavech vratit -1, nemam data, ani kdybych se
             // rozkrajel
        return -1;
    }
}
