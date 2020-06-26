#pragma once

/// Translator classes and interfaces
/// Basically we need something, that converts an input string into some other (translated) string
/// There may be several types of translators, mostly based on where the sources are.

#include "string_view_utf8.hpp"

// predpokladam nejaka factory, kde budou momentalne 2:
// no-translator - simple implementace, vraci totez
// cpuflash translator - bude spravovat nejakou oblast v pameti, kde bude vyhledavaci strom a bude to schopne prekladat
// vstupni retezec const char * na vystupni retezec const uin8_t

// jeste to bude chtit nejake data accessory, protoze to je neco, co de-facto ted implementuji ty ruzne string_view_utf8 classy, ale je to v podstate nezajima.
// factory se bude indexovat aktualne vybranym jazykem, coz bude adresarova cesta:
// nullptr nebo prazdna cesta nebo nenalezeno -> default vraci vstupni string
// /cpuflash/cs.mo
// /spiflash/cesta na filesystemu
// /usbflash/cesta na filesystemu
// /nevimco jeste
// podle toho se najde prekladac a ten provede prozkoumani daneho resource a vrati string view, ktere nad tim resource bude fungovat
// V pripade pomalosti nekterych zdroju dat se treba muze udelat nejaka cache v pameti treba s poslednimi 10 stringy
// (a klasicky vyrazovaci zpusob - jakmile se string pouzije, posune se o 1 nahoru)
//
// Nad tim celym bude sedet funkce:
extern string_view_utf8 gettext(const char *src);

// ta return value bude zasadni problem
// zrejme budu muset vracet nejakej union, kde budou vsechny podporovany zdroje dat a nejakym zpusobem se mezi nimi bude rozhodovat, kterej plati
// podstatny je, ze se za kazdou cenu chci vyhnout dynamicky alokaci ... zatimco kopie 10B na stack me nenasere
// neco na zpusob boost/std::any
// Potom uvnitr te classy udelat de-facto rozskok jakoby na virtualni metody - slozitost bude stejna, jen to bude rucne
// Klidne se ty pointery na ty 2 (slovy DVE) funkce muzou inicializovat pri vyrobe toho objektu, at je to rovnou spravne a nemusi se uvnitr delat rozskok.
