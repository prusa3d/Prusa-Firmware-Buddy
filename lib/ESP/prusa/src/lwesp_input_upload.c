#include "lwesp/lwesp_private.h"
#include "lwesp/lwesp.h"
#include "lwesp_input_upload.h"
#include "lwesp/lwesp_buff.h"
#include "lwesp_upload.h"
#include "dbg.h"

static uint32_t lwesp_recv_total_len;
static uint32_t lwesp_recv_calls;

lwespr_t lwesp_input_upload_process(const void *data, size_t len) {
    lwespr_t res = lwespOK;

    if (!esp.status.f.initialized) {
        return lwespERR;
    }

    lwesp_recv_total_len += len; /* Update total number of received bytes */
    ++lwesp_recv_calls;          /* Update number of calls */

    if (len > 0) {
        lwesp_core_lock();
        res = lwespi_input_upload_process(data, len); /* Process input data */
        lwesp_core_unlock();
    }
    return res;
}
