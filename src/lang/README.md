# Localization and translations

A good introductory tutorial is at
https://www.labri.fr/perso/fleury/posts/programming/a-quick-gettext-tutorial.html

## Processing pipeline description

Lets create the Prusa-Firmware-Buddy.pot file.

Since the xgettext isn't too good at iterating over a directory subtree, we must use find + parallel (I don't like xargs)

```bash
touch src/lang/po/Prusa-Firmware-Buddy.pot
find src -regextype posix-extended -regex "^.*\.c$|^.*\.cpp$|^.*\.h$|^.*\.hpp$" \
 | parallel -j1 xgettext --keyword=_ --keyword=N_ --language=C --package-name=Prusa-Firmware-Buddy \
 --package-version=4.1 --msgid-bugs-address=info@prusa3d.com --add-comments -j --sort-output -o src/lang/po/Prusa-Firmware-Buddy.pot {}
sed -i "s/SOME DESCRIPTIVE TITLE./Prusa-Firmware-Buddy/g" src/lang/po/Prusa-Firmware-Buddy.pot
sed -i "s/YEAR THE PACKAGE'S COPYRIGHT HOLDER/2020 Prusa Research/g" src/lang/po/Prusa-Firmware-Buddy.pot
sed -i "s/FIRST AUTHOR <EMAIL@ADDRESS>, YEAR./Prusa Research <info@prusa3d.com>, 2020/g" src/lang/po/Prusa-Firmware-Buddy.pot
```

Please note:
* We have 2 keywords to extract.
* For xgettext the target directory must exist. That applies to the following tools too
* xgettext is extremely rough on edge cases, even the Prusa-Firmware-Buddy.pot must exist prior to doing the "join" operation
* xgettext screws up the POT header every time, it must be manually updated afterwards (haven't found a way around it yet):

So, now, we have full template to start with, we can start to work on a translation for this program.
Lets start with a EN translation of it.
We need to create a file `src/lang/po/en/Prusa-Firmware-Buddy_en.po` extracted from the template.

```bash
msginit --input=src/lang/po/Prusa-Firmware-Buddy.pot --locale=en --output=src/lang/po/en/Prusa-Firmware-Buddy_en.po
```

Here it is necessary to take the `Prusa-Firmware-Buddy.pot` and Prusa-Firmware-Buddy_en.po
and send it to the content team to obtain translations for the rest of the languanges
They will send back `Prusa-Firmware-Buddy_{cs|de|es|fr|it|pl}.po` files, which are to be saved into corresponding subdirs in `src/lang/po/`

Normally, the last step, before installing the software, is to generate the .mo file:
```bash
msgfmt --output-file=po/cs/main.mo po/cs/main.po
```

However, in our case we have the translations converted into binary form and compiled into the FW statically
This is being done in the translator unit test (along with thorough checking of the whole infrastructure)
