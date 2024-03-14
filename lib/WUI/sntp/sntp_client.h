#ifndef SNTP_HANDLE_H
#define SNTP_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

void sntp_client_init(void);
void sntp_client_step(void);

#ifdef __cplusplus
}
#endif

#endif // SNTP_HANDLE_H
