#pragma once
#include "WindowMenuItems.hpp"
#include "i18n.h"

#include <wui_api.h>

struct NetDeviceID {

public:
    /// Special value to always represent the currently active interface.
    static constexpr uint32_t active_id = uint32_t(-1);

public:
    inline NetDeviceID(uint32_t val = active_id)
        : value_(val) {}

    inline NetDeviceID(const NetDeviceID &) = default;

    uint32_t get() const;

    /// \returns whether the net device with this id is connected
    bool is_connected() const;

    inline uint32_t operator()() const {
        return get();
    }

private:
    uint32_t value_ = active_id;
};

/// Wrapper for MI_NET menu items that accept device_id in their constructors
template <typename Parent, uint32_t device_id>
class WMI_NET final : public Parent {
public:
    WMI_NET()
        : Parent(device_id) {}
};

class MI_WIFI_STATUS_t : public WI_INFO_t {
    constexpr static const char *const label = N_("Status");

public:
    MI_WIFI_STATUS_t();
};

class MI_WIFI_INIT_t : public IWindowMenuItem {
    constexpr static const char *const label = N_("Setup Wi-Fi Module");

public:
    MI_WIFI_INIT_t();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_WIFI_CREDENTIALS_t : public IWindowMenuItem {
    constexpr static const char *const label = N_("Load Credentials");

public:
    MI_WIFI_CREDENTIALS_t();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_WIFI_CREDENTIALS_INI_FILE_t : public IWindowMenuItem {
    constexpr static const char *const label = N_("Create Credentials");

public:
    MI_WIFI_CREDENTIALS_INI_FILE_t();

protected:
    virtual void click(IWindowMenu &window_menu) override;
};

class MI_NET_INTERFACE_t : public WI_SWITCH_t<2> {
    constexpr static const char *const label = N_("Default");

    constexpr static const char *str_eth = "Eth"; // do not translate
    constexpr static const char *str_wifi = "Wi-Fi"; // do not translate

public:
    MI_NET_INTERFACE_t();
    virtual void OnChange(size_t old_index) override;
};

class MI_HOSTNAME : public WiInfo<config_store_ns::lan_hostname_max_len + 1> {
    // Printer's name within network
    constexpr static const char *const label { N_("Hostname") };

public:
    MI_HOSTNAME(NetDeviceID device_id = {});

    void update();

public:
    const NetDeviceID device_id;
};
static_assert(UpdatableMenuItem<MI_HOSTNAME>);

/// Use WMI_NET as a wrapper to provide the device_id
class MI_NET_IP : public WI_SWITCH_t<2> {
    constexpr static const char *const label = "LAN"; // do not translate

    constexpr static const char *str_static = "static"; // do not translate
    constexpr static const char *str_DHCP = "DHCP"; // do not translate

public:
    MI_NET_IP(NetDeviceID device_id);
    virtual void OnChange(size_t old_index) override;

public:
    const NetDeviceID device_id;
};

class MI_NET_IP_VER_t : public WI_SWITCH_t<2> {
    constexpr static const char *const label = N_("Protocol");

    constexpr static const char *str_v4 = "IPv4"; // do not translate
    constexpr static const char *str_v6 = "IPv6"; // do not translate

public:
    MI_NET_IP_VER_t();
    // virtual void OnChange(size_t old_index) override; //TODO
};

#define UNKNOWN_ADDR "N/A"
#define ADDR_LEN     sizeof("255.255.255.255")
#define UNKNOWN_MAC  "N/A"
#define MAC_LEN      sizeof("ff.ff.ff.ff.ff.ff")

class IMI_IP4_ADDR : public WiInfo<ADDR_LEN> {

public:
    IMI_IP4_ADDR(const char *label, NetDeviceID device_id, ETHVAR_t var);

    void update();

public:
    const ETHVAR_t var;
    const NetDeviceID device_id;

protected:
    // Make destructor protected to indicate that we don't need to emit a vtable for this class
    ~IMI_IP4_ADDR() = default;
};
static_assert(UpdatableMenuItem<IMI_IP4_ADDR>);

/// Use WMI_NET as a wrapper to provide the device_id
class MI_IP4_ADDR : public IMI_IP4_ADDR {
    static constexpr const char *const label = GuiDefaults::ScreenWidth > 240 ? N_("IPv4 Address") : N_("IP");

public:
    inline MI_IP4_ADDR(NetDeviceID device_id = {})
        : IMI_IP4_ADDR(label, device_id, ETHVAR_LAN_ADDR_IP4) {}
};

/// Use WMI_NET as a wrapper to provide the device_id
class MI_IP4_NMSK : public IMI_IP4_ADDR {
    static constexpr const char *const label = GuiDefaults::ScreenWidth > 240 ? N_("IPv4 Netmask") : N_("Mask");

public:
    inline MI_IP4_NMSK(NetDeviceID device_id = {})
        : IMI_IP4_ADDR(label, device_id, ETHVAR_LAN_MSK_IP4) {}
};

/// Use WMI_NET as a wrapper to provide the device_id
class MI_IP4_GWAY : public IMI_IP4_ADDR {
    static constexpr const char *const label = GuiDefaults::ScreenWidth > 240 ? N_("IPv4 Gateway") : N_("GW");

public:
    inline MI_IP4_GWAY(NetDeviceID device_id = {})
        : IMI_IP4_ADDR(label, device_id, ETHVAR_LAN_GW_IP4) {}
};

class MI_MAC_ADDR : public WiInfo<MAC_LEN> {
    static constexpr const char *const label = GuiDefaults::ScreenWidth > 240 ? N_("MAC Address") : N_("MAC");

public:
    MI_MAC_ADDR(NetDeviceID device_id = {});

    void update();

public:
    const NetDeviceID device_id;
};
static_assert(UpdatableMenuItem<MI_MAC_ADDR>);
