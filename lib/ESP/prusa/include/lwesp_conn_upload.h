
#ifndef LWESP_HDR_CONN_UPLOAD_H
#define LWESP_HDR_CONN_UPLOAD_H

#include "lwesp/lwesp.h"
#include "lwesp_upload.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

lwespr_t lwesp_conn_upload_start(lwesp_conn_p *conn, void *const arg, lwesp_evt_fn conn_evt_fn, const uint32_t blocking);

// lwespr_t lwesp_conn_close(lwesp_conn_p conn, const uint32_t blocking);
// lwespr_t lwesp_conn_send(lwesp_conn_p conn, const void *data, size_t btw, size_t *const bw, const uint32_t blocking);
// lwespr_t lwesp_conn_set_arg(lwesp_conn_p conn, void *const arg);
//
// void *lwesp_conn_get_arg(lwesp_conn_p conn);
// uint8_t lwesp_conn_is_active(lwesp_conn_p conn);
// uint8_t lwesp_conn_is_closed(lwesp_conn_p conn);
// int8_t lwesp_conn_getnum(lwesp_conn_p conn);
//
// lwespr_t lwesp_get_conns_status(const uint32_t blocking);
// lwesp_conn_p lwesp_conn_get_from_evt(lwesp_evt_t *evt);
//
// lwespr_t lwesp_conn_write(lwesp_conn_p conn, const void *data, size_t btw, uint8_t flush, size_t *const mem_available);
// lwespr_t lwesp_conn_recved(lwesp_conn_p conn, lwesp_pbuf_p pbuf);
//
// size_t lwesp_conn_get_total_recved_count(lwesp_conn_p conn);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_HDR_CONN_UPLOAD_H */
