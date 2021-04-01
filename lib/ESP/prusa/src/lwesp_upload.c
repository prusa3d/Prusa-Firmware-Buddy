#include "lwesp_upload.h"

void esp_upload_start() {
}

lwespr_t lwespi_upload_cmd(lwesp_msg_t *msg) {
    uint8_t *d = &msg->data;

    return lwespOK;
}
