#include "wui.h"
#include "netif_settings.h"

#include "marlin_client.hpp"
#include "wui_api.h"
#include "ethernetif.h"
#include "espif.h"
#include <option/mdns.h>
#if MDNS()
    #include "mdns/mdns.h"
#endif

#include <otp.hpp>
#include <mbedtls/sha256.h>
#include <tasks.hpp>

#include "sntp_client.h"
#include "log.h"
#include "ini_handler.h"

#include <atomic>
#include <array>
#include <cstring>
#include <cassert>
#include <lwip/ip.h>
#include <lwip/dns.h>
#include <lwip/tcp.h>
#include <lwip/altcp_tcp.h>
#include <lwip/netifapi.h>
#include <lwip/netif.h>
#include <lwip/tcpip.h>
#include <common/freertos_mutex.hpp>
#include <mutex>
#include "http_lifetime.h"
#include "main.h"
#include <ccm_thread.hpp>
#include "tasks.hpp"

#include "netdev.h"

#include "otp.hpp"
#include <config_store/store_instance.hpp>
#include <nhttp/server.h>
#include <random.h>

LOG_COMPONENT_DEF(WUI, LOG_SEVERITY_DEBUG);
LOG_COMPONENT_DEF(Network, LOG_SEVERITY_INFO);

using std::unique_lock;

#define LOOP_EVT_TIMEOUT 500UL

// Avoid confusing character pairs ‒ 1/l/I, 0/O.
static char charset[] = "abcdefghijkmnopqrstuvwxyzABCDEFGHJKLMNPQRSTUVWXYZ23456789";

void wui_generate_password(char *password, uint32_t length) {
    // One less, as the above contains '\0' at the end which we _do not_ want to generate.
    const uint32_t charset_length = sizeof(charset) / sizeof(char) - 1;
    uint32_t i = 0;

    while (i < length - 1) {
        uint32_t random = 0;
        if (!rand_u_secure(&random)) {
            // Failure in RNG, reset the password to „login disabled“.
            password[0] = 0;
            return;
        }
        password[i++] = charset[random % charset_length];
    }
    password[i] = 0;
}

void wui_store_apikey(char *password, uint32_t length) {
    config_store().prusalink_password.set(password, length);
}

void wui_store_user_password(char *password, uint32_t length) {
    config_store().prusalink_user_password.set(password, length);
}

int wui_ini_handler(void *user, const char *section, const char *name, const char *value) {
    if (user == nullptr || section == nullptr || name == nullptr || value == nullptr) {
        // Stop parsing
        return 0;
    }

    // Looking for our section
    if (strcmp("prusalink", section)) {
        return 1;
    }

    uint8_t *key_flags = (uint8_t *)user;
    size_t len = strlen(value);

    if (strcmp(name, "apikey") == 0) {
        if (len <= config_store_ns::pl_password_size - 1) {
            *key_flags |= 1;
            // Store new key if different
            if (strncmp(value, config_store().prusalink_password.get_c_str(), config_store_ns::pl_password_size)) {
                wui_store_apikey((char *)value, config_store_ns::pl_password_size);
            }
        }
    } else if (strcmp(name, "psw") == 0) {
        if (len <= config_store_ns::pl_password_size - 1) {
            *key_flags |= 2;
            // Store new psw if different
            if (strncmp(value, config_store().prusalink_user_password.get_c_str(), config_store_ns::pl_password_size)) {
                wui_store_user_password((char *)value, config_store_ns::pl_password_size);
            }
        }
    }

    return 1;
}

uint8_t wui_load_ini_file() {
    uint8_t keys_found = 0;
    (void)ini_load_file(wui_ini_handler, &keys_found);
    return keys_found;
}

namespace {

void prusalink_password_init(void) {
    if (!strcmp(config_store().prusalink_password.get().data(), "")) {
        // Empty string -- init
        char password[config_store_ns::pl_password_size] = { 0 };
        wui_generate_password(password, config_store_ns::pl_password_size);
        wui_store_apikey(password, config_store_ns::pl_password_size);
        // Init user password to api-key (legacy operation)
        wui_store_user_password(password, config_store_ns::pl_password_size);
    }
    return;
}

// This is the top-level manager of network settings and interfaces.
//
// It is responsible for bringing up all the interfaces with the right settings
// and reloading the settings as needed. It doesn't do much functionality on
// the network itself (it does DHCP, which is part of its core
// responsibilities, it starts the http server and ntp client ‒ starting them
// is probably part of the management responsibilities, but some kind of hooks
// mechanism might be better in the future).
//
// Technicalities:
//
// * In practice, the thing needs to be a singleton. We need that because
//   interrupts need to be sent somewhere, the rest of the printer needs to have
//   something to contact, etc. But the logic is not singletonish inside.
// * The external API is legacy ‒ taken from the original implementation. This
//   is transitional measure and some kind of newer & nicer interface will be
//   done later on. Maybe as some kind of facade on top of it.
// * This lives in its own thread/task. It handles both the reconfiguration and
//   reading packets from both our interfaces. Nevertheless, sending of packets
//   happens from whatever task they happen to be created. Similarly, accessing
//   the current configuration can happen from whatever.
// * We are lazy and do any kind of reconfiguration by shutting everything
//   down, reading new state from EEPROM and then bringing it up again. When
//   someone wants to change something, it is first written into EEPROM, then a
//   reload is signalled and happens in our thread.
// * While writing configuration is synchronized through the write to the
//   EEPROM, reading of the configuration needs to happen inside a lock. This is
//   not yet written, but we at least have a well defined place where it goes
//   inside this class.
class NetworkState {
public:
    enum NetworkAction {
        CoreInitDone = 1 << 0,
        Reconfigure = 1 << 1,
        EthInitDone = 1 << 2,
        EthData = 1 << 3,
        EspInitDone = 1 << 4,
        EspData = 1 << 5,
        TriggerNtp = 1 << 6,
        HealthCheck = 1 << 7,
#if MDNS()
        MdnsInitCheck = 1 << 8,
#endif
    };

private:
    // Allow running link, marlin client, etc?
    //
    // Set only before starting, then read only.
    static bool allow_full;
    freertos::Mutex mutex;
    enum class Mode {
        Off,
        Static,
        DHCP,
    };

    struct Iface {
        netif dev = {};
        netif_config_t desired_config = {};
#if MDNS()
        bool mdns_initialized = false;
#endif
    };

    // If ESP is not working for a minute, try to reset it if it reconnects.
    // (we probably could implement some kind of back-off strategy or whatever,
    // but let's keep it simple for now).
    static const constexpr uint32_t RESET_FAULTY_AFTER = 60 * 1000;

    std::array<Iface, NETDEV_COUNT> ifaces;
    ap_entry_t ap = { "", "" };
    uint32_t last_esp_ok;

    TaskHandle_t network_task;
    // This makes it a singleton. This one is accessed from outside.
    //
    // It is set only once at the start, when the thread is started.
    static std::atomic<NetworkState *> instance;
    uint32_t active = NETDEV_NODEV_ID;

    Mode iface_mode(const Iface &iface) {
        // Assumes already locked
        const auto flag = iface.desired_config.lan.flag;
        if (IS_LAN_OFF(flag)) {
            return Mode::Off;
        }

        if (IS_LAN_DHCP(flag)) {
            return Mode::DHCP;
        }

        assert(IS_LAN_STATIC(flag));

        return Mode::Static;
    }

    void link_callback(netif &iface) {
        status_callback(iface);
        unique_lock lock(mutex);
        uint32_t action = 0;
        if (&iface == &ifaces[NETDEV_ETH_ID].dev) {
            if (netif_is_link_up(&iface)) {
                log_info(Network, "Eth link went up");
                action = EthInitDone;
            } else {
                log_info(Network, "Eth link went down");
            }
        } else if (&iface == &ifaces[NETDEV_ESP_ID].dev) {
            if (netif_is_link_up(&iface)) {
                log_info(Network, "ESP link went up");
                action = EspInitDone;
            } else {
                log_info(Network, "ESP link went down");
            }
        } else {
            assert(0);
        }
        xTaskNotify(network_task, action, eSetBits);
    }
    static void link_callback_raw(struct netif *iface) {
        static_cast<NetworkState *>(iface->state)->link_callback(*iface);
    }
    void status_callback(netif &iface) {
        if (&iface == &ifaces[NETDEV_ETH_ID].dev) {
            unique_lock lock(mutex);
            // Or, shall we say copy info out?
            ethernetif_update_config(&iface);
        } else if (&iface == &ifaces[NETDEV_ESP_ID].dev) {
            // Nothing to be done for wifi here
        } else {
            assert(0); /* Unknown interface. */
        }
    }
    static void status_callback_raw(struct netif *iface) {
        static_cast<NetworkState *>(iface->state)->status_callback(*iface);
    }

#if MDNS()
    static void mdns_netif_init(netif *iface) {
        iface->flags |= NETIF_FLAG_IGMP;
        igmp_start(iface);
        //  TODO: Any way to handle errors? Can they even happen?
        mdns_resp_add_netif(iface);
        if (config_store().prusalink_enabled.get() == 1) {
            mdns_resp_add_service_prusalink(iface);
        }
    }
#endif

    void tcpip_init_done() {
#if MDNS()
        if (allow_full) {
            igmp_init();
            mdns_resp_init();
        }
#endif

        // We assume this callback is run from within the tcpip thread, not our
        // own!
        if (netif_add_noaddr(&ifaces[NETDEV_ETH_ID].dev, this, ethernetif_init, tcpip_input)) {
            netif_set_link_callback(&ifaces[NETDEV_ETH_ID].dev, link_callback_raw);
            netif_set_status_callback(&ifaces[NETDEV_ETH_ID].dev, status_callback_raw);
        } else {
            // FIXME: ???
        }
        if (netif_add_noaddr(&ifaces[NETDEV_ESP_ID].dev, this, espif_init, tcpip_input)) {
            netif_set_link_callback(&ifaces[NETDEV_ESP_ID].dev, link_callback_raw);
            netif_set_status_callback(&ifaces[NETDEV_ESP_ID].dev, status_callback_raw);
        } else {
            // FIXME: ???
        }
        if (allow_full) {
            wui_marlin_client_init();
        }

        // Won't fail with eSetBits
        xTaskNotify(network_task, CoreInitDone, eSetBits);
    }
    static void tcpip_init_done_raw(void *me) {
        static_cast<NetworkState *>(me)->tcpip_init_done();
    }
    void post_init(uint32_t face_index) {
        // Already locked by the caller.

        Iface &iface = ifaces[face_index];

        // FIXME: Error handling
        switch (iface_mode(iface)) {
        case Mode::DHCP:
            log_info(Network, "Starting DHCP on iface: %" PRIu32, face_index);
            if (err_t err = netifapi_dhcp_start(&iface.dev); err != ERR_OK) {
                log_warning(Network, "dhcp_start failed on iface: %" PRIu32 " with: %d", face_index, err);
            }
            break;
        case Mode::Static: {
            log_info(Network, "Setting static IP on iface: %" PRIu32, face_index);
            netif_config_t cfg;
            { // Scope for the lock
                unique_lock lock(mutex);
                // Yes, make a copy (for thread safety)
                cfg = iface.desired_config;
            }

            // FIXME: The DNS setting is _global_ for the whole network
            // stack, not only to specific interface. How do we want the
            // config of multiple interfaces to interact? Take it from the
            // selected interface?
            dns_setserver(0, &cfg.dns1_ip4);
            dns_setserver(1, &cfg.dns2_ip4);
            netifapi_netif_set_addr(&iface.dev, &cfg.lan.addr_ip4, &cfg.lan.msk_ip4, &cfg.lan.gw_ip4);
            netifapi_dhcp_inform(&iface.dev);
            break;
        }
        default:;
            // The device got turned off in the meantime/was off to start
            // with. Leave it be.
        }
    }

    void set_down(netif &iface) {
#if MDNS()
        netifapi_netif_common(&iface, nullptr, mdns_resp_remove_netif);
#endif
        // Already locked by the caller.
        netifapi_dhcp_stop(&iface);
        netifapi_netif_set_link_down(&iface);
        // FIXME: Error handling
        netifapi_netif_set_down(&iface);
    }

    void set_up(netif &iface) {
        // Already locked by the caller.
        netifapi_netif_set_link_up(&iface);
        // FIXME: Error handling
        netifapi_netif_set_up(&iface);
    }

    void join_ap() {
        unique_lock lock(mutex);
        const char *passwd = ap.pass[0] == '\0' ? NULL : ap.pass;
        espif_join_ap(ap.ssid, passwd);
    }

    void reconfigure() {
        log_info(Network, "Reconfigure");
        // Read some stuff from the eeprom.

        // Lock (even the desired config can be read from other threads, eg. the tcpip_thread from a callback :-(
        // (using unique_lock instead of scoped_lock as at other places, we need "pause")
        unique_lock lock(mutex);
        const uint32_t active_local = config_store().active_netdev.get();
        // Store into the atomic variable, but keep working with the stack copy.
        active = active_local;
        load_net_params(&ifaces[NETDEV_ETH_ID].desired_config, nullptr, NETDEV_ETH_ID);
        load_net_params(&ifaces[NETDEV_ESP_ID].desired_config, &ap, NETDEV_ESP_ID);

        // First, bring everything down. Then bring whatever is enabled up.
        for (auto &iface : ifaces) {
            // "Pause" the mutex for a while, this calls callbacks that also lock.
            lock.unlock();
            set_down(iface.dev);
            lock.lock();
#if MDNS()
            iface.mdns_initialized = false;
#endif

            // FIXME: Track down where exactly the hostname is stored, when it
            // can change, how it is synchronized with threads (or isn't!).
            iface.dev.hostname = iface.desired_config.hostname;

            if (&iface == &ifaces[NETDEV_ESP_ID]) {
                // The ESP interface is a bit weird. It doesn't support (yet)
                // disconnecting from AP, so we reset it. Then we have to wait
                // for it to become ready to join the AP, etc, which is done
                // asynchronously in the event loop.
                espif_reset();
            } else {
                // Other interfaces can just be turned on and be done with them.
                // "Pause" the mutex for a while, this calls callbacks that also lock.
                lock.unlock();
                set_up(iface.dev);
                lock.lock();
            }
        }

        if (active_local < ifaces.size()) {
            netifapi_netif_set_default(&ifaces[active_local].dev);
        }

        lock.unlock();

        if (allow_full && config_store().prusalink_enabled.get() == 1) {
            httpd_instance()->start();
        } else {
            httpd_instance()->stop();
        }
    }

    void run() __attribute__((noreturn)) {
        // Note: this is the only thing to initialize now, rest is after the tcpip
        // thread starts.
        tcpip_init(tcpip_init_done_raw, this);

        if (allow_full) {
            prusalink_password_init();
        }

        // During init, we store the events for later. Therefore, we accumulate
        // them until consumed.
        uint32_t events = TriggerNtp;
        bool initialized = false;

        uint32_t last_poll = 0;

        while (true) {
            events |= ulTaskNotifyTake(pdTRUE, LOOP_EVT_TIMEOUT);

            // The original code polled the packet sources from time to time
            // even if there was no interrupt. Not sure if there's a specific
            // reason for that, but we are keeping the legacy functionality to
            // be on the safe side ‒ doing an extra check won't hurt us.
            const uint32_t now = sys_now();
            if (now - last_poll >= LOOP_EVT_TIMEOUT) {
                last_poll = now;
                events |= EspData | HealthCheck | EthData | TriggerNtp;
#if MDNS()
                if (allow_full) {
                    events |= MdnsInitCheck;
                }
#endif
            }

            if (events & CoreInitDone) {
                // Chain the first reconfiguration to bring everything up as needed.
                // (No need to go through the notification.)
                events |= Reconfigure;
                initialized = true;
                TaskDeps::provide(TaskDeps::Dependency::networking_ready);
            }

            // Note: This is allowed even before we are fully initialized. This
            // is on purpose, because the initialization of ESP absolutely
            // needs the communication.
            //
            // That's OK, because internally it can protect itself, it tracks
            // if it is or isn't initialized.
            if (events & EspData) {
                events &= ~EspData;

                espif_input_once(&ifaces[NETDEV_ESP_ID].dev);

                // Delayed init, after the ESP told us it is ready and gave us a MAC address.
                // If we are reconfiguring don't send old connection information, wait for next loop and new ap info.
                if (iface_mode(ifaces[NETDEV_ESP_ID]) != Mode::Off && espif_need_ap() && !(events & Reconfigure)) {
                    join_ap();
                    set_up(ifaces[NETDEV_ESP_ID].dev);
                }
            }

            if (!initialized) {
                // Keep the events unprocessed for now, until the init is fully
                // done.
                continue;
            }

            if (events & Reconfigure) {
                // Invoked directly after start and every time something in eeprom changes.
                reconfigure();
            }

            if (events & EthInitDone) {
                post_init(NETDEV_ETH_ID);
            }

            if (events & EspInitDone) {
                post_init(NETDEV_ESP_ID);
            }

            if (events & EthData) {
                ethernetif_input_once(&ifaces[NETDEV_ETH_ID].dev);
            }

            if (events & TriggerNtp) {
                // TODO: This does some code gymnastics inside to track changes
                // of network configuration. Consider cleaning that up and
                // integrating into some kind of up/down mechanism.
                sntp_client_step();
            }

            if (events & HealthCheck) {
                const bool was_alive = espif_tick();

                // It's OK if the ESP is turned off on purpose or if it's up and running.
                const bool esp_ok = (iface_mode(ifaces[NETDEV_ESP_ID]) == Mode::Off || ap.ssid[0] == '\0' || (espif_link() && was_alive) || espif::scan::is_running());

                if (esp_ok) {
                    last_esp_ok = now;
                }

                const uint32_t faulty_for = now - last_esp_ok;

                if (faulty_for >= RESET_FAULTY_AFTER) {
                    log_warning(Network, "ESP not responsive, resetting");
                    // It's not OK for a long time. Try resetting it if that helps.
                    espif_reset();
                    last_esp_ok = now;
                }
            }

#if MDNS()
            if (events & MdnsInitCheck) {
                // We can afford to have only one interface active for the responder.
                //
                // Doing a check through all the interfaces out of
                // overabundance of caution - making sure we don't start the
                // second MDNS instance in case the active netdev changes and
                // we didn't _yet_ have time to call reconfigure and similar
                // situations (that would introduce a risk of exhausting some
                // resource - like the number of timeouts, where the code is
                // not prepared for the callback not being run eventually,
                // leading to some inconsistent internal state).
                bool any_active = false;
                for (const auto &iface : ifaces) {
                    if (iface.mdns_initialized) {
                        any_active = true;
                        break;
                    }
                }

                if (!any_active) {
                    const uint32_t active = config_store().active_netdev.get();
                    if (active < ifaces.size() && netif_ip4_addr(&ifaces[active].dev)->addr != 0) {
                        // Wait with initialization until we get an IP address.
                        ifaces[active].mdns_initialized = true;
                        netifapi_netif_common(&ifaces[active].dev, mdns_netif_init, nullptr);
                    }
                }
            }
#endif

            events = 0;
        }
    }
    static void task_main(const void *) {
        // Initialize our own thread ID.
        NetworkState state;
        state.run();
    }

    template <class F>
    static void with_iface(uint32_t netdev_id, F &&f) {
        NetworkState *state = instance;
        if (state != nullptr && netdev_id < state->ifaces.size()) {
            unique_lock lock(state->mutex);
            f(state->ifaces[netdev_id].dev, *state);
        }
    }
    bool netif_link(uint32_t netdev_id) {
        if (netdev_id == NETDEV_ETH_ID) {
            return ethernetif_link(&ifaces[NETDEV_ETH_ID]);
        }
        if (netdev_id == NETDEV_ESP_ID) {
            return espif_link();
        }
        assert(0);
        return false;
    }

public:
    NetworkState() {
        network_task = osThreadGetId();
        assert(instance == nullptr);
        instance = this;
        last_esp_ok = sys_now();
    }
    static void run_task(bool allow_full) {
        NetworkState::allow_full = allow_full;
        osThreadCCMDef(network, task_main, TASK_PRIORITY_WUI, 0, 1024);
        osThreadCreate(osThread(network), nullptr);
    }
    static void notify(NetworkAction action) {
        // Read out of the atomic variable
        NetworkState *state = instance;
        if (state != nullptr) {
            xTaskNotifyFromISR(state->network_task, action, eSetBits, NULL);
        }
    }

    static void get_addresses(uint32_t netdev_id, lan_t *config) {
        memset(config, 0, sizeof *config);
        with_iface(netdev_id, [&](netif &iface, NetworkState &) {
            config->addr_ip4.addr = netif_ip4_addr(&iface)->addr;
            config->msk_ip4.addr = netif_ip4_netmask(&iface)->addr;
            config->gw_ip4.addr = netif_ip4_gw(&iface)->addr;
        });
    }

    static bool get_mac(uint32_t netdev_id, uint8_t mac[6]) {
        NetworkState *state = instance;
        if (netdev_id == NETDEV_ETH_ID) {
            // TODO: Why not to copy address from netif? Maybe because we need
            // it sooner than when it's initialized?
            memcpy(mac, otp_get_mac_address()->mac, sizeof(otp_get_mac_address()->mac));
            return true;
        } else if (state != nullptr && netdev_id == NETDEV_ESP_ID) {
            unique_lock lock(state->mutex);
            if (esp_fw_state() == EspFwState::Ok) {
                memcpy(mac, state->ifaces[NETDEV_ESP_ID].dev.hwaddr, state->ifaces[NETDEV_ESP_ID].dev.hwaddr_len);
                return true;
            } else {
                return false;
            }
        } else {
            memset(mac, 0, sizeof(otp_get_mac_address()->mac));
            return false;
        }
    }

    static void get_hostname(uint32_t netdev_id, char *buffer, size_t buffer_len) {
        memset(buffer, 0, buffer_len);
        with_iface(netdev_id, [&](netif &iface, NetworkState &) {
            strlcpy(buffer, iface.hostname, buffer_len);
        });
    }

    static netdev_status_t get_status(uint32_t netdev_id) {
        netdev_status_t status = NETDEV_NETIF_DOWN;
        with_iface(netdev_id, [&](netif &iface, NetworkState &instance) {
            if (netif_is_link_up(&iface)) {
                if (instance.netif_link(netdev_id)) {
                    status = netif_ip4_addr(&iface)->addr != 0 ? NETDEV_NETIF_UP : NETDEV_NETIF_NOADDR;
                } else {
                    status = NETDEV_UNLINKED;
                }
            }
        });
        return status;
    }
    static uint32_t get_active() {
        NetworkState *state = instance;
        if (state != nullptr) {
            unique_lock lock(state->mutex);
            uint32_t active = state->active;
            return active;
        } else {
            return NETDEV_NODEV_ID;
        }
    }
};

std::atomic<NetworkState *> NetworkState::instance = nullptr;
bool NetworkState::allow_full = false;

} // namespace

void start_network_task(bool allow_full) {
    NetworkState::run_task(allow_full);
}

const char *wui_get_apikey() {
    return config_store().prusalink_password.get_c_str();
}

const char *wui_get_user_password() {
    return config_store().prusalink_user_password.get_c_str();
}

void notify_esp_data() {
    NetworkState::notify(NetworkState::NetworkAction::EspData);
}

void notify_ethernet_data() {
    NetworkState::notify(NetworkState::NetworkAction::EthData);
}

void netdev_get_ipv4_addresses(uint32_t netdev_id, lan_t *config) {
    NetworkState::get_addresses(netdev_id, config);
}

bool netdev_get_MAC_address(uint32_t netdev_id, uint8_t mac[6]) {
    return NetworkState::get_mac(netdev_id, mac);
}

void netdev_get_hostname(uint32_t netdev_id, char *buffer, size_t buffer_len) {
    NetworkState::get_hostname(netdev_id, buffer, buffer_len);
}

netdev_status_t netdev_get_status(uint32_t netdev_id) {
    return NetworkState::get_status(netdev_id);
}

uint32_t netdev_get_active_id() {
    return NetworkState::get_active();
}

void notify_reconfigure() {
    NetworkState::notify(NetworkState::NetworkAction::Reconfigure);
}

void netdev_set_active_id(uint32_t netdev_id) {
    assert(netdev_id <= NETDEV_COUNT);

    const auto target = static_cast<uint8_t>(netdev_id & 0xFF);
    if (config_store().active_netdev.get() != target) {
        config_store().active_netdev.set(target);
        notify_reconfigure();
    }
}

namespace {

template <class F>
void modify_flag(uint32_t netdev_id, F &&f) {
    assert(netdev_id == NETDEV_ETH_ID || netdev_id == NETDEV_ESP_ID);

    // Read it from the EEPROM, not from the state. For two reasons:
    // * While it likely can't happen, it's unclear what should happen if the
    //   state wasn't initiated yet.
    // * The config in the state is delayed (reloaded in reconfigure), we want
    //   as fresh value as possible. This still leaves the possibility of a
    //   race condition (two threads messing with the same variable), but that
    //   is unlikely.
    const uint8_t old = netdev_id == NETDEV_ETH_ID ? config_store().lan_flag.get() : config_store().wifi_flag.get();
    uint8_t flag = f(old);
    if (old != flag) {
        netdev_id == NETDEV_ETH_ID ? config_store().lan_flag.set(flag) : config_store().wifi_flag.set(flag);
        notify_reconfigure();
    }
}

} // namespace

void netdev_set_static(uint32_t netdev_id) {
    modify_flag(netdev_id, [](uint8_t flag) -> uint8_t {
        CHANGE_FLAG_TO_STATIC(flag);
        TURN_FLAG_ON(flag);
        return flag;
    });
}

void netdev_set_dhcp(uint32_t netdev_id) {
    modify_flag(netdev_id, [](uint8_t flag) -> uint8_t {
        CHANGE_FLAG_TO_DHCP(flag);
        TURN_FLAG_ON(flag);
        return flag;
    });
}

// Support for enable disable device (i.e. to disable wifi)
void netdev_set_enabled(const uint32_t netdev_id, const bool enabled) {
    modify_flag(netdev_id, [&enabled](uint8_t flag) -> uint8_t {
        if (enabled) {
            TURN_FLAG_ON(flag);
        } else {
            TURN_FLAG_OFF(flag);
        }
        return flag;
    });
}

bool netdev_is_enabled([[maybe_unused]] const uint32_t netdev_id) {
    const uint8_t flag = config_store().wifi_flag.get();
    return IS_LAN_ON(flag);
}

netdev_ip_obtained_t netdev_get_ip_obtained_type(uint32_t netdev_id) {
    // FIXME: This API is subtly wrong. What if the device is off or not exist?
    //
    // Defaulting to DHCP is done for historical reasons, but feels weird.
    if (netdev_id < NETDEV_COUNT) {
        uint8_t extract = 0;
        // We don't actually _modify_ it, just reuse the code.
        modify_flag(netdev_id, [&](uint8_t flag) {
            extract = flag;
            return flag;
        });

        return IS_LAN_DHCP(extract) ? NETDEV_DHCP : NETDEV_STATIC;
    } else {
        return NETDEV_DHCP;
    }
}
