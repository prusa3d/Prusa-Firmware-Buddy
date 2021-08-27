/// str_utils tests

#include <string.h>
#include <iostream>
#include <cstdint>
#include <vector>

//#define CATCH_CONFIG_MAIN // This tells Catch to provide a main() - only do this in one cpp file
#include "catch2/catch.hpp"

using Catch::Matchers::Equals;

#include "common/str_utils.hpp"
#include "lang/string_view_utf8.hpp"

static const std::uint8_t n255 = 255;
static const std::uint16_t n511 = 511;

/// Replaces all occurrences of @find with @replace
void replace(char *str, char find, char replace) {
    while (str[0] != 0) {
        if (str[0] == find)
            str[0] = replace;
        str++;
    }
}

char replaceChar(char c, char find, char replace) {
    if (c == ' ')
        return replace;
    return c;
}

TEST_CASE("Delete string", "[strdel]") {
    static constexpr char text[12] = "abcdXYZefgh";
    char str[n255] = "abcdXYZefgh";
    char *nostr = nullptr;
    size_t n;

    SECTION("void input") {
        n = strdel(nostr);
        CHECK(n == 0);
    }

    SECTION("delete 0 chars") {
        n = strdel(str, 0);
        CHECK(n == 0);
        CHECK(0 == strcmp(str, text));
    }

    SECTION("empty string") {
        strcpy(str, "");
        n = strdel(str);
        CHECK(n == 0);
        CHECK(0 == strcmp(str, ""));
    }

    SECTION("single char at beginning") {
        n = strdel(str);
        CHECK(n == 1);
        CHECK(0 == strcmp(str, "bcdXYZefgh"));
    }

    SECTION("single char inside") {
        n = strdel(str + 4, 1);
        CHECK(n == 1);
        CHECK(0 == strcmp(str, "abcdYZefgh"));
    }

    SECTION("substring over the end") {
        n = strdel(str + 7, 5);
        CHECK(n == 4);
        CHECK(0 == strcmp(str, "abcdXYZ"));
    }

    SECTION("substring inside") {
        n = strdel(str + 7, 2);
        CHECK(n == 2);
        CHECK(0 == strcmp(str, "abcdXYZgh"));
    }
}

TEST_CASE("Insert string", "[strins]") {
    static constexpr char text[12] = "abcdXYZefgh";
    char str[n255] = "abcdXYZefgh";
    char *nostr = nullptr;
    int n;

    SECTION("void input 1") {
        n = strins(nostr, n255, str);
        CHECK(n < 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("void input 2") {
        n = strins(str, n255, nostr);
        CHECK(n < 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("insert a char 0 times") {
        n = strins(str, n255, "a", 0);
        CHECK(n == 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("empty string 1") {
        n = strins(str, n255, "");
        CHECK(n == 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("empty string 2") {
        char empty[n255] = "";
        n = strins(empty, n255, str);
        CHECK(n == int(strlen(empty)));
        CHECK(strlen(str) == strlen(empty));
        REQUIRE_THAT(empty, Equals(text));
    }

    SECTION("single char at the beginning") {
        n = strins(str, n255, "a");
        CHECK(n == 1);
        REQUIRE_THAT(str, Equals("aabcdXYZefgh"));
    }

    SECTION("single char inside") {
        n = strins(str + 4, n255 - 4, "a");
        CHECK(n == 1);
        REQUIRE_THAT(str, Equals("abcdaXYZefgh"));
    }

    SECTION("string at the beginning") {
        n = strins(str, n255, "ABCD");
        CHECK(n == 4);
        REQUIRE_THAT(str, Equals("ABCDabcdXYZefgh"));
    }

    SECTION("string at the end") {
        n = strins(str + strlen(str), n255 - strlen(str), "ABC");
        CHECK(n == 3);
        REQUIRE_THAT(str, Equals("abcdXYZefghABC"));
    }

    SECTION("insert more times") {
        n = strins(str, n255, "123 ", 3);
        CHECK(n == 12);
        REQUIRE_THAT(str, Equals("123 123 123 abcdXYZefgh"));
    }

    SECTION("insert too much") {
        n = strins(str, strlen(str) + 9, "123 ", 3);
        CHECK(n < 0);
    }
}

TEST_CASE("Shift string", "[strshift]") {
    static constexpr char text[12] = "abcdXYZefgh";
    char str[n255] = "abcdXYZefgh";
    char *nostr = nullptr;
    int n;

    SECTION("void input") {
        n = strshift(nostr, n255);
        CHECK(n < 0);
    }

    SECTION("by 0") {
        n = strshift(str, n255, 0);
        CHECK(n == 0);
        REQUIRE_THAT(str, Equals(text));
    }

    SECTION("at the beginning; short text, long distance") {
        size_t size = strlen(str);
        int shift = 2 * size;
        n = strshift(str, n255, shift);
        CHECK(n == shift);
        CHECK(strlen(str) == size + shift);
        REQUIRE_THAT(str + shift, Equals(text));
    }

    SECTION("at the beginning; long text, short distance") {
        size_t size = strlen(str);
        int shift = size / 2;
        n = strshift(str, n255, shift);
        CHECK(n == shift);
        CHECK(strlen(str) == size + shift);
        REQUIRE_THAT(str + shift, Equals(text));
    }

    SECTION("in the middle") {
        n = strshift(str + 3, n255 - 3, 3);
        CHECK(n == 3);
        REQUIRE_THAT(str + 3 + 3, Equals("dXYZefgh"));
    }

    SECTION("too much") {
        n = strshift(str, strlen(str) + 3, 3);
        CHECK(n < 0);
    }
}

TEST_CASE("String to multi-line", "[str2multiline]") {
    char short_text[n511] = "Lorem ipsum dolor sit amet";
    char long_text[n511] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit, sed do eiusmod tempor incididunt ";

    int n;
    size_t length;

    SECTION("short text, long line") {
        length = 15;
        n = str2multiline(short_text, n511, length);
        CHECK(n == 2);
        REQUIRE_THAT(short_text, Equals("Lorem ipsum\ndolor sit amet"));
    }

    SECTION("long text, long line") {
        length = 15;
        n = str2multiline(long_text, n511, length);
        CHECK(n == 7);
        REQUIRE_THAT(long_text, Equals("Lorem ipsum\ndolor sit amet,\nconsectetur\nadipiscing\nelit, sed do\neiusmod tempor\nincididunt "));
    }

    SECTION("short text, short line") {
        length = 8;
        n = str2multiline(short_text, n511, length);
        CHECK(n == 4);
        REQUIRE_THAT(short_text, Equals("Lorem\nipsum\ndolor\nsit amet"));
    }

    SECTION("long text, short line") {
        length = 8;
        n = str2multiline(long_text, n511, length);
        CHECK(n == 14);
        REQUIRE_THAT(long_text, Equals("Lorem\nipsum\ndolor\nsit\namet,\nconsecte\ntur\nadipisci\nng elit,\nsed do\neiusmod\ntempor\nincididu\nnt "));
    }

    SECTION("long text, very short line") {
        length = 4;
        n = str2multiline(long_text, n511, length);
        CHECK(n == 26);
        REQUIRE_THAT(long_text, Equals("Lore\nm\nipsu\nm\ndolo\nr\nsit\namet\n,\ncons\necte\ntur\nadip\nisci\nng\nelit\n,\nsed\ndo\neius\nmod\ntemp\nor\ninci\ndidu\nnt "));
    }

    SECTION("specific combination") {
        char str[n255] = "123 123 1234 1234";
        length = 3;
        n = str2multiline(str, n255, length);
        CHECK(n == 6);
        REQUIRE_THAT(str, Equals("123\n123\n123\n4\n123\n4"));
    }

    SECTION("long text, long line, forced new lines") {
        char str[n255] = "Lorem ipsum dolor sit\namet, consectetur adipiscing elit, sed\ndo eiusmod tempor incididunt ";
        length = 15;
        n = str2multiline(str, n255, length);
        CHECK(n == 9);
        REQUIRE_THAT(str, Equals("Lorem ipsum\ndolor sit\namet,\nconsectetur\nadipiscing\nelit, sed\ndo eiusmod\ntempor\nincididunt "));
    }

    SECTION("long text, shorter line, nonbreaking spaces") {
        char str[n511] = "Lorem ipsum dolor sit" NBSP "amet, consectetur adipiscing elit, sed" NBSP "do" NBSP "eiusmod tempor incididunt ";
        length = 12;
        n = str2multiline(str, n255, length);
        CHECK(n == 10);
        REQUIRE_THAT(str, Equals("Lorem ipsum\ndolor\nsit amet,\nconsectetur\nadipiscing\nelit,\nsed do\neiusmod\ntempor\nincididunt "));
    }

    SECTION("too small buffer") {
        n = str2multiline(short_text, strlen(short_text), 1);
        CHECK(n < 0);
    }

    SECTION("BFW-1125.1") {
        char str[n255] = "123\n456";
        n = str2multiline(str, n255, 3);
        CHECK(n == 2);
        REQUIRE_THAT(str, Equals("123\n456"));
    }

    // SECTION("BFW-1125.2") {
    //     char str[n255] = "The status bar is at\n"
    //                      "the bottom of the  \n"
    //                      "screen. It contains\n"
    //                      "information about: \n"
    //                      " - Nozzle temp.    \n"
    //                      " - Heatbed temp.   \n"
    //                      " - Printing speed  \n"
    //                      " - Z-axis height   \n"
    //                      " - Selected filament";
    //     n = str2multiline(str, n255, 20);
    //     CHECK(n == 9);
    //     CHECK_THAT(str, Equals("The status bar is at\nthe bottom of the\nscreen. It contains\ninformation about: -\nNozzle temp. -\nHeatbed temp. -\nPrinting speed -\nZ-axis height -\nSelected filament"));
    // }

    SECTION("BFW-1149.1") {
        char str[n255] = "Nel prossimo passo, "
                         "usa la manopola per "
                         "regolare l'altezza "
                         "ugello. Controlla le "
                         "immagini sul "
                         "manuale per rifer."
                         "Nel prossimo passo, "
                         "usa la manopola per "
                         "regolare l'altezza "
                         "ugello. Controlla le "
                         "immagini sul "
                         "manuale per rifer.";
        n = str2multiline(str, n255, 20);
        CHECK(n == 12);
        CHECK_THAT(str, Equals("Nel prossimo passo,\n"
                               "usa la manopola per\n"
                               "regolare l'altezza\n"
                               "ugello. Controlla le\n"
                               "immagini sul manuale\n"
                               "per rifer.Nel\n"
                               "prossimo passo, usa\n"
                               "la manopola per\n"
                               "regolare l'altezza\n"
                               "ugello. Controlla le\n"
                               "immagini sul manuale\n"
                               "per rifer."));
    }

    SECTION("BFW-1149.2") {
        char str[n255] = "La couleur est-elle\ncorrecte ?";
        n = str2multiline(str, n255, 18);
        CHECK(n == 3);
        REQUIRE_THAT(str, Equals("La couleur\nest-elle\ncorrecte ?"));
    }
}

TEST_CASE("multi-line", "[text_wrap]") {
    SECTION("all EN texts") {
        using stack_buffer = std::array<memory_source::value_type, 32>;

        std::string origin, expected;
        size_t lines;

        std::tie(origin, lines, expected) = GENERATE(
            std::make_tuple<std::string, size_t, std::string>(
                "Bootloader Version %d.%d.%d Buddy Board %d.%d.%d %s", 3, "Bootloader Version\n%d.%d.%d Buddy Board\n%d.%d.%d %s"),
            std::make_tuple<std::string, size_t, std::string>(
                "A crash dump report (file dump.bin) has been saved to the USB drive.", 4, "A crash dump report\n(file dump.bin) has\nbeen saved to the\nUSB drive."),
            std::make_tuple<std::string, size_t, std::string>(
                "All tests finished successfully!", 2, "All tests finished\nsuccessfully!"),
            std::make_tuple<std::string, size_t, std::string>(
                "Are you sure to stop this printing?", 2, "Are you sure to stop\nthis printing?"),
            std::make_tuple<std::string, size_t, std::string>(
                "Bed leveling failed. Try again?", 2, "Bed leveling failed.\nTry again?"),
            std::make_tuple<std::string, size_t, std::string>(
                "Calibration successful! Happy printing!", 3, "Calibration\nsuccessful! Happy\nprinting!"),
            std::make_tuple<std::string, size_t, std::string>(
                "Checking hotend and heatbed heaters", 2, "Checking hotend and\nheatbed heaters"),
            std::make_tuple<std::string, size_t, std::string>(
                "Congratulations! XYZ calibration is ok. XY axes are perpendicular.", 4, "Congratulations! XYZ\ncalibration is ok.\nXY axes are\nperpendicular."),
            std::make_tuple<std::string, size_t, std::string>(
                "Continual beeps test press button to stop", 2, "Continual beeps test\npress button to stop"),
            std::make_tuple<std::string, size_t, std::string>(
                "Do you want to repeat the last step and readjust the distance between the nozzle and heatbed?", 5, "Do you want to\nrepeat the last step\nand readjust the\ndistance between the\nnozzle and heatbed?"),
            std::make_tuple<std::string, size_t, std::string>(
                "Do you want to use the current value? Current: %0.3f. Default: %0.3f. Click NO to use the default value (recommended)", 7, "Do you want to use\nthe current value?\nCurrent: %0.3f.\nDefault: %0.3f.\nClick NO to use the\ndefault value\n(recommended)"),
            std::make_tuple<std::string, size_t, std::string>(
                "Do you want to use last set value? Last: %0.3f. Default: %0.3f. Click NO to use default value.", 6, "Do you want to use\nlast set value?\nLast: %0.3f.\nDefault: %0.3f.\nClick NO to use\ndefault value."),
            std::make_tuple<std::string, size_t, std::string>(
                "Error saving crash dump report to the USB drive. Please reinsert the USB drive and try again.", 5, "Error saving crash\ndump report to the\nUSB drive. Please\nreinsert the USB\ndrive and try again."),
            std::make_tuple<std::string, size_t, std::string>(
                "Everything is alright. I will run XYZ calibration now. It will take approximately 12 minutes.", 6, "Everything is\nalright. I will run\nXYZ calibration now.\nIt will take\napproximately 12\nminutes."),
            std::make_tuple<std::string, size_t, std::string>(
                "Factory defaults loaded. The system will now restart.", 3, "Factory defaults\nloaded. The system\nwill now restart."),
            std::make_tuple<std::string, size_t, std::string>(
                "Filament not detected. Load filament now? Select NO to cancel, or IGNORE to disable the filament sensor and continue.", 7, "Filament not\ndetected. Load\nfilament now? Select\nNO to cancel, or\nIGNORE to disable\nthe filament sensor\nand continue."),
            std::make_tuple<std::string, size_t, std::string>(
                "Finishing buffered gcodes.", 2, "Finishing buffered\ngcodes."),
            std::make_tuple<std::string, size_t, std::string>(
                "Heating disabled due to 30 minutes of inactivity.", 3, "Heating disabled due\nto 30 minutes of\ninactivity."),
            std::make_tuple<std::string, size_t, std::string>(
                "Hi, this is your Original Prusa MINI.", 2, "Hi, this is your\nOriginal Prusa MINI."),
            std::make_tuple<std::string, size_t, std::string>(
                "In the next step, use the knob to adjust the nozzle height. Check the pictures in the handbook for reference.", 7, "In the next step,\nuse the knob to\nadjust the nozzle\nheight. Check the\npictures in the\nhandbook for\nreference."),
            std::make_tuple<std::string, size_t, std::string>(
                "IP addresses are not valid or the file \\prusa_printer_settings.ini\\ is not in the root directory of the USB drive.", 6, "IP addresses are not\nvalid or the file\n\\prusa_printer_settings.ini\\\nis not in the root\ndirectory of the USB\ndrive."),
            std::make_tuple<std::string, size_t, std::string>(
                "IP addresses or parameters are not valid or the file \\prusa_printer_settings.ini\\ is not in the root directory of the USB drive.", 7, "IP addresses or\nparameters are not\nvalid or the file\n\\prusa_printer_settings.ini\\\nis not in the root\ndirectory of the USB\ndrive."),
            std::make_tuple<std::string, size_t, std::string>(
                "Is filament in extruder gear?", 2, "Is filament in\nextruder gear?"),
            std::make_tuple<std::string, size_t, std::string>(
                "Is steel sheet on heatbed?", 2, "Is steel sheet on\nheatbed?"),
            std::make_tuple<std::string, size_t, std::string>(
                "Make sure the filament is inserted through the sensor.", 3, "Make sure the\nfilament is inserted\nthrough the sensor."),
            std::make_tuple<std::string, size_t, std::string>(
                "Measuring reference height of calib. points", 3, "Measuring reference\nheight of calib.\npoints"),
            std::make_tuple<std::string, size_t, std::string>(
                "Mesh bed leveling failed?", 2, "Mesh bed leveling\nfailed?"),
            std::make_tuple<std::string, size_t, std::string>(
                "No filament sensor detected. Verify that the sensor is connected and try again.", 5, "No filament sensor\ndetected. Verify\nthat the sensor is\nconnected and try\nagain."),
            std::make_tuple<std::string, size_t, std::string>(
                "Now, let's calibrate the distance between the tip of the nozzle and the print sheet.", 5, "Now, let's calibrate\nthe distance between\nthe tip of the\nnozzle and the print\nsheet."),
            std::make_tuple<std::string, size_t, std::string>(
                "Once the printer starts extruding plastic, adjust the nozzle height by turning the knob until the filament sticks to the print sheet.", 8, "Once the printer\nstarts extruding\nplastic, adjust the\nnozzle height by\nturning the knob\nuntil the filament\nsticks to the print\nsheet."),
            std::make_tuple<std::string, size_t, std::string>(
                "Place a sheet of paper under the nozzle during the calibration of first 4 points. If the nozzle catches the paper, power off printer immediately!", 8, "Place a sheet of\npaper under the\nnozzle during the\ncalibration of first\n4 points. If the\nnozzle catches the\npaper, power off\nprinter immediately!"),
            std::make_tuple<std::string, size_t, std::string>(
                "Please clean the nozzle for calibration. Click NEXT when done.", 4, "Please clean the\nnozzle for\ncalibration. Click\nNEXT when done."),
            std::make_tuple<std::string, size_t, std::string>(
                "Please insert a USB drive and try again.", 2, "Please insert a USB\ndrive and try again."),
            std::make_tuple<std::string, size_t, std::string>(
                "Please insert the USB drive that came with your MINI and reset the printer to flash the firmware", 5, "Please insert the\nUSB drive that came\nwith your MINI and\nreset the printer to\nflash the firmware"),
            std::make_tuple<std::string, size_t, std::string>(
                "Please place steel sheet on heatbed.", 2, "Please place steel\nsheet on heatbed."),
            std::make_tuple<std::string, size_t, std::string>(
                "Please remove steel sheet from heatbed.", 2, "Please remove steel\nsheet from heatbed."),
            std::make_tuple<std::string, size_t, std::string>(
                "Press CONTINUE and push filament into the extruder.", 3, "Press CONTINUE and\npush filament into\nthe extruder."),
            std::make_tuple<std::string, size_t, std::string>(
                "Press NEXT to run the Selftest, which checks for potential issues related to the assembly.", 5, "Press NEXT to run\nthe Selftest, which\nchecks for potential\nissues related to\nthe assembly."),
            std::make_tuple<std::string, size_t, std::string>(
                "Searching bed calibration points", 2, "Searching bed\ncalibration points"),
            std::make_tuple<std::string, size_t, std::string>(
                "Select when you want to automatically flash updated firmware from USB flash disk.", 5, "Select when you want\nto automatically\nflash updated\nfirmware from USB\nflash disk."),
            std::make_tuple<std::string, size_t, std::string>(
                "Settings successfully loaded", 2, "Settings\nsuccessfully loaded"),
            std::make_tuple<std::string, size_t, std::string>(
                "Static IPv4 addresses were not set.", 3, "Static IPv4\naddresses were not\nset."),
            std::make_tuple<std::string, size_t, std::string>(
                "The first layer calibration failed to finish. Double-check the printer's wiring, nozzle and axes, then restart the calibration.", 8, "The first layer\ncalibration failed\nto finish.\nDouble-check the\nprinter's wiring,\nnozzle and axes,\nthen restart the\ncalibration."),
            std::make_tuple<std::string, size_t, std::string>(
                "The printer is not calibrated. Start First Layer Calibration?", 4, "The printer is not\ncalibrated. Start\nFirst Layer\nCalibration?"),
            std::make_tuple<std::string, size_t, std::string>(
                "The selftest failed to finish. Double-check the printer's wiring and axes. Then restart the Selftest.", 6, "The selftest failed\nto finish.\nDouble-check the\nprinter's wiring and\naxes. Then restart\nthe Selftest."),
            std::make_tuple<std::string, size_t, std::string>(
                "The settings have been saved successfully in the \\prusa_printer_settings.ini\\ file.", 5, "The settings have\nbeen saved\nsuccessfully in the\n\\prusa_printer_settings.ini\\\nfile."),
            std::make_tuple<std::string, size_t, std::string>(
                "The status bar is at the bottom of the screen. It contains information about: - Nozzle temp. - Heatbed temp. - Printing speed - Z-axis height - Selected filament", 9, "The status bar is at\nthe bottom of the\nscreen. It contains\ninformation about: -\nNozzle temp. -\nHeatbed temp. -\nPrinting speed -\nZ-axis height -\nSelected filament"),
            std::make_tuple<std::string, size_t, std::string>(
                "The XYZ calibration failed to finish. Double-check the printer's wiring and axes, then restart the XYZ calibration.", 6, "The XYZ calibration\nfailed to finish.\nDouble-check the\nprinter's wiring and\naxes, then restart\nthe XYZ calibration."),
            std::make_tuple<std::string, size_t, std::string>(
                "There was an error saving the settings in the \\prusa_printer_settings.ini\\ file.", 5, "There was an error\nsaving the settings\nin the\n\\prusa_printer_settings.ini\\\nfile."),
            std::make_tuple<std::string, size_t, std::string>(
                "This operation can't be undone, current configuration will be lost! Are you really sure to reset printer to factory defaults?", 7, "This operation can't\nbe undone, current\nconfiguration will\nbe lost! Are you\nreally sure to reset\nprinter to factory\ndefaults?"),
            std::make_tuple<std::string, size_t, std::string>(
                "To calibrate with currently loaded filament, press NEXT. To change filament, press UNLOAD.", 6, "To calibrate with\ncurrently loaded\nfilament, press\nNEXT. To change\nfilament, press\nUNLOAD."),
            std::make_tuple<std::string, size_t, std::string>(
                "To calibrate with currently loaded filament, press NEXT. To load filament, press LOAD. To change filament, press UNLOAD.", 8, "To calibrate with\ncurrently loaded\nfilament, press\nNEXT. To load\nfilament, press\nLOAD. To change\nfilament, press\nUNLOAD."),
            std::make_tuple<std::string, size_t, std::string>(
                "USB Disk is not ready!", 2, "USB Disk is not\nready!"),
            std::make_tuple<std::string, size_t, std::string>(
                "USB Disk is Write protected!", 2, "USB Disk is Write\nprotected!"),
            std::make_tuple<std::string, size_t, std::string>(
                "Welcome to the Original Prusa MINI setup wizard. Would you like to continue?", 5, "Welcome to the\nOriginal Prusa MINI\nsetup wizard. Would\nyou like to\ncontinue?"),
            std::make_tuple<std::string, size_t, std::string>(
                "Check the print head heater & thermistor\nwiring for possible damage.", 4, "Check the print head\nheater & thermistor\nwiring for possible\ndamage."),
            std::make_tuple<std::string, size_t, std::string>(
                "Check the print head heater & thermistor wiring for\xA0possible damage.", 4, "Check the print head\nheater & thermistor\nwiring for possible\ndamage."),
            std::make_tuple<std::string, size_t, std::string>(
                "Now, let's calibrate\nthe distance between\nthe tip of the \nnozzle and the print\nsheet.", 5, "Now, let's calibrate\nthe distance between\nthe tip of the \nnozzle and the print\nsheet."));

        memory_source mem(origin);
        monospace font;
        text_wrapper<stack_buffer, const monospace *> w(240, &font);
        size_t n = 1, index = 0;
        char str[n511], c;

        while ((c = w.character(mem)) != '\0') {
            // str[index++] = replaceChar(c, ' ', '_');
            str[index++] = c;
            if (c == '\n')
                ++n;
        }
        str[index] = '\0';
        CHECK(n == lines);
        CHECK_THAT(str, Equals(expected));
    }

    SECTION("BFW-1149.3") {
        using stack_buffer = std::array<memory_source::value_type, 32>;

        memory_source mem(
            "The status bar is at "
            "the bottom of the "
            "screen. It contains "
            "information about:\n"
            "- Nozzle temp.\n"
            "- Heatbed temp.\n"
            "- Printing speed\n"
            "- Z-axis height\n"
            "- Selected filament");
        monospace font;
        text_wrapper<stack_buffer, const monospace *> w(240, &font);
        char str[n255], c;
        size_t index = 0;

        while ((c = w.character(mem)) != '\0')
            str[index++] = c;
        str[index] = '\0';
        CHECK_THAT(str, Equals("The status bar is at\n"
                               "the bottom of the\n"
                               "screen. It contains\n"
                               "information about:\n"
                               "- Nozzle temp.\n"
                               "- Heatbed temp.\n"
                               "- Printing speed\n"
                               "- Z-axis height\n"
                               "- Selected filament"));
    }

    SECTION("BFW-1149.4") {
        using stack_buffer = std::array<memory_source::value_type, 32>;
        memory_source mem(
            "Nel prossimo passo, "
            "usa la manopola per "
            "regolare l'altezza "
            "ugello. Controlla le "
            "immagini sul "
            "manuale per rifer."
            "Nel prossimo passo, "
            "usa la manopola per "
            "regolare l'altezza "
            "ugello. Controlla le "
            "immagini sul "
            "manuale per rifer.");
        monospace font;
        text_wrapper<stack_buffer, const monospace *> w(240, &font);
        char str[n255], c;
        size_t index = 0;

        while ((c = w.character(mem)) != '\0')
            str[index++] = c;
        str[index] = '\0';

        CHECK_THAT(str, Equals("Nel prossimo passo,\n"
                               "usa la manopola per\n"
                               "regolare l'altezza\n"
                               "ugello. Controlla le\n"
                               "immagini sul manuale\n"
                               "per rifer.Nel\n"
                               "prossimo passo, usa\n"
                               "la manopola per\n"
                               "regolare l'altezza\n"
                               "ugello. Controlla le\n"
                               "immagini sul manuale\n"
                               "per rifer."));
    }

    SECTION("BFW-1390") {
        using stack_buffer = std::array<memory_source::value_type, 32>;
        memory_source mem("Was filament unload successful?");
        monospace font;
        text_wrapper<stack_buffer, const monospace *> w(231, &font);
        char str[n255], c;
        size_t index = 0;

        while ((c = w.character(mem)) != '\0') {
            /// replace spaces with underscore to visualise spaces
            if (c == ' ')
                c = '_';
            str[index++] = c;
        }
        str[index] = '\0';

        CHECK_THAT(str, Equals("Was_filament_unload\nsuccessful?"));
    }
}

size_t to_unichar(const char *ss, std::vector<unichar> *out) {
    const uint8_t *s = reinterpret_cast<const uint8_t *>(ss);
    size_t index = 0;
    while (s && *s) {
        unichar ord = *s++;
        if (!UTF8_IS_NONASCII(ord)) {
            (*out)[index++] = ord;
        } else {
            ord &= 0x7F;
            for (unichar mask = 0x40; ord & mask; mask >>= 1) {
                ord &= ~mask;
            }
            while (UTF8_IS_CONT(*s)) {
                ord = (ord << 6) | (*s++ & 0x3F);
            }
            (*out)[index++] = ord;
        }
    }
    (*out)[index] = 0;
    return index;
}

TEST_CASE("multi-line UTF-8", "[str2multiline][text_wrap]") {
    SECTION("BFW-1149.5") {
        using stack_buffer = std::array<unichar, 32>;

        const std::uint8_t utf8str[] = "příliš žluťoučký kůň úpěl ďábelské ódy : PŘÍLIŠ ŽLUŤOUČKÝ KŮŇ ÚPĚL ĎÁBELSKÉ ÓDY";
        string_view_utf8 sf = string_view_utf8::MakeCPUFLASH(utf8str);
        monospace font;
        text_wrapper<stack_buffer, const monospace *> w(240, &font);
        unichar c;
        std::vector<unichar> str(n255), expected(n255);
        size_t index = 0;
        to_unichar("příliš žluťoučký kůň\núpěl ďábelské ódy :\nPŘÍLIŠ ŽLUŤOUČKÝ KŮŇ\nÚPĚL ĎÁBELSKÉ ÓDY", &expected);
        while ((c = w.character(sf)) != '\0')
            str[index++] = c;
        str[index] = '\0';
        CHECK_THAT(str, Equals(expected));
    }
}
