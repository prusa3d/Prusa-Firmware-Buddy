#ifndef SNTP_HANDLE_H
#define SNTP_HANDLE_H

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void sntp_client_init(const char *ntp_address, bool dhcp);
void sntp_client_step(bool ntp_via_dhcp, const char *ntp_ipv4_address);

#ifdef __cplusplus
}
#endif

#endif // SNTP_HANDLE_H
