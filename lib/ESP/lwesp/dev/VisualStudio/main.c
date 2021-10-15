// esp_dev_os.cpp : Defines the entry point for the console application.
//

#include "windows.h"
#include "esp/esp.h"
#include "esp/apps/esp_mqtt_client_api.h"
#include "esp/apps/esp_cayenne.h"

#include "mqtt_client.h"
#include "mqtt_client_api.h"
#include "http_server.h"
#include "station_manager.h"
#include "netconn_client.h"
#include "netconn_server.h"
#include "netconn_server_1thread.h"
#include "string.h"
#include "esp/esp_timeout.h"
#include "lwmem/lwmem.h"
#include "esp_config.h"

static void main_thread(void* arg);
DWORD main_thread_id;

esp_ap_t aps[10];
size_t aps_count;

static espr_t esp_evt(esp_evt_t* evt);
static espr_t esp_conn_evt(esp_evt_t* evt);

esp_sta_info_ap_t connected_ap_info;

#define safeprintf          printf

extern volatile uint8_t esp_ll_win32_driver_ignore_data;

/**
 * \brief           MQTT client info for server
 */
const esp_mqtt_client_info_t
cayenne_mqtt_client_info = {
    .id = "408793d0-3810-11e9-86b5-4fe3d2557533",

    .user = "8a215f70-a644-11e8-ac49-e932ed599553",
    .pass = "26aa943f702e5e780f015cd048a91e8fb54cca28",

    .keep_alive = 10,
};

esp_mqtt_client_api_p cayenne_client;
esp_cayenne_t cayenne;

uint8_t lwmem_region_1[0x4000];
lwmem_region_t lwmem_regions[] = {
    {lwmem_region_1, sizeof(lwmem_region_1)}
};

/**
 * \brief           Program entry point
 */
int
main() {
    printf("App start!\r\n");

    if (!lwmem_assignmem(lwmem_regions, sizeof(lwmem_regions) / sizeof(lwmem_regions[0]))) {
        printf("Could not assign memory for LwMEM!\r\n");
        return -1;
    }

    /* Create start main thread */
    CreateThread(0, 0, (LPTHREAD_START_ROUTINE)main_thread, NULL, 0, &main_thread_id);

    /* Do nothing at this point but do not close the program */
    while (1) {
        esp_delay(1000);
    }
}

/**
 * \brief           Cayenne callback function
 */
espr_t
cayenne_evt_func(esp_cayenne_t* c, esp_cayenne_evt_t* evt) {
    switch (esp_cayenne_evt_get_type(evt)) {
        case ESP_CAYENNE_EVT_CONNECT: {
            printf("Cayenne connected!\r\n");

            /* Once connected, send device status */
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_SYS_MODEL, ESP_CAYENNE_NO_CHANNEL, NULL, NULL, "IoT Board for CayenneAPI");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_SYS_VERSION, ESP_CAYENNE_NO_CHANNEL, NULL, NULL, "v1.0");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_SYS_CPU_MODEL, ESP_CAYENNE_NO_CHANNEL, NULL, NULL, "ARM Cortex-M4");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_SYS_CPU_SPEED, ESP_CAYENNE_NO_CHANNEL, NULL, NULL, "180000000");

#if 0                                    
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_DATA, 30, "temp", "c", "13");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_DATA, 31, "temp", "f", "13");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_DATA, 32, "voltage", "mv", "123");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_DATA, 34, "soil_w_ten", "kpa", "16");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_DATA, 35, "rel_hum", "p", "29");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_DATA, 36, "analog_actuator", NULL, "255");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_DATA, 37, "digital_actuator", "d", "1");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_DATA, 38, "analog_sensor", NULL, "255");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_DATA, 39, "digital_sensor", "d", "0");
            esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_DATA, 40, "digital_sensor", "d", "1");
#endif

            break;
        }
        case ESP_CAYENNE_EVT_DISCONNECT: {
            printf("Cayenne disconnected!\r\n");
            break;
        }
        case ESP_CAYENNE_EVT_DATA: {
            esp_cayenne_msg_t* msg = esp_cayenne_evt_data_get_msg(evt);
            if (msg->topic == ESP_CAYENNE_TOPIC_COMMAND) {
                /* Send callback to user, this part should be handled in user callback */
                /* Written here for test purposes only! */
                esp_cayenne_publish_response(c, msg, ESP_CAYENNE_RESP_OK, NULL);
                esp_cayenne_publish_data(c, ESP_CAYENNE_TOPIC_DATA, msg->channel, NULL, NULL, msg->values[0].value);
            }
            break;
        }
        default: break;
    }
    return espOK;
}

/**
 * \brief           Console input thread
 */
static void
input_thread(void* arg) {
    char buff[128];

#define IS_LINE(s)      (strncmp(buff, (s), sizeof(s) - 1) == 0)

    /* Notify user */
    printf("Start by writing commands..\r\n");

    /* Very simple input */
    while (1) {
        printf(" > ");
        fgets(buff, sizeof(buff), stdin);
        
        if (IS_LINE("join")) {
            esp_sta_join("Majerle WIFI", "majerle_internet_private", NULL, NULL, NULL, 1);
        } else if (IS_LINE("quit")) {
            esp_sta_quit(NULL, NULL, 1);
        } else if (IS_LINE("setip")) {
            esp_ip_t dev_ip;
            dev_ip.ip[0] = 192;
            dev_ip.ip[1] = 168;
            dev_ip.ip[2] = 1;
            dev_ip.ip[3] = 150;
            esp_sta_setip(&dev_ip, NULL, NULL, NULL, NULL, 1);
        } else if (IS_LINE("getip")) {
            esp_sta_getip(NULL, NULL, NULL, NULL, NULL, 1);
        } else if (IS_LINE("dhcpenable")) {
            esp_dhcp_configure(1, 0, 1, NULL, NULL, 1);
        } else if (IS_LINE("dhcpdisable")) {
            esp_dhcp_configure(1, 0, 1, NULL, NULL, 1);
        } else if (IS_LINE("listap")) {
            esp_sta_list_ap(NULL, aps, ESP_ARRAYSIZE(aps), &aps_count, NULL, NULL, 1);
            printf("Detected %d number of access points\r\n", (int)aps_count);
        } else if (IS_LINE("getapinfo")) {
            esp_sta_info_ap_t ap;
            esp_sta_get_ap_info(&ap, NULL, NULL, 1);
        } else if (IS_LINE("apenable")) {
            esp_set_wifi_mode(ESP_MODE_STA_AP, NULL, NULL, 1);
            esp_ap_configure("ESP8266_SSID", "its private", 13, ESP_ECN_WPA2_PSK, 5, 0, NULL, NULL, 1);
        } else if (IS_LINE("apdisable")) {
            esp_set_wifi_mode(ESP_MODE_STA, NULL, NULL, 1);
        } else if (IS_LINE("ping")) {
            uint32_t pingtime;
            esp_ping("example.com", &pingtime, NULL, NULL, 1);
            printf("Ping time result: %d\r\n", (int)pingtime);
        } else if (IS_LINE("hostnameset")) {
            esp_hostname_set("esp_dev", NULL, NULL, 1);
        } else if (IS_LINE("hostnameget")) {
            char hn[20];
            esp_hostname_get(hn, sizeof(hn), NULL, NULL, 1);
            printf("Hostname: %s\r\n", hn);
        } else if (IS_LINE("netconnclient")) {
            esp_sys_sem_t sem;
            esp_sys_sem_create(&sem, 0);
            esp_sys_thread_create(NULL, "netconn_client", (esp_sys_thread_fn)netconn_client_thread, &sem, 0, ESP_SYS_THREAD_PRIO);
            esp_sys_sem_wait(&sem, 0);
            esp_sys_sem_delete(&sem);
        } else if (IS_LINE("netconnserver")) {
            esp_sys_thread_create(NULL, "netconn_server", (esp_sys_thread_fn)netconn_server_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
        } else if (IS_LINE("mqttthread")) {
            esp_sys_thread_create(NULL, "mqtt_client_api", (esp_sys_thread_fn)mqtt_client_api_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
        } else if (IS_LINE("ignoreon")) {
            printf("Ignoring data...\r\n");
            esp_ll_win32_driver_ignore_data = 1;
        } else if (IS_LINE("ignoreoff")) {
            printf("Not ignoring data...\r\n");
            esp_ll_win32_driver_ignore_data = 0;
        } else {
            printf("Unknown input!\r\n");
        }
    }
}

/**
 * \brief           Main thread for init purposes
 */
static void
main_thread(void* arg) {
    esp_ip_t dev_ip;
    char hn[10];
    uint32_t ping_time;

    /* Init ESP library */
    esp_init(esp_evt, 1);

    if (esp_device_is_esp32()) {
        printf("Device is ESP32\r\n");
    }
    if (esp_device_is_esp8266()) {
        printf("Device is ESP8266\r\n");
    }
    /* Start thread to toggle device present */
    //esp_sys_thread_create(NULL, "device_present", (esp_sys_thread_fn)esp_device_present_toggle, NULL, 0, ESP_SYS_THREAD_PRIO);
    esp_hostname_set("abc", NULL, NULL, 1);
    esp_hostname_get(hn, sizeof(hn), NULL, NULL, 1);
    printf("Hostname: %s\r\n", hn);

    /*
     * Try to connect to preferred access point
     *
     * Follow function implementation for more info
     * on how to setup preferred access points for fast connection
     */
    //start_access_point_scan_and_connect_procedure();
    //esp_sys_thread_terminate(NULL);
    //connect_to_preferred_access_point(1);
    esp_sta_autojoin(1, NULL, NULL, 1);
    esp_sta_join("Majerle WIFI", "majerle_internet_private", NULL, NULL, NULL, 1);

    esp_ping("majerle.eu", &ping_time, NULL, NULL, 1);
    printf("Ping time: %d\r\n", (int)ping_time);

    /*
     * Check if device has set IP address
     *
     * This should always pass
     */
    //if (esp_sta_has_ip()) {

    //    esp_ip_t ip;
    //    uint8_t is_dhcp;

    //    esp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp);
    //    printf("Connected to WIFI!\r\n");
    //    printf("Device IP: %d.%d.%d.%d; is DHCP: %d\r\n", (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3], (int)is_dhcp);
    //    esp_sta_setip(&dev_ip, NULL, NULL, 0, NULL, NULL, 1);
    //    esp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp);
    //    printf("Device IP: %d.%d.%d.%d; is DHCP: %d\r\n", (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3], (int)is_dhcp);
    //    esp_dhcp_configure(1, 0, 1, 1, NULL, NULL, 1);
    //    esp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp);
    //    printf("Device IP: %d.%d.%d.%d; is DHCP: %d\r\n", (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3], (int)is_dhcp);
    //}

    //esp_sta_setip(&dev_ip, NULL, NULL, NULL, NULL, 1);
    //esp_dhcp_configure(1, 0, 1, NULL, NULL, 1);

    /* Start server on port 80 */
    //http_server_start();
    //esp_sys_thread_create(NULL, "netconn_client", (esp_sys_thread_fn)netconn_client_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    //esp_sys_thread_create(NULL, "netconn_server", (esp_sys_thread_fn)netconn_server_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    //esp_sys_thread_create(NULL, "netconn_server_single", (esp_sys_thread_fn)netconn_server_1thread_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    //esp_sys_thread_create(NULL, "mqtt_client", (esp_sys_thread_fn)mqtt_client_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    //esp_sys_thread_create(NULL, "mqtt_client_api", (esp_sys_thread_fn)mqtt_client_api_thread, NULL, 0, ESP_SYS_THREAD_PRIO);
    //esp_sys_thread_create(NULL, "mqtt_client_api_cayenne", (esp_sys_thread_fn)mqtt_client_api_cayenne_thread, NULL, 0, ESP_SYS_THREAD_PRIO);

    /*if (esp_cayenne_create(&cayenne, &cayenne_mqtt_client_info, cayenne_evt_func) != espOK) {
        printf("Cannot create new cayenne instance!\r\n");
    } else {

    }*/

    /* Notify user */
    esp_sys_thread_create(NULL, "input", (esp_sys_thread_fn)input_thread, NULL, 0, ESP_SYS_THREAD_PRIO);

    {
        espr_t res;
        esp_pbuf_p pbuf;
        esp_netconn_p client;

        client = esp_netconn_new(ESP_NETCONN_TYPE_TCP);
        if (client != NULL) {
            while (1) {
                res = esp_netconn_connect(client, "10.57.218.181", 123);
                if (res == espOK) {                     /* Are we successfully connected? */
                    printf("Connected to host\r\n");
                    do {
                        res = esp_netconn_receive(client, &pbuf);
                        if (res == espCLOSED) {     /* Was the connection closed? This can be checked by return status of receive function */
                            printf("Connection closed by remote side...\r\n");
                            break;
                        }
                        if (res == espOK && pbuf != NULL) {
                            printf("Received new data packet of %d bytes\r\n", (int)esp_pbuf_length(pbuf, 1));
                            esp_pbuf_free(pbuf);
                            pbuf = NULL;
                        }
                    } while (1);
                } else {
                    printf("Cannot connect to remote host!\r\n");
                }
            }
        }
        esp_netconn_delete(client);             /* Delete netconn structure */
    }

    /* Terminate thread */
    esp_sys_thread_terminate(NULL);
}

/**
 * \brief           Global ESP event function callback
 * \param[in]       evt: Event information
 * \return          \ref espOK on success, member of \ref espr_t otherwise
 */
static espr_t
esp_evt(esp_evt_t* evt) {
    switch (evt->type) {
        case ESP_EVT_INIT_FINISH: {
            /* Device is not present on init */
            //esp_device_set_present(0, NULL, NULL, 0);
            break;
        }
        case ESP_EVT_RESET: {
            if (esp_evt_reset_get_result(evt) == espOK) {
                printf("Reset sequence successful!\r\n");
            } else {
                printf("Reset sequence error!\r\n");
            }
            break;
        }
        case ESP_EVT_RESTORE: {
            if (esp_evt_restore_get_result(evt) == espOK) {
                printf("Restore sequence successful!\r\n");
            } else {
                printf("Restore sequence error!\r\n");
            }
            break;
        }
        case ESP_EVT_AT_VERSION_NOT_SUPPORTED: {
            esp_sw_version_t v_min, v_curr;

            esp_get_min_at_fw_version(&v_min);
            esp_get_current_at_fw_version(&v_curr);

            printf("Current ESP8266 AT version is not supported by library\r\n");
            printf("Minimum required AT version is: %d.%d.%d\r\n", (int)v_min.major, (int)v_min.minor, (int)v_min.patch);
            printf("Current AT version is: %d.%d.%d\r\n", (int)v_curr.major, (int)v_curr.minor, (int)v_curr.patch);
            break;
        }
        case ESP_EVT_WIFI_GOT_IP: {
            printf("Wifi got an IP address.\r\n");
            break;
        }
        case ESP_EVT_WIFI_CONNECTED: {
            printf("Wifi just connected. Read access point information\r\n");
            esp_sta_get_ap_info(&connected_ap_info, NULL, NULL, 0);
            break;
        }
        case ESP_EVT_WIFI_DISCONNECTED: {
            printf("Wifi just disconnected\r\n");
            break;
        }
        case ESP_EVT_STA_INFO_AP: {
            printf("SSID: %s, ch: %d, rssi: %d\r\n",
                esp_evt_sta_info_ap_get_ssid(evt),
                (int)esp_evt_sta_info_ap_get_channel(evt),
                (int)esp_evt_sta_info_ap_get_rssi(evt)
            );
            break;
        }
        case ESP_EVT_WIFI_IP_ACQUIRED: {
            esp_ip_t ip;
            uint8_t is_dhcp;

            printf("WIFI IP ACQUIRED!\r\n");
            if (esp_sta_copy_ip(&ip, NULL, NULL, &is_dhcp) == espOK) {
                printf("Device IP: %d.%d.%d.%d; is DHCP: %d\r\n", (int)ip.ip[0], (int)ip.ip[1], (int)ip.ip[2], (int)ip.ip[3], (int)is_dhcp);
            } else {
                printf("Acquired IP is not valid\r\n");
            }
            
            break;
        }
#if ESP_CFG_MODE_ACCESS_POINT
        case ESP_EVT_AP_CONNECTED_STA: {
            esp_mac_t* mac = esp_evt_ap_connected_sta_get_mac(evt);
            printf("New station connected to ESP's AP with MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
            break;
        }
        case ESP_EVT_AP_DISCONNECTED_STA: {
            esp_mac_t* mac = esp_evt_ap_disconnected_sta_get_mac(evt);
            printf("Station disconnected from ESP's AP with MAC: %02X:%02X:%02X:%02X:%02X:%02X\r\n",
                mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5]);
            break;
        }
        case ESP_EVT_AP_IP_STA: {
            esp_mac_t* mac = esp_evt_ap_ip_sta_get_mac(evt);
            esp_ip_t* ip = esp_evt_ap_ip_sta_get_ip(evt);
            printf("Station received IP address from ESP's AP with MAC: %02X:%02X:%02X:%02X:%02X:%02X and IP: %d.%d.%d.%d\r\n",
                mac->mac[0], mac->mac[1], mac->mac[2], mac->mac[3], mac->mac[4], mac->mac[5],
                ip->ip[0], ip->ip[1], ip->ip[2], ip->ip[3]);
            break;
        }
#endif /* ESP_CFG_MODE_ACCESS_POINT */
        default: break;
    }
    return espOK;
}

static espr_t
esp_conn_evt(esp_evt_t* evt) {
    static char data[] = "test data string\r\n";
    esp_conn_p conn;

    conn = esp_conn_get_from_evt(evt);

    switch (evt->type) {
        case ESP_EVT_CONN_ACTIVE: {
            printf("Connection active!\r\n");
            printf("Send API call: %d\r\n", (int)esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0));
            printf("Send API call: %d\r\n", (int)esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0));
            printf("Send API call: %d\r\n", (int)esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0));
            printf("Close API call: %d\r\n", (int)esp_conn_close(conn, 0));
            printf("Send API call: %d\r\n", (int)esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0));
            printf("Close API call: %d\r\n", (int)esp_conn_close(conn, 0));

            /*
            esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0);
            esp_conn_send(conn, data, sizeof(data) - 1, NULL, 0);
            */
            break;
        }
        case ESP_EVT_CONN_SEND: {
            espr_t res = esp_evt_conn_send_get_result(evt);
            if (res == espOK) {
                printf("Connection data sent!\r\n");
            } else {
                printf("Connect data send error!\r\n");
            }
            break;
        }
        case ESP_EVT_CONN_RECV: {
            esp_pbuf_p pbuf = esp_evt_conn_recv_get_buff(evt);
            esp_conn_p conn = esp_evt_conn_recv_get_conn(evt);
            printf("\r\nConnection data received: %d / %d bytes\r\n",
                (int)esp_pbuf_length(pbuf, 1),
                (int)esp_conn_get_total_recved_count(conn)
            );
            esp_conn_recved(conn, pbuf);
            break;
        }
        case ESP_EVT_CONN_CLOSE: {
            printf("Connection closed!\r\n");
            //esp_conn_start(NULL, ESP_CONN_TYPE_TCP, "majerle.eu", 80, NULL, esp_conn_evt, 0);
            break;
        }
        case ESP_EVT_CONN_ERROR: {
            printf("Connection error!\r\n");
            break;
        }
        default: break;
    }
    return espOK;
}

#if ESP_CFG_MEM_CUSTOM && 0

void *
esp_mem_malloc(size_t size) {
    void* ptr;

    while (WaitForSingleObject(allocation_mutex, INFINITE) != WAIT_OBJECT_0) {}
    ptr = malloc(size);
    ReleaseMutex(allocation_mutex);
    return ptr;
}

void *
esp_mem_realloc(void* ptr, size_t size) {
    void* p;

    while (WaitForSingleObject(allocation_mutex, INFINITE) != WAIT_OBJECT_0) {}
    p = realloc(ptr, size);
    ReleaseMutex(allocation_mutex);
    return p;
}

void *
esp_mem_calloc(size_t num, size_t size) {
    void* ptr;

    while (WaitForSingleObject(allocation_mutex, INFINITE) != WAIT_OBJECT_0) {}
    ptr = calloc(num, size);
    ReleaseMutex(allocation_mutex);
    return ptr;
}

void
esp_mem_free(void* ptr) {
    while (WaitForSingleObject(allocation_mutex, INFINITE) != WAIT_OBJECT_0) {}
    free(ptr);
    ReleaseMutex(allocation_mutex);
}
#endif /* ESP_CFG_MEM_CUSTOM */
