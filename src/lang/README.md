# GETTEXT
A good introductory tutorial for GETTEXT is at
https://www.labri.fr/perso/fleury/posts/programming/a-quick-gettext-tutorial.html

Please note:
* We have 2 keywords to extract.
* For xgettext the target directory must exist. That applies to the following tools too
* xgettext is extremely rough on edge cases, even the Prusa-Firmware-Buddy.pot must exist prior to doing the "join" operation
* xgettext screws up the POT header every time, it must be manually updated afterwards (haven't found a way around it yet):

**For complete translation tutorial [click here](../../utils/translations_and_fonts/README_TRANSLATIONS.md)**
