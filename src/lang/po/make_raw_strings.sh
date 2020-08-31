#!/bin/bash

#cat en/Prusa-Firmware-Buddy_en.po | grep msgid | cut -b 7- | sed "s@\"@@g" > keys.txt
#cat cs/Prusa-Firmware-Buddy_cs.po | grep msgstr | cut -b 8- | sed "s@\"@@g" > cs.txt
#cat de/Prusa-Firmware-Buddy_de.po | grep msgstr | cut -b 8- | sed "s@\"@@g" > de.txt
#cat es/Prusa-Firmware-Buddy_es.po | grep msgstr | cut -b 8- | sed "s@\"@@g" > es.txt
#cat fr/Prusa-Firmware-Buddy_fr.po | grep msgstr | cut -b 8- | sed "s@\"@@g" > fr.txt
#cat it/Prusa-Firmware-Buddy_it.po | grep msgstr | cut -b 8- | sed "s@\"@@g" > it.txt
#cat pl/Prusa-Firmware-Buddy_pl.po | grep msgstr | cut -b 8- | sed "s@\"@@g" > pl.txt

./POdump en/Prusa-Firmware-Buddy_en.po msgid  | tail -n +2 > keys.txt
./POdump cs/Prusa-Firmware-Buddy_cs.po msgstr | tail -n +2 > cs.txt
./POdump de/Prusa-Firmware-Buddy_de.po msgstr | tail -n +2 > de.txt
./POdump es/Prusa-Firmware-Buddy_es.po msgstr | tail -n +2 > es.txt
./POdump fr/Prusa-Firmware-Buddy_fr.po msgstr | tail -n +2 > fr.txt
./POdump it/Prusa-Firmware-Buddy_it.po msgstr | tail -n +2 > it.txt
./POdump pl/Prusa-Firmware-Buddy_pl.po msgstr | tail -n +2 > pl.txt

mv keys.txt ../../../tests/unit/lang/translator/
mv cs.txt ../../../tests/unit/lang/translator/
mv de.txt ../../../tests/unit/lang/translator/
mv es.txt ../../../tests/unit/lang/translator/
mv fr.txt ../../../tests/unit/lang/translator/
mv it.txt ../../../tests/unit/lang/translator/
mv pl.txt ../../../tests/unit/lang/translator/

cd ../../../build-tests/tests/unit/lang/translator/
./translator_tests

cd ../../../../../tests/unit/lang/translator/
./build.sh

cp hash_table_buckets.ipp        ../../../../src/lang/
cp hash_table_string_indices.ipp ../../../../src/lang/
mv stringBegins.cs.hpp ../../../../src/lang/
mv stringBegins.de.hpp ../../../../src/lang/
mv stringBegins.es.hpp ../../../../src/lang/
mv stringBegins.fr.hpp ../../../../src/lang/
mv stringBegins.it.hpp ../../../../src/lang/
mv stringBegins.pl.hpp ../../../../src/lang/

mv utf8Raw.cs.hpp ../../../../src/lang/
mv utf8Raw.de.hpp ../../../../src/lang/
mv utf8Raw.es.hpp ../../../../src/lang/
mv utf8Raw.fr.hpp ../../../../src/lang/
mv utf8Raw.it.hpp ../../../../src/lang/
mv utf8Raw.pl.hpp ../../../../src/lang/
