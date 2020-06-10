#ifndef SNTP_HANDLE_H
#define SNTP_HANDLE_H

#define TIME_STR_MAX_LEN 32
void sntp_client_init(void);
void sntp_client_stop(void);
void sntp_client_cycle(void);

#endif //SNTP_HANDLE_H
