/*
 * wui_REST_api.c
 *
 *  Created on: Jan 24, 2020
 *      Author: joshy <joshymjose[at]gmail.com>
 *  Modify on 09/17/2021
 *      Author: Marek Mosna <marek.mosna[at]prusa3d.cz>
 */

#include "wui_REST_api.h"
#include "wui_api.h"
#include "filament.h" //get_selected_filament_name
#include "marlin_client.h"
#include "lwip/init.h"
#include "netdev.h"

#include <basename.h>

#include <string.h>
#include <stdio.h>

extern uint32_t start_print;
extern char *filename;

// FIXME: We really need a proper JSON library. This is broken on many different levels.

const char *json_bool(bool value) {
    static const char json_true[] = "true";
    static const char json_false[] = "false";
    if (value) {
        return json_true;
    } else {
        return json_false;
    }
}

void get_printer(char *data, const uint32_t buf_len) {
    marlin_vars_t *vars = marlin_vars();
    const char *filament_material = get_selected_filament_name();

    bool operational = true;
    bool paused = false;
    bool printing = false;
    bool cancelling = false;
    bool pausing = false;
    bool ready = true;
    bool busy = false;

    marlin_client_loop();

    switch (vars->print_state) {
    case mpsPrinting:
        printing = busy = true;
        ready = operational = false;
        break;
    case mpsPausing_Begin:
    case mpsPausing_WaitIdle:
    case mpsPausing_ParkHead:
        pausing = paused = busy = true;
        ready = operational = false;
        break;
    case mpsPaused:
        paused = true;
        break;
    case mpsResuming_Begin:
    case mpsResuming_Reheating:
    case mpsResuming_UnparkHead:
        ready = operational = false;
        busy = printing = true;
        break;
    case mpsAborting_Begin:
    case mpsAborting_WaitIdle:
    case mpsAborting_ParkHead:
        cancelling = busy = true;
        ready = operational = false;
        break;
    case mpsFinishing_WaitIdle:
    case mpsFinishing_ParkHead:
        busy = true;
        ready = operational = false;
        break;
    case mpsAborted:
    case mpsFinished:
    case mpsIdle:
    default:
        break;
    }

    snprintf(data, buf_len,
        "{"
        "\"telemetry\":{"
        "\"temp-bed\":%.1f,"
        "\"temp-nozzle\":%.1f,"
        "\"print-speed\":%d,"
        "\"z-height\":%.1f,"
        "\"material\":\"%s\""
        "},"
        "\"temperature\":{"
        "\"tool0\":{"
        "\"actual\":%.1f,"
        "\"target\":%.1f,"
        // Note: our own extension, because our printers sometimes display
        // different "target" temperature than what they heat towards.
        "\"display\":%.1f,"
        "\"offset\":0"
        "},"
        "\"bed\":{"
        "\"actual\":%.1f,"
        "\"target\":%.1f,"
        "\"offset\":0"
        "}"
        "},"
        "\"state\":{"
        "\"text\":\"%s\","
        "\"flags\":{"
        "\"operational\":%s,"
        "\"paused\":%s,"
        "\"printing\":%s,"
        "\"cancelling\":%s,"
        "\"pausing\":%s,"
        // We don't have an SD card.
        "\"sdReady\":false,"
        "\"error\":false,"
        "\"ready\":%s,"
        "\"closedOrError\":false,"
        "\"busy\":%s"
        "}"
        "}"
        "}",
        (double)vars->temp_bed,
        (double)vars->temp_nozzle,
        (int)vars->print_speed,
        (double)vars->pos[2], // XYZE, mm
        filament_material,
        (double)vars->temp_nozzle,
        (double)vars->target_nozzle,
        (double)vars->display_nozzle,
        (double)vars->temp_bed,
        (double)vars->target_bed,

        printing ? "Printing" : "Operational",

        json_bool(operational), json_bool(paused), json_bool(printing), json_bool(cancelling), json_bool(pausing),
        json_bool(ready), json_bool(busy));
}

void get_version(char *data, const uint32_t buf_len) {

    snprintf(data, buf_len,
        "{"
        "\"api\":\"%s\","
        "\"server\":\"%s\","
        "\"text\":\"PrusaLink MINI\","
        "\"hostname\":\"%s\""
        "}",
        PL_VERSION_STRING, LWIP_VERSION_STRING, netdev_get_hostname(netdev_get_active_id()));
}

void get_job(char *data, const uint32_t buf_len) {
    marlin_vars_t *vars = marlin_vars();

    marlin_client_loop();

    bool has_job = false;
    const char *state = "Unknown";

    switch (vars->print_state) {
    case mpsFinishing_WaitIdle:
    case mpsFinishing_ParkHead:
    case mpsPrinting:
        has_job = true;
        state = "Printing";
        break;
    case mpsPausing_Begin:
    case mpsPausing_WaitIdle:
    case mpsPausing_ParkHead:
        has_job = true;
        state = "Pausing";
        break;
    case mpsPaused:
        has_job = true;
        state = "Paused";
        break;
    case mpsResuming_Begin:
    case mpsResuming_Reheating:
    case mpsResuming_UnparkHead:
        has_job = true;
        state = "Resuming";
        break;
    case mpsAborting_Begin:
    case mpsAborting_WaitIdle:
    case mpsAborting_ParkHead:
        has_job = true;
        state = "Cancelling";
        break;
    case mpsAborted:
    case mpsFinished:
    case mpsIdle:
        state = "Operational";
        break;
    }

    if (has_job) {
        snprintf(data, buf_len,
            "{"
            "\"state\":\"%s\","
            "\"job\":{"
            "\"estimatedPrintTime\":%" PRIu32 ","
            "\"file\":{\"name\":\"%s\",\"path\":\"%s\"}"
            "}," // } job
            "\"progress\":{"
            "\"completion\":%f,"
            "\"printTime\":%" PRIu32 ","
            "\"printTimeLeft\":%" PRIu32 ""
            "}" // } progress
            "}",
            state,

            vars->print_duration + vars->time_to_end, basename(vars->media_LFN), vars->media_LFN,
            ((double)vars->sd_percent_done / 100.0), // We might want to have better resolution that whole percents.
            vars->print_duration, vars->time_to_end);
    } else {
        /*
         * If we do not have any job, we don't really have much meaningful info
         * to provide. Unfortunately, the octoprint API docs don't mention the
         * situation.
         *
         * Examining their source code it seems they are returning nulls in
         * such case, though if this is by choice or accidental is unclear.
         * Let's do the same.
         */
        snprintf(data, buf_len,
            "{"
            "\"state\": \"%s\","
            "\"job\": null,"
            "\"progress\": null"
            "}",
            state);
    }
}

void get_files(char *data, const uint32_t buf_len) {

    /*
     * Warning: This is a hack/stub.
     *
     * * This is being used both to answer /api/files _and_ as the content of the post GCODE.
     * * We don't have a way to stream the body (eg. generate on the fly as we
     *   are iterating through the files on the flash drive).
     * * This is missing a lot of fields in the files.
     *
     * This probably depends on first getting an actual HTTP server.
     */

    snprintf(data, buf_len,
        "{"
        "\"files\": [{"
        "\"local\": {"
        "\"name\": \"%s\","
        "\"origin\": \"local\""
        "}"
        "}],"
        "\"done\": %d"
        "}",
        filename, (int)!start_print);
}
