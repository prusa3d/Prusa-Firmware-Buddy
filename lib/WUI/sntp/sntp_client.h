#ifndef SNTP_HANDLE_H
#define SNTP_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

void sntp_client_static_init(const char *ntp_address);
void sntp_client_step(bool ntp_via_dhcp, const char *ntp_ipv4_address);
void sntp_client_stop(void);

#ifdef __cplusplus
}
#endif

#endif // SNTP_HANDLE_H
