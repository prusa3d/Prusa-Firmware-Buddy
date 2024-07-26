
#include "screen_menu_metrics.hpp"

#include <ScreenHandler.hpp>
#include <img_resources.hpp>
#include <i18n.h>
#include <metric_handlers.h>
#include <config_store/store_instance.hpp>
#include <log_dest_syslog.h>
#include <guiconfig/guiconfig.h>

LOG_COMPONENT_REF(GUI);

/**
 * @brief Check if allowed config changed and something needs to be disabled.
 */
static void check_disable() {
    MetricsAllow metrics_allow = config_store().metrics_allow.get();
    if (metrics_allow == MetricsAllow::None) {
        // Nothing is allowed, disable all
        metric_handler_syslog_configure("", 0);
        syslog_configure("", 0);
    } else if (metrics_allow == MetricsAllow::One) {
        // Only one is allowed, disable other
        if (strcmp(metric_handler_syslog_get_host(), config_store().metrics_host.get_c_str()) != 0
            || metric_handler_syslog_get_port() != config_store().metrics_port.get()) {
            metric_handler_syslog_configure("", 0);
        }
        if (strcmp(syslog_get_host(), config_store().metrics_host.get_c_str()) != 0
            || syslog_get_port() != config_store().syslog_port.get()) {
            syslog_configure("", 0);
        }
    }
}

// Host show and click
MI_METRICS_HOST::MI_METRICS_HOST(const char *const label)
    : WiInfo(_(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {
}

void MI_METRICS_HOST::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)this);
}

void MI_METRICS_HOST::ChangeHost(const char *str) {
    if (strlen(str) == 0) { // Host not set
        ChangeInformation("---");
    } else {
        ChangeInformation(str);
    }
}

// Port show and click
MI_METRICS_PORT::MI_METRICS_PORT(const char *const label)
    : WiInfo(config_store().metrics_port.get(), _(label)) {}

void MI_METRICS_PORT::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)this);
}

void MI_METRICS_PORT::ChangePort(uint16_t port) {
    if (port == 0) { // Invalid port
        ChangeInformation("---");
    } else {
        char port_str[GetInfoLen()];
        snprintf(port_str, std::size(port_str), "%u", port);
        ChangeInformation(port_str);
    }
}

// Info message
MI_METRICS_INFO_LABEL::MI_METRICS_INFO_LABEL()
    : IWindowMenuItem(_(label), &img::question_16x16, is_enabled_t::yes, is_hidden_t::no) {}

Response MetricsInfoMsgbox(string_view_utf8 txt) {
    static constexpr Response rsp { Response::Ok };
    const PhaseTexts labels = { get_response_text(rsp) };
    MsgBoxBase msgbox(GuiDefaults::RectScreen, { rsp }, 0, &labels, txt);
    msgbox.set_text_font(GuiDefaults::FontMenuSpecial);
    Screens::Access()->gui_loop_until_dialog_closed();
    return msgbox.GetResult();
}

void MI_METRICS_INFO_LABEL::click(IWindowMenu & /*window_menu*/) {
#if HAS_MINI_DISPLAY()
    MetricsInfoMsgbox(_(txt_info));
#else
    MsgBoxBase msgbox(GuiDefaults::RectScreenNoHeader, {}, 0, nullptr, _(txt_info));
    msgbox.SetAlignment(Align_t::LeftTop());
    msgbox.AdjustTextRect(Rect16(30, 0, GuiDefaults::ScreenWidth - 60, GuiDefaults::ScreenHeight));
    Screens::Access()->gui_loop_until_dialog_closed();
#endif
}

// Allow switch
MI_METRICS_ALLOW::MI_METRICS_ALLOW()
    : WI_SWITCH_t(ftrstd::to_underlying(config_store().metrics_allow.get()), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no, _(label_none), _(label_one), _(label_all))
    , warning_not_shown(config_store().metrics_allow.get() == MetricsAllow::None) {}

void MI_METRICS_ALLOW::OnChange(size_t old_index) {
    if (old_index == ftrstd::to_underlying(MetricsAllow::None) && index != ftrstd::to_underlying(MetricsAllow::None)) { // Enable
        if (warning_not_shown && MsgBoxWarning(_(txt_confirm), Responses_YesNo, 1) != Response::Yes) {
            index = ftrstd::to_underlying(MetricsAllow::None); // User changed his mind
        } else {
            warning_not_shown = false;
        }
    }

    config_store().metrics_allow.set(MetricsAllow(index));

    check_disable();
}

// Init SYSLOG handler at start
MI_METRICS_INIT::MI_METRICS_INIT()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().metrics_init.get(), _(label), nullptr, is_enabled_t::yes, is_hidden_t::no) {}

void MI_METRICS_INIT::OnChange([[maybe_unused]] size_t old_index) {
    config_store().metrics_init.set(index);

    // Configure
    if (index) {
        metric_handler_syslog_configure(config_store().metrics_host.get_c_str(), config_store().metrics_port.get());
        syslog_configure(config_store().metrics_host.get_c_str(), config_store().syslog_port.get());
    }
}

// Stored configuration splitter
MI_METRICS_CONF_LABEL::MI_METRICS_CONF_LABEL()
    : IWindowMenuItem(_(label), &img::arrow_right_10x16, is_enabled_t::yes, is_hidden_t::no) {}

void MI_METRICS_CONF_LABEL::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)this);
}

// Current configuration splitter
MI_METRICS_CURRENT_LABEL::MI_METRICS_CURRENT_LABEL()
    : IWindowMenuItem(_(label), &img::arrow_right_10x16, is_enabled_t::yes, is_hidden_t::no) {}

void MI_METRICS_CURRENT_LABEL::click(IWindowMenu & /*window_menu*/) {
    Screens::Access()->WindowEvent(GUI_event_t::CHILD_CLICK, (void *)this);
}

// Menu
ScreenMenuMetricsSettings::ScreenMenuMetricsSettings()
    : detail::ScreenMenuMetricsSettings(_(label)) {
    ClrMenuTimeoutClose(); // Complicated menu, don't close this menu automatically

    Item<MI_METRICS_CONF_HOST>().ChangeHost(config_store().metrics_host.get_c_str());
    Item<MI_METRICS_CONF_PORT>().ChangePort(config_store().metrics_port.get());
    Item<MI_METRICS_CONF_LOG_PORT>().ChangePort(config_store().syslog_port.get());

    refresh_current();
}

void ScreenMenuMetricsSettings::refresh_current() {
    Item<MI_METRICS_CURRENT_M_HOST>().ChangeHost(metric_handler_syslog_get_host());
    Item<MI_METRICS_CURRENT_M_PORT>().ChangePort(metric_handler_syslog_get_port());

    Item<MI_METRICS_CURRENT_L_HOST>().ChangeHost(syslog_get_host());
    Item<MI_METRICS_CURRENT_L_PORT>().ChangePort(syslog_get_port());
}

void ScreenMenuMetricsSettings::windowEvent([[maybe_unused]] window_t *sender, GUI_event_t event, void *param) {
    if (event == GUI_event_t::LOOP) {
        // Periodically refresh current hosts and ports
        static int refresh = 0;
        if (++refresh > 20) { // Approx 1 s
            refresh = 0;

            refresh_current();
        }
    } else if (event == GUI_event_t::CHILD_CLICK) {
        auto &store = config_store();

        // Menu item was clicked, offer clear or store
        if (param == reinterpret_cast<void *>(&Item<MI_METRICS_CONF_LABEL>())) {
            // Clear all
            if (MsgBoxQuestion(_(txt_clear_all_ask), Responses_YesNo) == Response::Yes) {
                {
                    auto transaction = store.get_backend().transaction_guard();
                    store.metrics_host.set("");
                    store.metrics_port.set(uint16_t(0));
                    store.syslog_port.set(uint16_t(0));
                }
                Item<MI_METRICS_CONF_HOST>().ChangeHost(store.metrics_host.get_c_str());
                Item<MI_METRICS_CONF_PORT>().ChangePort(store.metrics_port.get());
                Item<MI_METRICS_CONF_LOG_PORT>().ChangePort(store.syslog_port.get());
            }
        } else if (param == reinterpret_cast<void *>(&Item<MI_METRICS_CONF_HOST>())) {
            // Clear host
            if (MsgBoxQuestion(_(txt_clear_ask), Responses_YesNo) == Response::Yes) {
                store.metrics_host.set("");
                Item<MI_METRICS_CONF_HOST>().ChangeHost(store.metrics_host.get_c_str());
            }
        } else if (param == reinterpret_cast<void *>(&Item<MI_METRICS_CONF_PORT>())) {
            // Clear metrics port
            if (MsgBoxQuestion(_(txt_clear_ask), Responses_YesNo) == Response::Yes) {
                store.metrics_port.set(uint16_t(0));
                Item<MI_METRICS_CONF_PORT>().ChangePort(store.metrics_port.get());
            }
        } else if (param == reinterpret_cast<void *>(&Item<MI_METRICS_CONF_LOG_PORT>())) {
            // Clear log port
            if (MsgBoxQuestion(_(txt_clear_ask), Responses_YesNo) == Response::Yes) {
                store.syslog_port.set(uint16_t(0));
                Item<MI_METRICS_CONF_LOG_PORT>().ChangePort(store.syslog_port.get());
            }
        } else if (param == reinterpret_cast<void *>(&Item<MI_METRICS_CURRENT_LABEL>())) {
            // Store all
            if (MsgBoxQuestion(_(txt_all_ask), Responses_YesNo) == Response::Yes) {
                {
                    auto transaction = store.get_backend().transaction_guard();
                    store.metrics_host.set(metric_handler_syslog_get_host());
                    store.metrics_port.set(metric_handler_syslog_get_port());
                    store.syslog_port.set(syslog_get_port());
                }
                Item<MI_METRICS_CONF_HOST>().ChangeHost(store.metrics_host.get_c_str());
                Item<MI_METRICS_CONF_PORT>().ChangePort(store.metrics_port.get());
                Item<MI_METRICS_CONF_LOG_PORT>().ChangePort(store.syslog_port.get());
            }
        } else if (param == reinterpret_cast<void *>(&Item<MI_METRICS_CURRENT_M_HOST>())) {
            // Store metrics host
            if (MsgBoxQuestion(_(txt_host_ask), Responses_YesNo) == Response::Yes) {
                store.metrics_host.set(metric_handler_syslog_get_host());
                Item<MI_METRICS_CONF_HOST>().ChangeHost(store.metrics_host.get_c_str());
            }
        } else if (param == reinterpret_cast<void *>(&Item<MI_METRICS_CURRENT_M_PORT>())) {
            // Store metrics port
            if (MsgBoxQuestion(_(txt_m_port_ask), Responses_YesNo) == Response::Yes) {
                store.metrics_port.set(metric_handler_syslog_get_port());
                Item<MI_METRICS_CONF_PORT>().ChangePort(store.metrics_port.get());
            }
        } else if (param == reinterpret_cast<void *>(&Item<MI_METRICS_CURRENT_L_HOST>())) {
            // Store log host
            if (MsgBoxQuestion(_(txt_host_ask), Responses_YesNo) == Response::Yes) {
                // Use log host to store as metrics host (we remember only one host)
                store.metrics_host.set(syslog_get_host());
                Item<MI_METRICS_CONF_HOST>().ChangeHost(store.metrics_host.get_c_str());
            }
        } else if (param == reinterpret_cast<void *>(&Item<MI_METRICS_CURRENT_L_PORT>())) {
            // Store log port
            if (MsgBoxQuestion(_(txt_l_port_ask), Responses_YesNo) == Response::Yes) {
                store.syslog_port.set(syslog_get_port());
                Item<MI_METRICS_CONF_LOG_PORT>().ChangePort(store.syslog_port.get());
            }
        }
        check_disable(); // Check if allowed config changed and needs to be disabled
        refresh_current();
    }
}
