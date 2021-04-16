
#ifndef LWESP_HDR_CONN_UPLOAD_H
#define LWESP_HDR_CONN_UPLOAD_H

#include "lwesp/lwesp.h"
#include "lwesp_upload.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

lwespr_t lwesp_conn_upload_start(lwesp_conn_p *conn, void *const arg, lwesp_evt_fn conn_evt_fn, const uint32_t blocking);
lwespr_t lwesp_conn_upload_read_reg(lwesp_conn_p *conn, void *const arg, uint32_t addr, lwesp_evt_fn conn_evt_fn, const uint32_t blocking);
lwespr_t lwesp_conn_upload_flash(lwesp_conn_p *conn, void *const arg, uint32_t bin_size, uint32_t offset, lwesp_evt_fn conn_evt_fn, const uint32_t blocking);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_HDR_CONN_UPLOAD_H */
