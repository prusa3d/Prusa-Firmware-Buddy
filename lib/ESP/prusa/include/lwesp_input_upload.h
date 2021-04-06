#ifndef LWESP_HDR_INPUT_UPLOAD_H
#define LWESP_HDR_INPUT_UPLOAD_H

#include <string.h>
#include "lwesp/lwesp.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

lwespr_t lwesp_input_upload_process(const void *data, size_t len);

/**
 * \}
 */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* LWESP_HDR_INPUT_UPLOAD_H */
