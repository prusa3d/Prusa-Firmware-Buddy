#pragma once

#include "screen_menu.hpp"
#include "WindowMenuItems.hpp"
#include <guiconfig/guiconfig.h>
#include <MItem_network.hpp>
#include <netdev.h>

class MI_METRICS_HOST : public WiInfo<config_store_ns::metrics_host_size> {
public:
    MI_METRICS_HOST();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_METRICS_PORT : public WiSpin {
public:
    MI_METRICS_PORT();

protected:
    virtual void OnClick() override;
};

class MI_SYSLOG_PORT : public WiSpin {
public:
    MI_SYSLOG_PORT();

protected:
    virtual void OnClick() override;
};

class MI_METRICS_INFO_LABEL : public IWindowMenuItem {
public:
    MI_METRICS_INFO_LABEL();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_METRICS_ENABLE : public WI_ICON_SWITCH_OFF_ON_t {
public:
    MI_METRICS_ENABLE();

protected:
    virtual void OnChange(size_t old_index) override;
};

class MI_METRICS_LIST : public IWindowMenuItem {
public:
    MI_METRICS_LIST();

protected:
    virtual void click(IWindowMenu &) override;
};

class ScreenMenuMetricsSettings : public ScreenMenu<EFooter::Off, MI_RETURN, MI_METRICS_INFO_LABEL, MI_METRICS_ENABLE, MI_METRICS_HOST, MI_METRICS_PORT, MI_SYSLOG_PORT, WMI_NET<MI_MAC_ADDR, NETDEV_ETH_ID>, MI_METRICS_LIST> {
public:
    ScreenMenuMetricsSettings();
};
