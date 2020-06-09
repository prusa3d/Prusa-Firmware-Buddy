#pragma once

// Based on recommendation from GNU/gettext
// prepare the sources for localization/internationalization (aka l10n/i18n)
#define _(String)  (String)
#define N_(String) String
#define textdomain(Domain)
#define bindtextdomain(Package, Directory)

// A good introductory tutorial is at:
// https://www.labri.fr/perso/fleury/posts/programming/a-quick-gettext-tutorial.html

// Processing pipeline description:
// Lets create the Prusa-Firmware-Buddy.pot file.
// Since the xgettext isn't too good at iterating over a directory subtree, we must use find + parallel (I don't like xargs)
// $ touch src/lang/po/Prusa-Firmware-Buddy.pot
//   find src -regextype posix-extended -regex "^.*\.c$|^.*\.cpp$|^.*\.h$|^.*\.hpp$" | parallel -j1 xgettext --keyword=_ --keyword=N_ --language=C --add-comments -j --sort-output -o src/lang/po/Prusa-Firmware-Buddy.pot {}
// Please note:
// - We have 2 keywords to extract.
// - For xgettext the target directory must exist. That applies to the following tools too
// - Due to the fact, that xgettext is extremely stupid, even the Prusa-Firmware-Buddy.pot must exist prior to doing the "join" operation
//
// So, now, we have full template to start with, we can start to work on a translation for this program.
// Lets start with a CZ translation of it.
// We need to create a file po/cs/main.po extracted from the template.
// $ msginit --input=po/Prusa-Firmware-Buddy.pot --locale=cs --output=po/cs/Prusa-Firmware-Buddy.po
//
// The last step, before installing the software, is to generate the .mo file:
// $ msgfmt --output-file=po/cs/main.mo po/cs/main.po
//
// ------------------------------
// Updating
// We first need to update the .pot file (as previously):
// $ xgettext --keyword=_ --language=C --add-comments --sort-output -o po/hello.pot hello.c
//
// Then, we merge it with the previous po/fr/hello.po file :
// $ msgmerge --update po/fr/hello.po po/hello.pot
//
// Finally, we write the translation of the new string in the .po file and build the .mo file as follow:
// $ msgfmt --output-file=po/fr/hello.mo po/fr/hello.po
