#include "control_command.h"
#include "handler.h"
#include "json_parser.h"

#include <algorithm>
#include <cassert>
#include <cstring>
#include <cmath>
#include <optional>

namespace nhttp::printer {

using namespace handler;
using http::Status;
using json::Event;
using json::Type;
using std::nullopt;
using std::optional;
using std::string_view;

ControlCommand::ControlCommand(size_t content_length, bool can_keep_alive, bool json_errors)
    : content_length(content_length)
    , can_keep_alive(can_keep_alive)
    , json_errors(json_errors) {
    memset(buffer.data(), 0, buffer.size());
}

StatusPage ControlCommand::process() {
    optional<int> nozzle;
    optional<int> bed;
    optional<int> chamber;
    optional<int> speed;
    optional<int> flow;
    optional<int> fan_print;
    optional<int> chamber_fan;
    optional<int> led;

    bool do_home = false, home_x = false, home_y = false, home_z = false;
    optional<float> move_x, move_y, move_z, move_e;
    optional<int> feedrate;

    bool do_load_filament = false, do_unload_filament = false;
    bool do_purge = false, do_filament_change = false;
    bool do_cooldown = false, do_motors_off = false;
    bool do_stop = false, do_reboot = false;
    // String fields need null-terminated copies. The JSON parser returns
    // string_views with explicit length pointing into the request buffer
    // (NOT null-terminated). Anything that subsequently reads until \0
    // picks up trailing JSON syntax (e.g. the closing quote/brace) — for
    // gcode this leaks into echo:enqueueing output, for dialog_response
    // it breaks the exact-match Response lookup.
    char gcode_cmd_buf[120] = { 0 };  // M-code lines are short; cap with margin
    bool gcode_cmd_set = false;
    char dialog_btn_buf[32] = { 0 };
    bool dialog_btn_set = false;

    bool got_any_field = false;

    const auto parse_result = parse_command(reinterpret_cast<char *>(buffer.data()), buffer_used, [&](const Event &event) {
        if (event.depth != 1) {
            return;
        }
        const auto &key = event.key.value();
        const auto &val = event.value.value();

        if (event.type == Type::String) {
            if (key == "gcode") {
                const size_t copy_len = std::min(val.size(), sizeof(gcode_cmd_buf) - 1);
                memcpy(gcode_cmd_buf, val.data(), copy_len);
                gcode_cmd_buf[copy_len] = '\0';
                gcode_cmd_set = true;
                got_any_field = true;
            } else if (key == "dialog_response") {
                const size_t copy_len = std::min(val.size(), sizeof(dialog_btn_buf) - 1);
                memcpy(dialog_btn_buf, val.data(), copy_len);
                dialog_btn_buf[copy_len] = '\0';
                dialog_btn_set = true;
                got_any_field = true;
            }
            return;
        }

        if (event.type == Type::Primitive) {
            if (val == "true") {
                if (key == "home") { do_home = true; home_x = home_y = home_z = true; }
                else if (key == "home_x") { do_home = true; home_x = true; }
                else if (key == "home_y") { do_home = true; home_y = true; }
                else if (key == "home_z") { do_home = true; home_z = true; }
                else if (key == "load_filament") { do_load_filament = true; }
                else if (key == "unload_filament") { do_unload_filament = true; }
                else if (key == "purge") { do_purge = true; }
                else if (key == "filament_change") { do_filament_change = true; }
                else if (key == "cooldown") { do_cooldown = true; }
                else if (key == "motors_off") { do_motors_off = true; }
                else if (key == "stop") { do_stop = true; }
                else if (key == "reboot") { do_reboot = true; }
                got_any_field = true;
                return;
            }
            if (val == "false") {
                got_any_field = true;
                return;
            }

            bool is_float = false;
            for (auto c : val) {
                if (c == '.') { is_float = true; break; }
            }

            if (is_float) {
                float fval = strtof(val.data(), nullptr);
                if (key == "move_x") { move_x = fval; }
                else if (key == "move_y") { move_y = fval; }
                else if (key == "move_z") { move_z = fval; }
                else if (key == "move_e") { move_e = fval; }
            } else {
                int ival = atoi(val.data());
                if (key == "nozzle") { nozzle = ival; }
                else if (key == "bed") { bed = ival; }
                else if (key == "chamber") { chamber = ival; }
                else if (key == "speed") { speed = ival; }
                else if (key == "flow") { flow = ival; }
                else if (key == "fan_print") { fan_print = ival; }
                else if (key == "chamber_fan") { chamber_fan = ival; }
                else if (key == "led") { led = ival; }
                else if (key == "feedrate") { feedrate = ival; }
                else if (key == "move_x") { move_x = static_cast<float>(ival); }
                else if (key == "move_y") { move_y = static_cast<float>(ival); }
                else if (key == "move_z") { move_z = static_cast<float>(ival); }
                else if (key == "move_e") { move_e = static_cast<float>(ival); }
            }
            got_any_field = true;
        }
    });

    switch (parse_result) {
    case JsonParseResult::ErrMem:
        return StatusPage(Status::PayloadTooLarge, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "Too many JSON tokens");
    case JsonParseResult::ErrReq:
        return StatusPage(Status::BadRequest, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "Couldn't parse JSON");
    case JsonParseResult::Ok:
        break;
    }

    if (!got_any_field) {
        return StatusPage(Status::BadRequest, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors, nullopt, "No control fields provided");
    }

    if (do_stop) {
        emergency_stop();
        return StatusPage(Status::NoContent, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors);
    }

    if (nozzle.has_value()) set_nozzle_temp(*nozzle);
    if (bed.has_value()) set_bed_temp(*bed);
    if (chamber.has_value()) set_chamber_temp(*chamber);
    if (speed.has_value()) set_speed(*speed);
    if (flow.has_value()) set_flow(*flow);
    if (fan_print.has_value()) set_fan_print(*fan_print);
    if (chamber_fan.has_value()) set_chamber_fan(*chamber_fan);
    if (led.has_value()) set_led(*led);

    if (do_load_filament) load_filament();
    if (do_unload_filament) unload_filament();
    if (do_purge) purge();
    if (do_filament_change) filament_change();
    if (do_cooldown) cooldown();
    if (do_motors_off) motors_off();

    if (do_home) home_axes(home_x, home_y, home_z);
    if (move_x.has_value() || move_y.has_value() || move_z.has_value() || move_e.has_value()) {
        relative_move(move_x.value_or(0), move_y.value_or(0), move_z.value_or(0), move_e.value_or(0), feedrate.value_or(1000));
    }

    if (gcode_cmd_set) send_gcode(gcode_cmd_buf);
    if (dialog_btn_set) dialog_response(dialog_btn_buf);
    if (do_reboot) reboot();

    return StatusPage(Status::NoContent, can_keep_alive ? StatusPage::CloseHandling::KeepAlive : StatusPage::CloseHandling::Close, json_errors);
}

void ControlCommand::step(string_view input, bool terminated_by_client, uint8_t *, size_t, Step &out) {
    if (content_length > buffer.size()) {
        out = Step { 0, 0, StatusPage(Status::PayloadTooLarge, StatusPage::CloseHandling::ErrorClose, json_errors) };
        return;
    }

    const size_t rest = content_length - buffer_used;
    const size_t to_read = std::min(input.size(), rest);

    memcpy(buffer.data() + buffer_used, input.data(), to_read);
    buffer_used += to_read;

    if (content_length > buffer_used) {
        if (terminated_by_client) {
            out = Step { to_read, 0, StatusPage(Status::BadRequest, StatusPage::CloseHandling::ErrorClose, json_errors, nullopt, "Truncated request") };
            return;
        } else {
            out = Step { to_read, 0, Continue() };
            return;
        }
    }

    out = Step { to_read, 0, process() };
}

} // namespace nhttp::printer
