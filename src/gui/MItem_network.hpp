#pragma once

#include <WindowMenuItems.hpp>
#include <i18n.h>
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
    constexpr static const char *const label = HAS_MINI_DISPLAY() ? N_("Status") : N_("Wi-Fi Status");

public:
    MI_WIFI_STATUS_t();

    void Loop() override;
};

class MI_WIFI_SSID final : public WiInfoString {
    constexpr static const char *const label = N_("SSID");

public:
    MI_WIFI_SSID();
};

class MI_WIFI_SETUP final : public IWindowMenuItem {
    constexpr static const char *const label = N_("Wi-Fi Wizard");

public:
    MI_WIFI_SETUP();

protected:
    void click(IWindowMenu &window_menu) override;
};

class MI_NET_INTERFACE_t : public MenuItemSwitch {
public:
    MI_NET_INTERFACE_t();
    virtual void OnChange(size_t old_index) override;
};

class MI_HOSTNAME : public WiInfo<HAS_MINI_DISPLAY() ? 16 : config_store_ns::lan_hostname_max_len + 1> {
    // Printer's name within network
    constexpr static const char *const label = N_("Hostname");

public:
    MI_HOSTNAME();

    void Loop() override;

protected:
    void click(IWindowMenu &menu) override;
};

/// Use WMI_NET as a wrapper to provide the device_id
class MI_NET_IP : public MenuItemSwitch {
public:
    MI_NET_IP(NetDeviceID device_id);
    virtual void OnChange(size_t old_index) override;

public:
    const NetDeviceID device_id;
};

class MI_NET_IP_VER_t : public MenuItemSwitch {
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
    using AddrType = ip_addr_t(lan_t::*);

public:
    IMI_IP4_ADDR(const char *label, NetDeviceID device_id, AddrType addr);

    void Loop() override;

public:
    const NetDeviceID device_id;
    const AddrType addr;

protected:
    // Make destructor protected to indicate that we don't need to emit a vtable for this class
    ~IMI_IP4_ADDR() = default;
};

/// Use WMI_NET as a wrapper to provide the device_id
class MI_IP4_ADDR : public IMI_IP4_ADDR {
    static constexpr const char *const label = GuiDefaults::ScreenWidth > 240 ? N_("IPv4 Address") : N_("IP");

public:
    inline MI_IP4_ADDR(NetDeviceID device_id = {})
        : IMI_IP4_ADDR(label, device_id, &lan_t::addr_ip4) {}
};

/// Use WMI_NET as a wrapper to provide the device_id
class MI_IP4_NMSK : public IMI_IP4_ADDR {
    static constexpr const char *const label = GuiDefaults::ScreenWidth > 240 ? N_("IPv4 Netmask") : N_("Mask");

public:
    inline MI_IP4_NMSK(NetDeviceID device_id = {})
        : IMI_IP4_ADDR(label, device_id, &lan_t::msk_ip4) {}
};

/// Use WMI_NET as a wrapper to provide the device_id
class MI_IP4_GWAY : public IMI_IP4_ADDR {
    static constexpr const char *const label = GuiDefaults::ScreenWidth > 240 ? N_("IPv4 Gateway") : N_("Gateway");

public:
    inline MI_IP4_GWAY(NetDeviceID device_id = {})
        : IMI_IP4_ADDR(label, device_id, &lan_t::gw_ip4) {}
};

/// Use WMI_NET as a wrapper to provide the device_id
class MI_IP4_DNS1 : public WiInfo<ADDR_LEN> {
    static constexpr const char *const label = HAS_MINI_DISPLAY() ? N_("DNS") : N_("DNS Server");

public:
    MI_IP4_DNS1();

    void Loop() override;
};

class MI_MAC_ADDR : public WiInfo<MAC_LEN> {
    static constexpr const char *const label = HAS_MINI_DISPLAY() ? N_("MAC") : N_("MAC Address");

public:
    MI_MAC_ADDR(NetDeviceID device_id = {});

    void Loop() override;

public:
    const NetDeviceID device_id;
};
