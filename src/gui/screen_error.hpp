#pragma once

#include <array>
#include <gui.hpp>
#include <screen.hpp>
#include <gui/qr.hpp>
#include <window_text.hpp>
#include <window_icon.hpp>
#include <window_header.hpp>
#include <gui/text_error_url.hpp>
#include <crash_dump/dump.hpp>

class ScreenError : public screen_t {
public:
    ScreenError();

private:
    std::array<char, crash_dump::MSG_TITLE_MAX_LEN> err_title_buff;
    std::array<char, crash_dump::MSG_MAX_LEN> err_msg_buff;
    std::array<char, 30> debug_info_buff;

protected:
    virtual void windowEvent(window_t *sender, GUI_event_t event, void *param) override;

    window_header_t header;

    window_text_t txt_err_title;
    window_text_t txt_err_desc;
    window_text_t txt_printer_model;
    window_text_t txt_fw_version;
    window_text_t txt_helper;
    window_text_t txt_scan_me;
    window_text_t txt_debug_info;
    window_text_t txt_bsod_msg;

    TextErrorUrlWindow txt_help_link;
    QRErrorUrlWindow qr;

    BasicWindow title_line;
};
