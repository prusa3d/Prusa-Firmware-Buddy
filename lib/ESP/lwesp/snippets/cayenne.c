/*
 * MQTT client API example for Cayenne MQTT API
 *
 * Simple example for testing purposes only.
 *
 *
 */
#include "lwesp/apps/lwesp_cayenne.h"

/* Override safeprintf function */
#define safeprintf printf

/**
 * \brief           MQTT client info for server
 */
static const lwesp_mqtt_client_info_t
    mqtt_client_info
    = {
          /* Device ID */
          .id = "869f5a20-af9c-11e9-b01f-db5cf74e7fb7",

          /* User credentials */
          .user = "8a215f70-a644-11e8-ac49-e932ed599553",
          .pass = "26aa943f702e5e780f015cd048a91e8fb54cca28",

          .keep_alive = 60,
      };

/**
 * \brief           Cayenne handle
 */
static lwesp_cayenne_t cayenne;

/**
 * \brief           Cayenne event function
 * \param[in]       c: Cayenne instance
 * \param[in]       evt: Event function
 */
static lwespr_t
cayenne_evt(lwesp_cayenne_t *c, lwesp_cayenne_evt_t *evt) {
    switch (lwesp_cayenne_evt_get_type(evt)) {
    case LWESP_CAYENNE_EVT_CONNECT: {
        /* We are connected, build schema */

        /* Sensors.. */
        lwesp_cayenne_publish_data(c, LWESP_CAYENNE_TOPIC_DATA, 0, "temp", "c", "20.7");
        lwesp_cayenne_publish_data(c, LWESP_CAYENNE_TOPIC_DATA, 1, "temp", "c", "23.7");
        lwesp_cayenne_publish_data(c, LWESP_CAYENNE_TOPIC_DATA, 2, "temp", "c", "26.7");

        /* Actuators.. */
        lwesp_cayenne_publish_data(c, LWESP_CAYENNE_TOPIC_COMMAND, 10, "temp", "c", "23.7");
        lwesp_cayenne_publish_data(c, LWESP_CAYENNE_TOPIC_COMMAND, 11, "temp", "c", "26.7");

        break;
    }
    case LWESP_CAYENNE_EVT_DATA: {
        /* Reply with the same */
        safeprintf("DATA: %s\r\n", evt->evt.data.msg->values[0].value);
        lwesp_cayenne_publish_response(c, evt->evt.data.msg, LWESP_CAYENNE_RLWESP_OK, "0");
        //evt->evt.data.msg->channel
        break;
    }
    default:
        break;
    }
    return lwespOK;
}

/**
 * \brief           MQTT thread
 */
void cayenne_thread(void const *arg) {
    char s[20];
    float temp = 0.1f;

    /* Start cayenne instance */
    if (lwesp_cayenne_create(&cayenne, &mqtt_client_info, cayenne_evt) != lwespOK) {
        safeprintf("[CAYENNE] Cannot create cayenne instance..\r\n");
    }

    while (1) {
        if (lwesp_mqtt_client_api_is_connected(cayenne.api_c)) {
            sprintf(s, "%.2f", temp);
            lwesp_cayenne_publish_data(&cayenne, LWESP_CAYENNE_TOPIC_DATA, 0, "temp", "c", s);
            sprintf(s, "%.2f", temp + 1.0f);
            lwesp_cayenne_publish_data(&cayenne, LWESP_CAYENNE_TOPIC_DATA, 1, "temp", "c", s);
            sprintf(s, "%.2f", temp + 2.2f);
            lwesp_cayenne_publish_data(&cayenne, LWESP_CAYENNE_TOPIC_DATA, 2, "temp", "c", s);
            temp += 0.34f;
        }
        lwesp_delay(1000);
    }

    lwesp_sys_thread_terminate(NULL);
}
