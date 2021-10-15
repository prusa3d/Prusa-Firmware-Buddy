/**
 * \page            page_faq Frequently asked questions
 * \tableofcontents
 *
 * \section         sect_faq_min_at_sw What is the minimum ESP8266 AT software version?
 *
 * Library follows latest AT releases available on official Espressif website.
 *
 *  - `ESP8266 IDF AT Bin V2.0` AT command version is mandatory: https://www.espressif.com/en/support/download/at
 * 	- Use `AT+GMR\r\n` command to query current AT version running on ESP device
 *
 * \section         sect_faq_min_at_sw What is the minimum ESP32 AT software version?
 *
 * Library follows latest AT releases available on official Espressif website.
 *
 *  - `ESP32 AT Bin V1.2` AT command version is mandatory: https://www.espressif.com/en/support/download/at
 * 	- Use `AT+GMR\r\n` command to query current AT version running on ESP device
 *
 * \section         sect_faq_can_rtos Can I use this library with operating system?
 *
 * Library only works with operating systems. There is no way to use it without in current revision
 *
 * \section         sect_faq_where_do_i_get_more_info_about_esp_at_firmware Where do I get more info about ESP firmware?
 *
 * Information related to AT firmware/used pinouts for ESP device are available in official git repository
 * https://github.com/espressif/esp32-at/
 */