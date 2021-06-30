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
#include "nvs.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

#include "netif/ppp/ppp.h"
#include "netif/ppp/pppos.h"
#include "lwip/sio.h"

#include "driver/gpio.h"

#include "netif/bridgeif.h"


/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "esptest"
#define EXAMPLE_ESP_WIFI_PASS      "lwesp8266"
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





#define GPIO_OUTPUT_IO_2    GPIO_NUM_2
#define GPIO_OUTPUT_PIN_SEL   ((1ULL<<GPIO_OUTPUT_IO_2))

void debug_c(const char c) {
	static const int DELAY_US = 1000000 / 115200;

	// Start bit
	ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_IO_2, 0));
	ets_delay_us(DELAY_US);

	for(int i = 0; i < 8; ++i) {
		ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_IO_2, ((int)c) >> i & 1));
		ets_delay_us(DELAY_US);
	}

	// Stop bit
	ESP_ERROR_CHECK(gpio_set_level(GPIO_OUTPUT_IO_2, 1));
	ets_delay_us(DELAY_US);
}

void debug_s(const char* data) {
	while(*data) {
		debug_c(*data);
		data++;
	}
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG,"connect to the AP fail");
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

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS
        },
    };

    /* Setting a password implies station will connect to all security modes including WEP/WPA.
        * However these modes are deprecated and not advisable to be used. Incase your Access point
        * doesn't support WPA2, these mode can be enabled by commenting below line */

    if (strlen((char *)wifi_config.sta.password)) {
        wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }

    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    vEventGroupDelete(s_wifi_event_group);
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

void debug_setup() {	
	gpio_config_t io_conf;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    ESP_ERROR_CHECK(gpio_config(&io_conf));
}

err_t wifi_input(struct pbuf *p, struct netif *inp) {
    //print("PBUF FLAGS");

  /*  printf("PRINTING PBUF AT: %d, len: %d\n", (int)p, p->len);
    for(uint i = 0; i < p->len; ++i) {
        printf("Char at %d: | %d | %c\n", i, ((char*)p->payload)[i], ((char*)p->payload)[i]);
    }*/

    printf("\nAT+INPUT:%d,", p->tot_len);
    do {
        //printf("%.*s", p->len, (char*)p->payload);
        fflush(0);
       /* for(uint i = 0; i < p->len; ++i) {
            write(0, &((char*)p->payload)[i], 1);
        }*/
        write(0, p->payload, p->len);

        fflush(0);
        //printf("ABOUT TO SWITCH TO NEXT\n");
        p = p->next;
    } while(p);
    //printf("ABOUT TO RETURN\n");
	return 0;
}

err_t (*out)(struct netif *netif, struct pbuf *p) = 0;
struct netif *wifi_net_if = 0;

static err_t dummy_out(struct netif *netif, struct pbuf *p) {
    printf("DUMMY OUT\n");
    return 0;
}

static void output_rx_thread(void *arg) {
    printf("RX THREAD ENTRY\n");

    static char* CMD = "AT+OUTPUT:";
    static const int CMD_LEN = 10;

    int state = 0;

    for(;;) {
        char c;
        read(0, &c, 1);

        if(state < CMD_LEN) {
            if(CMD[state] == c) {
                state++;
                if(state != CMD_LEN) {
                    //printf("STATE: %d, CHAR: %c\n", state, c);
                    continue;
                }
            } else {
                if(CMD[0] == c) {
                    state = 1;
                } else {
                    state = 0;
                }
            }
        }

        if(state == CMD_LEN) {
            char c = 0;
            uint len = 0;

            while(c != ',') {
                read(0, &c, 1);
                if(c >= '0' && c <= '9') {
                    len = len * 10 + c - '0';
                }
            }

            //printf("READING PACKET LEN: %d\n", len);

            char *buff = (char*)malloc(len);

            /*
            int rrr = read(0, buff, len);
            if(rrr != len) {
                printf("FAILED TO READ ALL DATA: %d\n", rrr);
            }*/
            for(uint i = 0 ;i < len; ++i) {
                int rrr = read(0, &((char*)buff)[i], 1);
                if(rrr != 1) {
                    printf("FAILED TOR EAD SINGLE: %d\n", rrr);
                }
            }

          /*  printf("\nREAD: ");
            for(uint i = 0; i < len; ++i) {
                printf("%d ", buff[i]);
            }
            printf("\n");*/

            if(out && wifi_net_if) {
                struct pbuf *p = pbuf_alloc(PBUF_RAW_TX, len, PBUF_RAM);
                memcpy(p->payload, buff, len);
                out(wifi_net_if, p);
                pbuf_free(p);
            }

            free(buff);


            state = 0;
        }

    }
}

void app_main() {
    printf("APP MAIN ENTRY\n");

	esp_log_level_set("*", ESP_LOG_ERROR);
	
    ESP_ERROR_CHECK(nvs_flash_init());
	
	//debug_setup();

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
		
    wifi_net_if = netif_find("en1");
	wifi_net_if->input = &wifi_input;
    out = wifi_net_if->linkoutput;
    wifi_net_if->linkoutput = &dummy_out;
	
#if LWIP_NETIF_STATUS_CALLBACK
    netif_set_status_callback(&pppos_netif, netif_status_callback);
#endif /* LWIP_NETIF_STATUS_CALLBACK */

//     sys_thread_new("pppos_rx_thread", pppos_rx_thread, NULL, DEFAULT_THREAD_STACKSIZE, DEFAULT_THREAD_PRIO);
	xTaskCreate(&output_rx_thread, "output_rx_thread", 2048, NULL, tskIDLE_PRIORITY, NULL);
}
