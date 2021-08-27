#pragma once

// bod 2 - uchodit parser na g-code (bohuzel nemuzu pouzit boost::spirit, to by
// me Robert asi sezral :) ) ale ono to nebude tak strasne - jde o to, ze se
// musi cist radek zacinajici na strednik, pak vyignorovat mezery a pak jen
// nacist radek az do konce Problem je ale v tom, ze ne kazdej comment je
// obrazek, cili nejdriv musim ve fajlu najit nasledujici: ; thumbnail begin
// 220x165 25968 ma to u sebe velikost v pixelech a dylku v bajtech pak to bez
// preruseni (zatim!) obsahuje base64 definici a konci to: ; thumbnail end takze
// se bude cist po radkach a bude se hledat radek, ktery zacina na "; thumbnail"
// pak se zkontroluje, jestli je to spravna velikost (protoze jsou tam 3 a ja
// chci ten prostredni) Navic jsem nucen pouzivat ten hnusnej FILE interface,
// protoze na Mini asi nic jinyho nebude

#include "../Marlin/src/libs/circularqueue.h"
#include "base64_stream_decoder.h"
#include <algorithm>
#include <stdio.h>
#include <string.h>

#include "ff.h"

// jeste budu asi potrebovat nacitac cele radky, pricemz musim dat pozor, aby se
// precetlo max 80 znaku a vse ostatni az do \n se zahodilo
struct SLine {
    static const size_t MAX = 80;
    uint8_t size;
    size_t
        expectedLineSize; // ocekavana delka radky, ktera chtela byt nactena - u
    // base64 je nutne zkontrolovat, ze mi nic nevypadlo
    uint8_t l[MAX + 1]; // MAX+1 zamerne, abych mohl mit posledni bajt 0
    SLine() {
        Reset();
    }
    void AppendByte(uint8_t b) {
        if (size < MAX) {
            l[size++] = b;
            l[size] = 0; // prepare termination
        }
        ++expectedLineSize;
    }
    inline void Reset() {
        size = 0;
        expectedLineSize = 0;
        l[0] = l[MAX] = 0;
    }
    operator const char *() const {
        return (const char *)(l);
    }

    bool IsBeginThumbnail() const;
    bool IsEndThumbnail() const;
};

// zamerne je to singleton, aby se vsude vynutilo, ze je opravdu jen jedna
// instance v celem programu Lze to pripadne predelat, ale je potreba myslet na
// to, ze dekodovaci automaty nejsou bezestavove a musi nekde svoje stavy
// pamatovat.
class GCodeThumbDecoder {
    GCodeThumbDecoder() {
    }

    Base64StreamDecoder base64SD;

    // dale budu potrebovat jednoduchy kruhovy buffer, kam se zdekoduje base64
    // radka radka ma max 80 znaku, z cehoz je 77 platnych base64 Jelikoz base64
    // znamena, ze se vezme 3x8 bitu, spoji se do 24bitu a z toho se udela 4x6
    // bitu, tak je to redukce 4:3 jestli dobre pocitam cili 77/4*3 ~ 58B
    // pricemz neco muze pribyt z predchoziho radku ... rekneme 60B bude kruhovy
    // buffer, akorat to budu chtit na 2 radky, abych mohl navazovat (da se
    // velikost optimalizovat)
    typedef CircularQueue<uint8_t, 128> TBytesQueue;
    TBytesQueue bytesQ;

    // tohle nacte jeden bajt z fajlu
    bool ReadByte(FILE *f, uint8_t &byte);

    // @return false, pokud se doslo na konec fajlu
    // pritom muze byt neco v line nactene, to je normalni
    bool ReadLine(FILE *f, SLine &line);

    bool AppendBase64Chars(const char *src, TBytesQueue &bytesQ);

    // tak, tady uz nastava problem - idealne bych mel vracet jednotlivy bajty
    // (to jeste souvisi s png interfacem) takze parsovat celou lajnu se mi moc
    // nelibi bude potreba ten dekoder predelat tak, aby nekoncil pripadne s
    // chybou, ale nechal si par bajtu pretyct do dalsiho volani cili to bude
    // muset bejt automat... Idealne bych to cely jakoby potreboval otocit - png
    // vola void user_read_data(png_structp png_ptr, png_bytep data, png_size_t
    // length); a dale mozna chybova a warningova funkce: void
    // user_error_fn(png_structp png_ptr, png_const_charp error_msg); void
    // user_warning_fn(png_structp png_ptr, png_const_charp warning_msg); Takze,
    // png ocekava nacteni daneho poctu bajtu do sveho bufferu, cili uplne cteni
    // cele radky mozna nebude potreba bude stacit, kdyz dokazu precist stabilne
    // 3 bajty - protoze to je max. blok, ktery mi base64 decoder da.

    bool LineIsBase64(const char *l, TBytesQueue &bytesQ) {
        // lajna musi zacinat '; ' a pak tam musi byt base64 znaky
        // pokud to bude cokoli jineho, vracim false a error
        if (l[0] != ';' || l[1] != ' ')
            return false;
        // tohle se vola pote, co se zkontroluje, jestli dana lajna neni
        // thumbnail end, takze se tim uz nemusim zabyvat
        return AppendBase64Chars(l + 2, bytesQ);
    }

    // tohle bude hledat ve fajlu komentare ve formatu ; thumbnail atd.
    // ted je to hodne podobny tomu, jak ma vypadat png read funkce
    // dostane pointer na buffer a pocet bajtu, ktere ma nacist (coz musi
    // splnit)
    // void user_read_data(FILE *f, uint8_t *data, size_t length){

    // tak jeste jinak
    // read pro fatfs vypada takto:
    // int _f_fread(struct _reent* _r, void* pv, char* pc, int n)
    //{
    //	UINT br = 0;
    //	f_read(&fil, pc, n, &br);
    //	return br;
    //}
    // nelze pouzit normalni libc read, protoze to zas neni FILE*
    // pv me asi nezajima, nevim, co to je
    // pc je buffer, do kteryho to cpu
    // n je pocet znaku, ktery chci nacist
    // vracim pocet nactenych znaku nebo -1, pokud pruser
    // jeste si sem naprasim file pointer...
    // _r je pointer na reentrant, cili na sFILE strukturu ... v mym pripade to
    // bude to FILE

    // Nekde venku budu potrebovat nejakej context, kde si uchovam stav automatu
    // stavy budou:
    // searching - hledam zacatek thumbnailu
    // base64 - ctu base64
    // end - neni nic dalsiho

    enum class States : uint8_t { Searching,
        Base64,
        End,
        Error };

    States state = States::Searching;

public:
    inline static GCodeThumbDecoder &Instance() {
        static GCodeThumbDecoder i;
        return i;
    }

    // idealne to udelat tak, ze bych se vubec nemusel zabejvat koncema radku -
    // normalni search automat, kterej proste ve fajlu najde spravnou uvodni
    // sekvenci

    int Read(FILE *f, char *pc, int n);

    void Reset() {
        // opakovani pokusu - cteni po vice bajtech
        // nutno resetovat automaty, coz je taky potreba vyresit
        state = States::Searching;
        base64SD.Reset();
        while (!bytesQ.isEmpty())
            bytesQ.dequeue();
    }
};
