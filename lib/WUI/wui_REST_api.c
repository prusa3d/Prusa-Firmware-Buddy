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
#include "json_encode.h"
#include "marlin_client.h"
#include "lwip/init.h"
#include "netdev.h"

#include <basename.h>

#include <string.h>
#include <stdio.h>

extern uint32_t start_print;
extern char *filename;

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

    JSONIFY_STR(filament_material);

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
        filament_material_escaped,
        (double)vars->temp_nozzle,
        (double)vars->target_nozzle,
        (double)vars->display_nozzle,
        (double)vars->temp_bed,
        (double)vars->target_bed,

        // No need to json-escape here, we have const inputs in here.
        printing ? "Printing" : "Operational",

        jsonify_bool(operational), jsonify_bool(paused), jsonify_bool(printing), jsonify_bool(cancelling), jsonify_bool(pausing),
        jsonify_bool(ready), jsonify_bool(busy));
}

void get_version(char *data, const uint32_t buf_len) {
    /*
     * FIXME: The netdev_get_hostname doesn't properly synchronize. That needs
     * a fix of its own. But to not make things even worse than they are, we
     * make sure to copy it out to our side first and make sure it doesn't
     * change during the JSON stringification which could lead to a different
     * length of the output and stack smashing.
     */
    const char *hostname_unsynchronized = netdev_get_hostname(netdev_get_active_id());
    const size_t hostname_in_len = strlen(hostname_unsynchronized);
    char hostname[hostname_in_len + 1];
    memcpy(hostname, hostname_unsynchronized, hostname_in_len);
    hostname[hostname_in_len] = '\0';
    JSONIFY_STR(hostname);

    snprintf(data, buf_len,
        "{"
        "\"api\":\"%s\","
        "\"server\":\"%s\","
        "\"text\":\"PrusaLink MINI\","
        "\"hostname\":\"%s\""
        "}",
        PL_VERSION_STRING, LWIP_VERSION_STRING, hostname_escaped);
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
        const char *filename = basename(vars->media_LFN);
        JSONIFY_STR(filename);
        const char *path = vars->media_LFN;
        JSONIFY_STR(path);

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

            vars->print_duration + vars->time_to_end, filename_escaped, path_escaped,
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

    JSONIFY_STR(filename);

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
        filename_escaped, (int)!start_print);
}
