#ifndef __MQTT_CLIENT_API_H
#define __MQTT_CLIENT_API_H

#ifdef __cplusplus
extern "C" {
#endif

void mqtt_client_api_thread(void const* arg);
void mqtt_client_api_cayenne_thread(void const* arg);

#ifdef __cplusplus
}
#endif

#endif
