/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "esp_wifi.h"
#include "esp_aio.h"
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "lwip/pbuf.h"
#include "lwip/netif.h"

#include "driver/gpio.h"
#include "driver/uart.h"

// INTRON
// 0 as uint8_t
// hw addr LEN as uint8_t
// hw addr data bytes
#define MSG_DEVINFO 0

// INTRON
// 1 as uint8_t
// link up as bool (uint8_t)
#define MSG_LINK 1

// INTRON
// 2 as uint8_t
// ssid size as uint8_t
// ssid bytes
// pass size as uint8_t
// pass bytes
#define MSG_CLIENTCONFIG 2

// INTRON
// 3 as uint8_t
// LEN as uint32_t
// DATA
#define MSG_PACKET 3




/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
// #define EXAMPLE_ESP_WIFI_SSID      "esptest"
// #define EXAMPLE_ESP_WIFI_PASS      "lwesp8266"
#define EXAMPLE_ESP_MAXIMUM_RETRY  CONFIG_ESP_MAXIMUM_RETRY

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void sendLink(uint8_t up);

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        sendLink(0);
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED) {
        sendLink(1);
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:%s",
                 ip4addr_ntoa(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void) {
    s_wifi_event_group = xEventGroupCreate();

    tcpip_adapter_init();

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // wifi_config_t wifi_config = {
    //     .sta = {
    //         .ssid = EXAMPLE_ESP_WIFI_SSID,
    //         .password = EXAMPLE_ESP_WIFI_PASS
    //     },
    // };

    // /* Setting a password implies station will connect to all security modes including WEP/WPA.
    //     * However these modes are deprecated and not advisable to be used. Incase your Access point
    //     * doesn't support WPA2, these mode can be enabled by commenting below line */

    // if (strlen((char *)wifi_config.sta.password)) {
    //     wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    // }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    // ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    // ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    // EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
    //         WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
    //         pdFALSE,
    //         pdFALSE,
    //         portMAX_DELAY);

    // /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
    //  * happened. */
    // if (bits & WIFI_CONNECTED_BIT) {
    //     printf("WIFI CONNECTED");
    //     // ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
    //     //          EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    // } else if (bits & WIFI_FAIL_BIT) {
    //     printf("WIFI FAILED TO CONNECT");
    //     // ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
    //     //          EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    // } else {
    //     printf("!!!! UNEXPECTED");
    //     ESP_LOGE(TAG, "UNEXPECTED EVENT");
    // }
 
    // ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    // ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    // vEventGroupDelete(s_wifi_event_group);
}

#if LWIP_NETIF_STATUS_CALLBACK
static void netif_status_callback(struct netif *nif) {
  printf("PPPNETIF: %c%c%d is %s\n", nif->name[0], nif->name[1], nif->num,
         netif_is_up(nif) ? "UP" : "DOWN");
#if LWIP_IPV4
  printf("IPV4: Host at %s ", ip4addr_ntoa(netif_ip4_addr(nif)));
  printf("mask %s ", ip4addr_ntoa(netif_ip4_netmask(nif)));
  printf("gateway %s\n", ip4addr_ntoa(netif_ip4_gw(nif)));
#endif /* LWIP_IPV4 */
#if LWIP_IPV6
  printf("IPV6: Host at %s\n", ip6addr_ntoa(netif_ip6_addr(nif, 0)));
#endif /* LWIP_IPV6 */
#if LWIP_NETIF_HOSTNAME
  printf("FQDN: %s\n", netif_get_hostname(nif));
#endif /* LWIP_NETIF_HOSTNAME */
}
#endif /* LWIP_NETIF_STATUS_CALLBACK */

static const char INTRON[] = {'U', 'N', 'U'};

err_t wifi_input(struct pbuf *p, struct netif *inp) {
    // printf("Printing packet to UART\n\r");
    uart_write_bytes(UART_NUM_0, INTRON, sizeof(INTRON));
    const uint8_t t = MSG_PACKET;
    const uint32_t l = p->len;
    uart_write_bytes(UART_NUM_0, (const char*)&t, 1);
    uart_write_bytes(UART_NUM_0, (const char*)&l, sizeof(uint32_t));
    uart_write_bytes(UART_NUM_0, (const char*)p->payload, p->len);
    // printf("Packet UART out done\n\r");
    pbuf_free(p);
    return 0;
}

err_t (*out)(struct netif *netif, struct pbuf *p) = 0;
struct netif *wifi_net_if = 0;

static err_t dummy_out(struct netif *netif, struct pbuf *p) {
    // printf("DUMMY OUT\n\r");
    return 0;
}


static void sendDeviceInfo() {
    if(!wifi_net_if) {
        printf("Net if not available !!!\n\r");
        return;
    }

    printf("Sending device info\n\r");
    uart_write_bytes(UART_NUM_0, INTRON, sizeof(INTRON));
    const uint8_t t = MSG_DEVINFO;
    uart_write_bytes(UART_NUM_0, (const char*)&t, 1);
    uart_write_bytes(UART_NUM_0, (const char*)&wifi_net_if->hwaddr_len, sizeof(uint8_t));
    uart_write_bytes(UART_NUM_0, (const char*)&wifi_net_if->hwaddr, wifi_net_if->hwaddr_len);
}

static void sendLink(uint8_t up) {
    printf("Sending link status: %d\n\r", up);
    uart_write_bytes(UART_NUM_0, INTRON, sizeof(INTRON));
    const uint8_t t = MSG_LINK;
    uart_write_bytes(UART_NUM_0, (const char*)&t, 1);
    uart_write_bytes(UART_NUM_0, (const char*)&up, sizeof(uint8_t));
}



static void waitForIntron() {
    // printf("Waiting for intron\n\r");
    uint pos = 0;
    while(pos < sizeof(INTRON)) {
        char c;
        int read = uart_read_bytes(UART_NUM_0, (uint8_t*)&c, 1, portMAX_DELAY);
        if(read == 1) {
            if (c == INTRON[pos]) {
                pos++;
            } else {
                printf("Invalid: %c, val: %d\n", c, (int)c);
                pos = 0;
            }
        } else {
            printf("Timeout!!!\n\r");
        }
    }
    // printf("Intron found\n\r");
}

static size_t readUART(uint8_t *buff, size_t len) {
    size_t trr = 0;
    while(trr < len) {
        int read = uart_read_bytes(UART_NUM_0, ((uint8_t*)buff) + trr, len - trr, portMAX_DELAY);
        if(read < 0) {
            printf("FAILED TO READ UART DATA\n\r");

            if(read != len) {
                printf("READ %d != %d expected\n", trr, len);
            }
        }
        trr += read;
    }
    return trr;
}

// static int send_cb(struct esp_aio *aio) {
//     free((void*)aio->pbuf);
//     free(aio);
//     return 0;
// }

// int ieee80211_output_pbuf(esp_aio_t *aio);


// static uint8_t dummy_buff[2048];

static void readPacket() {
    // printf("Reading packet\n\r");
    uint32_t size = 0;
    readUART((uint8_t*)&size, sizeof(uint32_t));
    // printf("Receiving packet size: %d\n\r", size);

    if(!out || !wifi_net_if) {
        printf("Not ready to output packets !!!\n\r");
        return;
    }

    // printf("Allocating pbuf size: %d, free heap: %d\n\r", size, esp_get_free_heap_size());
    struct pbuf *p = pbuf_alloc(PBUF_RAW_TX, size, PBUF_POOL);
    if(!p) {
        printf("Out of mem for packet size: %d!!!\n\r", size);
        return;
    }

    readUART(p->payload, p->len);
    err_t ret = out(wifi_net_if, p);
    if(ret != ERR_OK) {
        printf("Failed to send packet !!!\n\r");
    }
    pbuf_free(p);
    

    // esp_aio_t *aio = (esp_aio_t*)malloc(sizeof(esp_aio_t));
    // if(!aio) {
    //     printf("OUt of mem allocing aio !!!\n\r");
    //     return;
    // }
    // memset(aio, 0, sizeof(esp_aio_t));
    // aio->pbuf = (char*)malloc(size);
    // if(!aio->pbuf) {
    //     printf("Out of mem allocing aio data !!!\n\r");
    //     return;
    // }
    // aio->len = size;
    // aio->cb = send_cb;

    // readUART(aio->pbuf, aio->len);

    // err_t err = ieee80211_output_pbuf(aio);
    // if (err != ERR_OK) {
    //     printf("Failed to send wifi data\n\r");
    //     free((void*)aio->pbuf);
    //     free(aio);
    // }
}

static void readWifiClient() {
    wifi_config_t wifi_config;
    memset(&wifi_config, 0, sizeof(wifi_config_t));

    uint8_t ssid_len = 0;
    readUART(&ssid_len, 1);
    printf("Reading SSID len: %d\n\r", ssid_len);
    readUART(wifi_config.sta.ssid, ssid_len);

    uint8_t pass_len = 0;
    readUART(&pass_len, 1);
    printf("Reading PASS len: %d\n\r", pass_len);
    readUART(wifi_config.sta.password, pass_len);

    printf("Reconfiguring wifi\n\r");

    /* Setting a password implies station will connect to all security modes including WEP/WPA.
        * However these modes are deprecated and not advisable to be used. Incase your Access point
        * doesn't support WPA2, these mode can be enabled by commenting below line */
    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_stop());
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    sendDeviceInfo();
}

static void readMessage() {
    waitForIntron();

    uint8_t type = 0;
    size_t read = uart_read_bytes(UART_NUM_0, (uint8_t*)&type, 1, portMAX_DELAY);
    if(read != 1) {
        printf("Cannot read message type\n\r");
        return;
    }
    
    // printf("Detected message type: %d\n\r", type);
    if(type == MSG_PACKET) {
        readPacket();
    } else if (type == MSG_CLIENTCONFIG) {
        readWifiClient();
    } else {
        printf("Unknown message type: %d !!!\n\r", type);
    }
}


static void output_rx_thread(void *arg) {
    printf("RX THREAD ENTRY\n\r");

    for(;;) {
        readMessage();
    }
}

void app_main() {
    printf("APP MAIN ENTRY\n");

	esp_log_level_set("*", ESP_LOG_ERROR);
	
    ESP_ERROR_CHECK(nvs_flash_init());
	
	//debug_setup();

     // Configure parameters of an UART driver,
    // communication pins and install the driver
    uart_config_t uart_config = {
        .baud_rate = 500000,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_0, &uart_config);
    uart_driver_install(UART_NUM_0, 1024 * 2, 0, 0, NULL, 0);


    printf("UART RE-INITIALIZED\n\r");

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
		
    wifi_net_if = netif_find("en1");
	wifi_net_if->input = &wifi_input;
    out = wifi_net_if->linkoutput;
    wifi_net_if->linkoutput = &dummy_out;

    sendDeviceInfo();
	
#if LWIP_NETIF_STATUS_CALLBACK
    netif_set_status_callback(&pppos_netif, netif_status_callback);
#endif /* LWIP_NETIF_STATUS_CALLBACK */

//     sys_thread_new("pppos_rx_thread", pppos_rx_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
    printf("Creating RX thread");
	xTaskCreate(&output_rx_thread, "output_rx_thread", 2048, NULL, tskIDLE_PRIORITY, NULL);
}
