#include <inttypes.h>
#include <buddy/usb_device.hpp>
#include "usbh_msc.h"
#include <logging/log.hpp>
#include <buddy/littlefs_internal.h>
#include <config_store/store_instance.hpp>

LOG_COMPONENT_DEF(USBMSC, logging::Severity::debug);

uint8_t tud_msc_get_maxlun_cb(void) {
    return 1;
}

// Invoked when received SCSI READ10 command
// - Address = lba * BLOCK_SIZE + offset
//   - offset is only needed if CFG_TUD_MSC_EP_BUFSIZE is smaller than BLOCK_SIZE.
//
// - Application fill the buffer (up to bufsize) with address contents and return number of read byte. If
//   - read < bufsize : These bytes are transferred first and callback invoked again for remaining data.
//
//   - read == 0      : Indicate application is not ready yet e.g disk I/O busy.
//                      Callback invoked again with the same parameters later on.
//
//   - read < 0       : Indicate application error e.g invalid address. This request will be STALLed
//                      and return failed status in command status wrapper phase.
int32_t tud_msc_read10_cb([[maybe_unused]] uint8_t lun, uint32_t lba, uint32_t offset, void *buffer, uint32_t bufsize) {
    const struct lfs_config *lfs = littlefs_internal_config_get();
    assert(lfs != NULL);

    log_debug(USBMSC, "read: %u: lba %" PRIu32 " + offset %" PRIu32, lun, lba, offset);

    int retval = lfs->read(lfs, lba, offset, buffer, bufsize);
    return retval == 0 ? (int32_t)bufsize : -1;
}

// Invoked when received SCSI WRITE10 command
// - Address = lba * BLOCK_SIZE + offset
//   - offset is only needed if CFG_TUD_MSC_EP_BUFSIZE is smaller than BLOCK_SIZE.
//
// - Application write data from buffer to address contents (up to bufsize) and return number of written byte. If
//   - write < bufsize : callback invoked again with remaining data later on.
//
//   - write == 0      : Indicate application is not ready yet e.g disk I/O busy.
//                       Callback invoked again with the same parameters later on.
//
//   - write < 0       : Indicate application error e.g invalid address. This request will be STALLed
//                       and return failed status in command status wrapper phase.
//
int32_t tud_msc_write10_cb([[maybe_unused]] uint8_t lun, uint32_t lba, uint32_t offset, uint8_t *buffer, uint32_t bufsize) {
    const struct lfs_config *lfs = littlefs_internal_config_get();
    int retval = -1;

    if (offset == 0) {
        // we are at the beginning of the block, so lets clear it first
        if (lfs->erase(lfs, lba) != 0) {
            goto cleanup_and_return;
        }
    }

    if (lfs->prog(lfs, lba, offset, buffer, bufsize) != 0) {
        goto cleanup_and_return;
    }
    retval = bufsize;

cleanup_and_return:
    log_debug(USBMSC, "write(lba: %" PRIu32 ", offset: %" PRIu32 ", size: %" PRIu32 ") -> %i", lba, offset, bufsize, retval);

    return retval;
}

// Invoked when received SCSI_CMD_INQUIRY
// Application fill vendor id, product id and revision with string up to 8, 16, 4 characters respectively
void tud_msc_inquiry_cb([[maybe_unused]] uint8_t lun, uint8_t vendor_id[8], uint8_t product_id[16], uint8_t product_rev[4]) {
    const char vid[] = "Prusa";
    const char pid[] = "MINI";
    const char rev[] = "1.0";

    memcpy(vendor_id, vid, strlen(vid));
    memcpy(product_id, pid, strlen(pid));
    memcpy(product_rev, rev, strlen(rev));
}

/// Check whether the user has enabled the USB MSC feature
static bool usb_msc_enabled() {
    return config_store().usb_msc_enabled.get();
}

// Invoked when received Test Unit Ready command.
// return true allowing host to read/write this LUN e.g SD card inserted
bool tud_msc_test_unit_ready_cb([[maybe_unused]] uint8_t lun) {
    return usb_msc_enabled();
}

// Invoked when received SCSI_CMD_READ_CAPACITY_10 and SCSI_CMD_READ_FORMAT_CAPACITY to determine the disk size
// Application update block count and block size
void tud_msc_capacity_cb([[maybe_unused]] uint8_t lun, uint32_t *block_count, uint16_t *block_size) {
    const struct lfs_config *lfs = littlefs_internal_config_get();

    if (usb_msc_enabled() && lfs) {
        *block_count = lfs->block_count;
        *block_size = lfs->block_size;
    } else {
        // this makes sure that read/write operations will fail if usb msc isn't enabled
        *block_size = 0;
        *block_count = 0;
    }
}

/**
 * Invoked when received an SCSI command not in built-in list below.
 * - READ_CAPACITY10, READ_FORMAT_CAPACITY, INQUIRY, TEST_UNIT_READY, START_STOP_UNIT, MODE_SENSE6, REQUEST_SENSE
 * - READ10 and WRITE10 has their own callbacks
 *
 * \param[in]   lun         Logical unit number
 * \param[in]   scsi_cmd    SCSI command contents which application must examine to response accordingly
 * \param[out]  buffer      Buffer for SCSI Data Stage.
 *                            - For INPUT: application must fill this with response.
 *                            - For OUTPUT it holds the Data from host
 * \param[in]   bufsize     Buffer's length.
 *
 * \return      Actual bytes processed, can be zero for no-data command.
 * \retval      negative    Indicate error e.g unsupported command, tinyusb will \b STALL the corresponding
 *                          endpoint and return failed status in command status wrapper phase.
 */
extern bool tud_msc_set_sense(uint8_t lun, uint8_t sense_key, uint8_t add_sense_code, uint8_t add_sense_qualifier);
int32_t tud_msc_scsi_cb(uint8_t lun, uint8_t const scsi_cmd[16], void *buffer, uint16_t bufsize) {
    // read10 & write10 has their own callback and MUST not be handled here

    void const *response = NULL;
    int32_t resplen = 0;

    // most scsi handled is input
    bool in_xfer = true;

    log_debug(USBMSC, "cmd %02X", scsi_cmd[0]);

    switch (scsi_cmd[0]) {
    case 0x1e:
        // Host is about to read/write etc ... better not to disconnect disk
        resplen = 0;
        break;

    default:
        // Set Sense = Invalid Command Operation
        tud_msc_set_sense(lun, 0x05, 0x20, 0x00);

        // negative means error -> tinyusb could stall and/or response with failed status
        resplen = -1;
        break;
    }

    // return resplen must not larger than bufsize
    if (resplen > bufsize) {
        resplen = bufsize;
    }

    if (response && (resplen > 0)) {
        if (in_xfer) {
            memcpy(buffer, response, resplen);
        } else {
            // SCSI output
        }
    }

    return resplen;
}

bool tud_msc_start_stop_cb([[maybe_unused]] uint8_t lun, [[maybe_unused]] uint8_t power_condition, [[maybe_unused]] bool start, [[maybe_unused]] bool load_eject) {
    log_debug(USBMSC, "START STOP CB: %u %u %i %i", lun, power_condition, start, load_eject);
    return true;
}
