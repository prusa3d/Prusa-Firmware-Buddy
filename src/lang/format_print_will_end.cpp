#include "format_print_will_end.hpp"
#include "i18n.h"

// chci, aby fungovalo neco jako:
// strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "Today at %H:%M?", &print_end); //@@TODO translate somehow
// strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%a at %H:%MM", &print_end);
// strftime(pw->text_etime.data(), MAX_END_TIMESTAMP_SIZE, "%m-%d at %H:%MM", &print_end);

// v podstate jde o situace:
// Dnes v H:M
// Den v H:M
// datum v H:M
// Kdyz uz se to pise na cisto, je treba zohlednit slusne receno lokalni specifika
// nebo se na ne aspon pripravit:
// - format datumu
// - 12/24h

size_t FormatMsgPrintWillEnd::AppendStrftime(char *ptr, size_t maxsize, const tm *timeptr, bool hours24) {
    if (hours24) {
        return snprintf(ptr, maxsize, " %02d:%02d", timeptr->tm_hour, timeptr->tm_min);
    } else {
        return strftime(ptr, maxsize, " %I:%M %p", timeptr);
    }
}

// cele se to vyrenderuje v utf8 do predaneho bufferu, ktery musi byt dost velky.
size_t FormatMsgPrintWillEnd::Today(char *ptr, size_t maxsize, const tm *timeptr, bool hours24) {
    // Used in "The print will end:", keep it up to 10 characters
    // here the "at" will be inconsistent with FormatMsgPrintWillEndOnDay_i18n @@TODO
    string_view_utf8 day = _("Today at");
    size_t bytesCopied = day.copyToRAM(ptr, maxsize);
    ptr += bytesCopied;

    bytesCopied += AppendStrftime(ptr, maxsize - bytesCopied, timeptr, hours24);

    return bytesCopied;
}

size_t FormatMsgPrintWillEnd::DayOfWeek(char *ptr, size_t maxsize, const tm *timeptr, bool hours24) {
    static const char *dayCodes[7] = {
        // abbreviated Sunday - max 3 characters
        N_("Sun"),
        // abbreviated Monday - max 3 characters
        N_("Mon"),
        // abbreviated Tuesday - max 3 characters
        N_("Tue"),
        // abbreviated Wednesday - max 3 characters
        N_("Wed"),
        // abbreviated Thursday - max 3 characters
        N_("Thr"),
        // abbreviated Friday - max 3 characters
        N_("Fri"),
        // abbreviated Saturday - max 3 characters
        N_("Sat")
    };

    string_view_utf8 day = _(dayCodes[timeptr->tm_wday]);
    size_t bytesCopied = day.copyToRAM(ptr, maxsize);
    ptr += bytesCopied;

    bytesCopied += AppendStrftime(ptr, maxsize - bytesCopied, timeptr, hours24);

    return bytesCopied;
}

size_t FormatMsgPrintWillEnd::Date(char *ptr, size_t maxsize, const tm *timeptr, bool hours24, DateFormat dateFormat) {
    size_t bytesCopied = 0;
    switch (dateFormat) {
    case ISO: // ISO
        bytesCopied = snprintf(ptr, maxsize, "%02d-%02d", timeptr->tm_mon + 1, timeptr->tm_mday);
        break;
    case CS: // cs
        bytesCopied = snprintf(ptr, maxsize, "%d.%d.", timeptr->tm_mday, timeptr->tm_mon + 1);
        break;
    case MD: // m/d
        bytesCopied = snprintf(ptr, maxsize, "%02d/%02d", timeptr->tm_mon + 1, timeptr->tm_mday);
        break;
    case DM: // d/m
        bytesCopied = snprintf(ptr, maxsize, "%02d/%02d", timeptr->tm_mday, timeptr->tm_mon + 1);
        break;
    }
    ptr += bytesCopied;

    bytesCopied += AppendStrftime(ptr, maxsize - bytesCopied, timeptr, hours24);

    return bytesCopied;
}
