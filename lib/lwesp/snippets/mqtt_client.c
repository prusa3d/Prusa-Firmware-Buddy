/*
 * MQTT client example with ESP device.
 *
 * Once device is connected to network,
 * it will try to connect to mosquitto test server and start the MQTT.
 *
 * If successfully connected, it will publish data to "esp8266_mqtt_topic" topic every x seconds.
 *
 * To check if data are sent, you can use mqtt-spy PC software to inspect
 * test.mosquitto.org server and subscribe to publishing topic
 */

#include "lwesp/apps/lwesp_mqtt_client.h"
#include "lwesp/lwesp.h"
#include "lwesp/lwesp_timeout.h"
#include "mqtt_client.h"

/**
 * \brief           MQTT client structure
 */
static lwesp_mqtt_client_p
mqtt_client;

/**
 * \brief           Client ID is structured from ESP station MAC address
 */
static char
mqtt_client_id[13];

/**
 * \brief           Connection information for MQTT CONNECT packet
 */
static const lwesp_mqtt_client_info_t
mqtt_client_info = {
    .id = mqtt_client_id,                       /* The only required field for connection! */

    .keep_alive = 10,
    // .user = "test_username",
    // .pass = "test_password",
};

static void mqtt_cb(lwesp_mqtt_client_p client, lwesp_mqtt_evt_t* evt);
static void example_do_connect(lwesp_mqtt_client_p client);
static uint32_t retries = 0;

/**
 * \brief           Custom callback function for ESP events
 */
static lwespr_t
mqtt_lwesp_cb(lwesp_evt_t* evt) {
    switch (lwesp_evt_get_type(evt)) {
#if LWESP_CFG_MODE_STATION
        case LWESP_EVT_WIFI_GOT_IP: {
            example_do_connect(mqtt_client);    /* Start connection after we have a connection to network client */
            break;
        }
#endif /* LWESP_CFG_MODE_STATION */
        default:
            break;
    }
    return lwespOK;
}

/**
 * \brief           MQTT client thread
 * \param[in]       arg: User argument
 */
void
mqtt_client_thread(void const* arg) {
    lwesp_mac_t mac;

    lwesp_evt_register(mqtt_lwesp_cb);              /* Register new callback for general events from ESP stack */

    /* Get station MAC to format client ID */
    if (lwesp_sta_getmac(&mac, NULL, NULL, 1) == lwespOK) {
        snprintf(mqtt_client_id, sizeof(mqtt_client_id), "%02X%02X%02X%02X%02X%02X",
                 (unsigned)mac.mac[0], (unsigned)mac.mac[1], (unsigned)mac.mac[2],
                 (unsigned)mac.mac[3], (unsigned)mac.mac[4], (unsigned)mac.mac[5]
                );
    } else {
        strcpy(mqtt_client_id, "unknown");
    }
    printf("MQTT Client ID: %s\r\n", mqtt_client_id);

    /*
     * Create a new client with 256 bytes of RAW TX data
     * and 128 bytes of RAW incoming data
     */
    mqtt_client = lwesp_mqtt_client_new(256, 128);/* Create new MQTT client */
    if (lwesp_sta_is_joined()) {                  /* If ESP is already joined to network */
        example_do_connect(mqtt_client);        /* Start connection to MQTT server */
    }

    /* Make dummy delay of thread */
    while (1) {
        lwesp_delay(1000);
    }
}

/**
 * \brief           Timeout callback for MQTT events
 * \param[in]       arg: User argument
 */
void
mqtt_timeout_cb(void* arg) {
    static uint32_t num = 10;
    lwesp_mqtt_client_p client = arg;
    lwespr_t res;

    static char tx_data[20];

    if (lwesp_mqtt_client_is_connected(client)) {
        sprintf(tx_data, "R: %u, N: %u", (unsigned)retries, (unsigned)num);
        if ((res = lwesp_mqtt_client_publish(client, "esp8266_mqtt_topic", tx_data, LWESP_U16(strlen(tx_data)), LWESP_MQTT_QOS_EXACTLY_ONCE, 0, (void*)num)) == lwespOK) {
            printf("Publishing %d...\r\n", (int)num);
            num++;
        } else {
            printf("Cannot publish...: %d\r\n", (int)res);
        }
    }
    lwesp_timeout_add(10000, mqtt_timeout_cb, client);
}

/**
 * \brief           MQTT event callback function
 * \param[in]       client: MQTT client where event occurred
 * \param[in]       evt: Event type and data
 */
static void
mqtt_cb(lwesp_mqtt_client_p client, lwesp_mqtt_evt_t* evt) {
    switch (lwesp_mqtt_client_evt_get_type(client, evt)) {
        /*
         * Connect event
         * Called if user successfully connected to MQTT server
         * or even if connection failed for some reason
         */
        case LWESP_MQTT_EVT_CONNECT: {            /* MQTT connect event occurred */
            lwesp_mqtt_conn_status_t status = lwesp_mqtt_client_evt_connect_get_status(client, evt);

            if (status == LWESP_MQTT_CONN_STATUS_ACCEPTED) {
                printf("MQTT accepted!\r\n");
                /*
                 * Once we are accepted by server,
                 * it is time to subscribe to different topics
                 * We will subscrive to "mqtt_lwesp_example_topic" topic,
                 * and will also set the same name as subscribe argument for callback later
                 */
                lwesp_mqtt_client_subscribe(client, "esp8266_mqtt_topic", LWESP_MQTT_QOS_EXACTLY_ONCE, "esp8266_mqtt_topic");

                /* Start timeout timer after 5000ms and call mqtt_timeout_cb function */
                lwesp_timeout_add(5000, mqtt_timeout_cb, client);
            } else {
                printf("MQTT server connection was not successful: %d\r\n", (int)status);

                /* Try to connect all over again */
                example_do_connect(client);
            }
            break;
        }

        /*
         * Subscribe event just happened.
         * Here it is time to check if it was successful or failed attempt
         */
        case LWESP_MQTT_EVT_SUBSCRIBE: {
            const char* arg = lwesp_mqtt_client_evt_subscribe_get_argument(client, evt);  /* Get user argument */
            lwespr_t res = lwesp_mqtt_client_evt_subscribe_get_result(client, evt); /* Get result of subscribe event */

            if (res == lwespOK) {
                printf("Successfully subscribed to %s topic\r\n", arg);
                if (!strcmp(arg, "esp8266_mqtt_topic")) {   /* Check topic name we were subscribed */
                    /* Subscribed to "esp8266_mqtt_topic" topic */

                    /*
                     * Now publish an even on example topic
                     * and set QoS to minimal value which does not guarantee message delivery to received
                     */
                    lwesp_mqtt_client_publish(client, "esp8266_mqtt_topic", "test_data", 9, LWESP_MQTT_QOS_AT_MOST_ONCE, 0, (void*)1);
                }
            }
            break;
        }

        /* Message published event occurred */
        case LWESP_MQTT_EVT_PUBLISH: {
            uint32_t val = (uint32_t)lwesp_mqtt_client_evt_publish_get_argument(client, evt); /* Get user argument, which is in fact our custom number */

            printf("Publish event, user argument on message was: %d\r\n", (int)val);
            break;
        }

        /*
         * A new message was published to us
         * and now it is time to read the data
         */
        case LWESP_MQTT_EVT_PUBLISH_RECV: {
            const char* topic = lwesp_mqtt_client_evt_publish_recv_get_topic(client, evt);
            size_t topic_len = lwesp_mqtt_client_evt_publish_recv_get_topic_len(client, evt);
            const uint8_t* payload = lwesp_mqtt_client_evt_publish_recv_get_payload(client, evt);
            size_t payload_len = lwesp_mqtt_client_evt_publish_recv_get_payload_len(client, evt);

            LWESP_UNUSED(payload);
            LWESP_UNUSED(payload_len);
            LWESP_UNUSED(topic);
            LWESP_UNUSED(topic_len);
            break;
        }

        /* Client is fully disconnected from MQTT server */
        case LWESP_MQTT_EVT_DISCONNECT: {
            printf("MQTT client disconnected!\r\n");
            example_do_connect(client);         /* Connect to server all over again */
            break;
        }

        default:
            break;
    }
}

/** Make a connection to MQTT server in non-blocking mode */
static void
example_do_connect(lwesp_mqtt_client_p client) {
    if (client == NULL) {
        return;
    }

    /*
     * Start a simple connection to open source
     * MQTT server on mosquitto.org
     */
    retries++;
    lwesp_timeout_remove(mqtt_timeout_cb);
    lwesp_mqtt_client_connect(mqtt_client, "test.mosquitto.org", 1883, mqtt_cb, &mqtt_client_info);
}
