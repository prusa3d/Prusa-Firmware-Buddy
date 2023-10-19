#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"

/**
 * @brief Parent to show ip or host and pass click to menu.
 */
class MI_METRICS_HOST : public WiInfo<config_store_ns::metrics_host_size> {
public:
    MI_METRICS_HOST(const char *const label);

    /**
     * @brief Change shown host.
     * @param str new host to show
     */
    void ChangeHost(const char *str);

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

/**
 * @brief Parent to show port number and pass click to menu.
 */
class MI_METRICS_PORT : public WiInfo<6> {
public:
    MI_METRICS_PORT(const char *const label);

    /**
     * @brief Change shown port.
     * @param port new port to show
     */
    void ChangePort(uint16_t port);

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

/**
 * @brief Info.
 */
class MI_METRICS_INFO_LABEL : public WI_LABEL_t {
    static constexpr const char *const label = N_("What is this?");
    static constexpr const char *const txt_info = N_("This feature allows you to gather diagnostic data to show in Grafana. Be careful, it can send unencrypted data to the internet.\n\nAllow any host and use M33x G-codes to configure metrics and system log. After that, you can store host and port by clicking the current configuration.");

public:
    MI_METRICS_INFO_LABEL();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

/**
 * @brief Allow Net Metrics.
 * Select if metrics are allowed to be enabled by G-code, or only given host is allowed.
 */
class MI_METRICS_ALLOW : public WI_SWITCH_t<3> {
    static constexpr const char *const label = N_("Allow");
    static constexpr const char *const txt_confirm = N_("This will allow network to be enabled by M33x G-codes. It can send unencrypted diagnostics data to the internet. Do you really want to allow this?");
    static constexpr const char *const label_none = N_("None");
    static constexpr const char *const label_one = N_("Only Stored");
    static constexpr const char *const label_all = N_("Any Host");

    bool warning_not_shown; ///< Show warning only once per coming into this menu

public:
    MI_METRICS_ALLOW();
    virtual void OnChange(size_t old_index) override;
};

/**
 * @brief If enabled, initialize metrics and log host on start.
 */
class MI_METRICS_INIT : public WI_ICON_SWITCH_OFF_ON_t {
    static constexpr const char *const label = N_("Enable Stored on Startup");

public:
    MI_METRICS_INIT();
    virtual void OnChange(size_t old_index) override;
};

/**
 * @brief Splitter to show stored configuration.
 */
class MI_METRICS_CONF_LABEL : public WI_LABEL_t {
    static constexpr const char *const label = N_("Stored Configuration:");

public:
    MI_METRICS_CONF_LABEL();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

namespace screen_menu_metrics_labels {
#ifdef USE_ST7789
static constexpr const char *const host = N_("H");
static constexpr const char *const metrics_host = host;
static constexpr const char *const log_host = host;
static constexpr const char *const metrics_port = N_("Metrics Port");
static constexpr const char *const log_port = N_("Log Port");
#else /*USE_ST7789*/
// Keep spaces in front for menu alignment
static constexpr const char *const host = N_("  Host");
static constexpr const char *const metrics_host = N_("  Metrics Host");
static constexpr const char *const log_host = N_("  Log Host");
static constexpr const char *const metrics_port = N_("  Metrics Port");
static constexpr const char *const log_port = N_("  Log Port");
#endif /*USE_ST7789*/
} // namespace screen_menu_metrics_labels

// Stored configuration for init and to allow
class MI_METRICS_CONF_HOST : public MI_METRICS_HOST {

public:
    MI_METRICS_CONF_HOST()
        : MI_METRICS_HOST(screen_menu_metrics_labels::host) {}
};

class MI_METRICS_CONF_PORT : public MI_METRICS_PORT {

public:
    MI_METRICS_CONF_PORT()
        : MI_METRICS_PORT(screen_menu_metrics_labels::metrics_port) {}
};

class MI_METRICS_CONF_LOG_PORT : public MI_METRICS_PORT {

public:
    MI_METRICS_CONF_LOG_PORT()
        : MI_METRICS_PORT(screen_menu_metrics_labels::log_port) {}
};

/**
 * @brief Splitter to show current configuration.
 */
class MI_METRICS_CURRENT_LABEL : public WI_LABEL_t {
    static constexpr const char *const label = N_("Current Configuration:");

public:
    MI_METRICS_CURRENT_LABEL();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

// Current configuration
class MI_METRICS_CURRENT_M_HOST : public MI_METRICS_HOST {

public:
    MI_METRICS_CURRENT_M_HOST()
        : MI_METRICS_HOST(screen_menu_metrics_labels::metrics_host) {}
};

class MI_METRICS_CURRENT_M_PORT : public MI_METRICS_PORT {

public:
    MI_METRICS_CURRENT_M_PORT()
        : MI_METRICS_PORT(screen_menu_metrics_labels::metrics_port) {}
};

class MI_METRICS_CURRENT_L_HOST : public MI_METRICS_HOST {

public:
    MI_METRICS_CURRENT_L_HOST()
        : MI_METRICS_HOST(screen_menu_metrics_labels::log_host) {}
};

class MI_METRICS_CURRENT_L_PORT : public MI_METRICS_PORT {

public:
    MI_METRICS_CURRENT_L_PORT()
        : MI_METRICS_PORT(screen_menu_metrics_labels::log_port) {}
};

/**
 * @brief Screen menu for metrics settings.
 */
namespace detail {
using ScreenMenuMetricsSettings = ScreenMenu<EFooter::Off, MI_RETURN, MI_METRICS_INFO_LABEL, MI_METRICS_ALLOW, MI_METRICS_INIT,
    MI_METRICS_CONF_LABEL, MI_METRICS_CONF_HOST, MI_METRICS_CONF_PORT, MI_METRICS_CONF_LOG_PORT,
    MI_METRICS_CURRENT_LABEL, MI_METRICS_CURRENT_M_HOST, MI_METRICS_CURRENT_M_PORT, MI_METRICS_CURRENT_L_HOST, MI_METRICS_CURRENT_L_PORT>;
}

class ScreenMenuMetricsSettings : public detail::ScreenMenuMetricsSettings {
    static constexpr const char *const label = N_("METRICS & LOG");
    static constexpr const char *const txt_clear_all_ask = N_("Clear all stored values?");
    static constexpr const char *const txt_clear_ask = N_("Clear this value?");
    static constexpr const char *const txt_all_ask = N_("Store current values?");
    static constexpr const char *const txt_host_ask = N_("Store this as Host?");
    static constexpr const char *const txt_m_port_ask = N_("Store this as Metrics Port?");
    static constexpr const char *const txt_l_port_ask = N_("Store this as Log port?");

    /**
     * @brief Refresh items that show current hosts and ports.
     */
    void refresh_current();

public:
    ScreenMenuMetricsSettings();

protected:
    void windowEvent(EventLock /*has private ctor*/, window_t *sender, GUI_event_t event, void *param);
};
