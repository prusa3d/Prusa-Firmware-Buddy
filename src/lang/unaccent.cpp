#include "unaccent.hpp"
#include <algorithm>

using namespace std;

/// these are our utf8 characters so far
/// ¡ ¿ À Á È É Ê Í Ó Ú Ü  ß à á â ä  è é ê í ò ó ö  ú ü  ý Ą ą ć Č č Ę ę Ě ě Ł ł ń ň Ř ř ś Š š Ů ů ź ż ž
/// converted through unaccent:
/// ¡ ¿ A A E E E I O U U  ß a a a a  e e e i o o o  u u  y A a c C c E e E e L l n n R r s S s U u z z z
/// some of them we don't have in our fonts, so hand-tuning must be done
/// _ _ A A E E E I O U U ss a a a ae e e e i o o oe u ue y A a c C c E e E e L l n n R r s S s U u z z z
/// 2020-07-13 update
/// ¡ ¿ À Á È É Ê Í Ó Ú Ü ß  à á â ä  è é ê í ò ó ö  ú ü  ý Ą ą ć Č č Ę ę Ě ě Ł ł ń ň Ř ř Ś ś Š š Ů ů ź ż Ž ž
/// ¡ ¿ A A E E E I O U U ss a a a ae e e e i o o oe u ue y A a c C c E e E e L l n n R r S s S s U u z z Z z
/// 2020-07-17 update
/// ¡ ¿ À Á È É Ê Í Ó Ú Ü ß  à á â ä  è é ê í ò ó ö  ú ü  ý Ą ą ć Č č Ę ę Ě ě Ł ł ń ň Ř ř Ś ś Š š ť Ů ů ź ż Ž ž
/// ¡ ¿ A A E E E I O U U ss a a a ae e e e i o o oe u ue y A a c C c E e E e L l n n R r S s S s t U u z z Z z
/// 2020-07-28 update
/// ¡ ¿ À Á Ä È É Ê Í Ó Ú Ü Ý ß à á â ä è é ê í ò ó ö ú ü ý Ą ą ć Č č Ę ę Ě ě Ł ł ń ň Ř ř Ś ś Š š ť Ů ů ź ż Ž ž
/// ¡ ¿ A A A E E E I O U U Y s a a a a e e e i o o o u u y A a c C c E e E e L l n n R r S s S s t U u z z Z z
/// 2020-07-29 update
/// ¡ ¿ À Á Ä È É Ê Í Ó Ú Ü Ý ß à á â ä ç è é ê í ñ ò ó ö ú ü ý Ą ą ć Č č Ę ę Ě ě Ł ł ń ň Ř ř Ś ś Š š ť Ů ů ź ż Ž ž
/// ¡ ¿ A A A E E E I O U U Y s a a a a   e e e i   o o o u u y A a c C c E e E e L l n n R r S s S s t U u z z Z z
/// 2020-08-31 update
/// ¡ ¿ À Á Ä È É Ê Í Ó Ù Ú Ü Ý ß à á â ä ç è é ê í ñ ò ó ö ù ú ü ý Ą ą Ć ć Č č Ę ę Ě ě Ł ł ń ň Ř ř Ś ś Š š ť Ů ů ź ż Ž ž
/// ¡ ¿ À Á Ä È É Ê Í Ó   Ú Ü Ý ß à á â ä ç è é ê í ñ ò ó ö   ú ü ý Ą ą   ć Č č Ę ę Ě ě Ł ł ń ň Ř ř Ś ś Š š ť Ů ů ź ż Ž ž
/// 2020-11-04 update
/// ¡ ¿ À Á Ä È É Ê Í Ó   Ú Ü Ý ß à á â ä ç è é ê í ñ ò ó ö   ú ü ý Ą ą   ć Č č Ę ę Ě ě Ł ł   ń ň Ř ř Ś ś Š š ť Ů ů ź ż Ž ž
/// ¡ ¿ À Á   È É Ê Í Ó Ù Ú Ü Ý ß à á â ä   è é ê í ñ ò ó ö ù ú ü ý Ą ą Ć ć Č č Ę ę Ě ě Ł ł Ń ń ň Ř ř Ś ś Š š ť Ů ů ź ż   ž‬
const UnaccentTable::Item UnaccentTable::table[] = {
    { 0xa1, 1, "!" },  // ¡
    { 0xbf, 1, "?" },  // ¿
    { 0xc0, 1, "A" },  // À
    { 0xc1, 1, "A" },  // Á
    { 0xc4, 1, "A" },  // Ä
    { 0xc8, 1, "E" },  // È
    { 0xc9, 1, "E" },  // É
    { 0xca, 1, "E" },  // Ê
    { 0xcd, 1, "I" },  // Í
    { 0xd3, 1, "O" },  // Ó
    { 0xd9, 1, "U" },  // Ù
    { 0xda, 1, "U" },  // Ú
    { 0xdc, 1, "U" },  // Ü
    { 0xdd, 1, "Y" },  // Ý
    { 0xdf, 1, "s" },  // ß
    { 0xe0, 1, "a" },  // à
    { 0xe1, 1, "a" },  // á
    { 0xe2, 1, "a" },  // â
    { 0xe4, 1, "a" },  // ä
    { 0xe7, 1, "c" },  // ç
    { 0xe8, 1, "e" },  // è
    { 0xe9, 1, "e" },  // é
    { 0xea, 1, "e" },  // ê
    { 0xed, 1, "i" },  // í
    { 0xf1, 1, "n" },  // ñ
    { 0xf2, 1, "o" },  // ò
    { 0xf3, 1, "o" },  // ó
    { 0xf6, 1, "o" },  // ö
    { 0xf9, 1, "u" },  // ù
    { 0xfa, 1, "u" },  // ú
    { 0xfc, 1, "u" },  // ü
    { 0xfd, 1, "y" },  // ý
    { 0x104, 1, "A" }, // Ą
    { 0x105, 1, "a" }, // ą
    { 0x106, 1, "C" }, // Ć
    { 0x107, 1, "c" }, // ć
    { 0x10c, 1, "C" }, // Č
    { 0x10d, 1, "c" }, // č
    { 0x118, 1, "E" }, // Ę
    { 0x119, 1, "e" }, // ę
    { 0x11a, 1, "E" }, // Ě
    { 0x11b, 1, "e" }, // ě
    { 0x141, 1, "L" }, // Ł
    { 0x142, 1, "l" }, // ł
    { 0x143, 1, "N" }, // Ń
    { 0x144, 1, "n" }, // ń
    { 0x148, 1, "n" }, // ň
    { 0x158, 1, "R" }, // Ř
    { 0x159, 1, "r" }, // ř
    { 0x15a, 1, "S" }, // Ś
    { 0x15b, 1, "s" }, // ś
    { 0x160, 1, "S" }, // Š
    { 0x161, 1, "s" }, // š
    { 0x165, 1, "t" }, // ť
    { 0x16e, 1, "U" }, // Ů
    { 0x16f, 1, "u" }, // ů
    { 0x17a, 1, "z" }, // ź
    { 0x17c, 1, "z" }, // ż
    { 0x17d, 1, "Z" }, // Ž
    { 0x17e, 1, "z" }, // ž
    { 0xffff, 1, "?" } // generic fallback character
};

const UnaccentTable::Item &UnaccentTable::Utf8RemoveAccents(unichar c) {
    constexpr size_t tableSize = sizeof(table) / sizeof(Item);
    const Item *tableEnd = table + tableSize;
    const Item *i = lower_bound(table, tableEnd, c, [](const Item &s, unichar c) {
        return s.key < c;
    });
    if (i == tableEnd) {
        return table[tableSize - 1];
    }
    if (i->key != c) {
        return table[tableSize - 1];
    }
    return *i;
}
