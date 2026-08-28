/// @file

/// Ban a single symbol and give a reason. Works only on GNU compiler :(
#define BAN_SYMBOL(reason, symbol)                           \
    asm(".section .gnu.warning." #symbol ",\"\",%progbits\n" \
        ".ascii \"" #symbol "() is banned: " reason "\"\n"   \
        ".previous\n")

BAN_SYMBOL("use snprintf()", __itoa);
BAN_SYMBOL("use snprintf()", __utoa);
BAN_SYMBOL("use snprintf()", itoa);
BAN_SYMBOL("use snprintf()", utoa);
