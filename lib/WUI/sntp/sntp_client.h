#ifndef SNTP_HANDLE_H
#define SNTP_HANDLE_H

#ifdef __cplusplus
extern "C" {
#endif

/*!
 * \brief One pass of the SNTP client's reconciliation
 *
 * Called periodically from the network loop. Compares what should be true --
 * some interface up, and the NTP server the config store currently wants --
 * with what the lwIP client is doing, and stops or restarts it to match.
 *
 * Config changes are picked up by the comparison itself, so a settings reload
 * needs no notification and no observable interface down/up transition.
 */
void sntp_client_step(void);

#ifdef __cplusplus
}
#endif

#endif // SNTP_HANDLE_H
