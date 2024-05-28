
#include "screen_menu_metrics.hpp"

#include <ScreenHandler.hpp>
#include <img_resources.hpp>
#include <i18n.h>
#include <metric_handlers.h>
#include <config_store/store_instance.hpp>
#include <logging/log_dest_syslog.hpp>
#include <guiconfig/guiconfig.h>

LOG_COMPONENT_REF(GUI);

MI_METRICS_HOST::MI_METRICS_HOST()
    : WiInfo(_("Host")) {
    ChangeInformation(config_store().metrics_host.get().data());
}

void MI_METRICS_HOST::click(IWindowMenu &) {
    // TODO ability to change host
}

MI_METRICS_PORT::MI_METRICS_PORT()
    : WiInfo(config_store().metrics_port.get(), _("Metrics Port")) {}

void MI_METRICS_PORT::click(IWindowMenu &) {
    // TODO ability to change port
}

MI_SYSLOG_PORT::MI_SYSLOG_PORT()
    : WiInfo(config_store().syslog_port.get(), _("Syslog Port")) {}

void MI_SYSLOG_PORT::click(IWindowMenu &) {
    // TODO ability to change port
}

// Info message
MI_METRICS_INFO_LABEL::MI_METRICS_INFO_LABEL()
    : IWindowMenuItem(_("What is this?"), &img::question_16x16) {}

void MI_METRICS_INFO_LABEL::click(IWindowMenu &) {
    MsgBoxInfo(_("This feature allows you to gather diagnostic data to show in Grafana.\n\nBe careful, it can send unencrypted data to the internet."));
}

MI_METRICS_ENABLE::MI_METRICS_ENABLE()
    : WI_ICON_SWITCH_OFF_ON_t(config_store().enable_metrics.get(), _("Enable Metrics")) {}

void MI_METRICS_ENABLE::OnChange([[maybe_unused]] size_t old_index) {
    config_store().enable_metrics.set(index);
    logging::syslog_reconfigure();
    metrics_reconfigure();
}

ScreenMenuMetricsSettings::ScreenMenuMetricsSettings()
    : ScreenMenu(_("METRICS & LOG")) {}
