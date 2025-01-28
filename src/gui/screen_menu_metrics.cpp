
#include "screen_menu_metrics.hpp"

#include <ScreenHandler.hpp>
#include <img_resources.hpp>
#include <i18n.h>
#include <metric_handlers.h>
#include <config_store/store_instance.hpp>
#include <logging/log_dest_syslog.hpp>
#include <guiconfig/guiconfig.h>
#include <numeric_input_config_common.hpp>
#include <dialog_text_input.hpp>
#include <screen_menu_metrics_list.hpp>

LOG_COMPONENT_REF(GUI);

MI_METRICS_HOST::MI_METRICS_HOST()
    : WiInfo(_("Host")) {
    ChangeInformation(config_store().metrics_host.get().data());
}

void MI_METRICS_HOST::click(IWindowMenu &) {
    auto host = config_store().metrics_host.get();

    if (!DialogTextInput::exec(GetLabel(), host)) {
        return;
    }

    ChangeInformation(host.data());
    config_store().metrics_host.set(host);
    metrics_reconfigure();
    logging::syslog_reconfigure();
}

MI_METRICS_PORT::MI_METRICS_PORT()
    : WiSpin(config_store().metrics_port.get(), numeric_input_config::network_port, _("Metrics Port")) {}

void MI_METRICS_PORT::OnClick() {
    config_store().metrics_port.set(value());
    metrics_reconfigure();
}

MI_SYSLOG_PORT::MI_SYSLOG_PORT()
    : WiSpin(config_store().syslog_port.get(), numeric_input_config::network_port, _("Syslog Port")) {}

void MI_SYSLOG_PORT::OnClick() {
    config_store().syslog_port.set(value());
    logging::syslog_reconfigure();
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
    if (value() && !config_store().enable_metrics.get()) {
        MsgBoxWarning(_("Failed to enable metrics. Check your settings."), Responses_Ok);
        set_value(false, false);
    }
}

MI_METRICS_LIST::MI_METRICS_LIST()
    : IWindowMenuItem(_("Metrics List"), nullptr, is_enabled_t::yes, is_hidden_t::no, expands_t::yes) {
}

void MI_METRICS_LIST::click(IWindowMenu &) {
    Screens::Access()->Open<ScreenMenuMetricsList>();
}

ScreenMenuMetricsSettings::ScreenMenuMetricsSettings()
    : ScreenMenu(_("METRICS & LOG")) {}
