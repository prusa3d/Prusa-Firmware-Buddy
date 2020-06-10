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
/*
$ touch src/lang/po/Prusa-Firmware-Buddy.pot
$ find src -regextype posix-extended -regex "^.*\.c$|^.*\.cpp$|^.*\.h$|^.*\.hpp$" \
 | parallel -j1 xgettext --keyword=_ --keyword=N_ --language=C --package-name=Prusa-Firmware-Buddy \
 --package-version=4.1 --msgid-bugs-address=info@prusa3d.com --add-comments -j --sort-output -o src/lang/po/Prusa-Firmware-Buddy.pot {}
$ sed -i "s/SOME DESCRIPTIVE TITLE./Prusa-Firmware-Buddy/g" src/lang/po/Prusa-Firmware-Buddy.pot
$ sed -i "s/YEAR THE PACKAGE'S COPYRIGHT HOLDER/2020 Prusa Research/g" src/lang/po/Prusa-Firmware-Buddy.pot
$ sed -i "s/FIRST AUTHOR <EMAIL@ADDRESS>, YEAR./Prusa Research <info@prusa3d.com>, 2020/g" src/lang/po/Prusa-Firmware-Buddy.pot
*/
// Please note:
// - We have 2 keywords to extract.
// - For xgettext the target directory must exist. That applies to the following tools too
// - xgettext is extremely rough on edge cases, even the Prusa-Firmware-Buddy.pot must exist prior to doing the "join" operation
// - xgettext screws up the POT header every time, it must be manually updated afterwards (haven't found a way around it yet):
// - keep the above comment in C-style to prevent gcc warning: multi-line comment [-Wcomment] - there are intentionally backslashes
//   to make the long find... statement readable
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
