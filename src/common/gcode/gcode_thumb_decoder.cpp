#include "gcode_thumb_decoder.h"

bool SLine::IsBeginThumbnail(uint16_t expected_width, uint16_t expected_height, bool allow_larder) const {
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
            if ((x == expected_width && y == expected_height) || (allow_larder && x >= expected_width && y >= expected_height)) {
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
    if (base64SD.has_value()) {
        // a tohle uz je i s dekodovanim
        while (*src) {
            uint8_t bajt;
            switch (base64SD->ConsumeChar(*src++, &bajt)) {
            case 1:
                bytesQ.enqueue(bajt);
                break;
            case -1:
                return false; // chyba v dekodovani
            default:
                break;        // zadny bajt na vystup
            }
        }
    } else {
        // Pass-through mode, we just extract the preview, but don't decode the
        // base64.
        while (*src) {
            bytesQ.enqueue(*src++);
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
            if (l.IsBeginThumbnail(expected_width, expected_height, allow_larger)) {
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
        [[fallthrough]];
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

#ifndef UNITTESTS // this hacky stuff doesn't work when compiling on x86 GCC, so skip this

static int gcode_thumb_read([[maybe_unused]] struct _reent *_r, [[maybe_unused]] void *pv, [[maybe_unused]] char *pc, [[maybe_unused]] int n) {
    GCodeThumbDecoder *gd = reinterpret_cast<GCodeThumbDecoder *>(pv);
    int count = gd->Read(pc, n);
    if (count < 0) {
        return 0;
    }
    return count;
}

static int gcode_thumb_write([[maybe_unused]] struct _reent *_r, [[maybe_unused]] void *pv, [[maybe_unused]] const char *pc, [[maybe_unused]] int n) {
    return 0;
}

static int gcode_thumb_close([[maybe_unused]] struct _reent *_r, [[maybe_unused]] void *pv) {
    return 0;
}

static _fpos_t gcode_thumb_seek([[maybe_unused]] struct _reent *_r, [[maybe_unused]] void *pv, [[maybe_unused]] _fpos_t fpos, [[maybe_unused]] int ipos) {
    return 0;
}

int f_gcode_thumb_open(GCodeThumbDecoder *gd, FILE *fp) {
    memset(fp, 0, sizeof(FILE));
    fp->_read = gcode_thumb_read;
    fp->_write = gcode_thumb_write;
    fp->_close = gcode_thumb_close;
    fp->_seek = gcode_thumb_seek;
    // we can use the cookie to pass any user-defined pointer/context to all of the I/O routines
    fp->_cookie = reinterpret_cast<void *>(gd);
    fp->_file = -1;
    fp->_flags = __SRD;
    fp->_lbfsize = 512;
    fp->_bf._base = (uint8_t *)malloc(fp->_lbfsize);
    fp->_bf._size = fp->_lbfsize;

    return 0;
}

int f_gcode_thumb_close(FILE *fp) {
    if (fp && fp->_bf._base) {
        free(fp->_bf._base);
    }
    return 0;
}

#endif
