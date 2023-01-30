/**
 * @file eeprom_dump.cpp
 */
#include "eeprom_dump.hpp"
#include "scratch_buffer.hpp"

#include "eeprom.h"
#include "eeprom_function_api.h"
#include "crc32.h"
#include "version.h"
#include "../Marlin/src/module/temperature.h"
#include "cmath_ext.h"
#include "footer_eeprom.hpp"
#include "eeprom_structure.hpp"

#include <dirent.h>
#include <cstdlib>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <array>

#define EEPROM_DEFAULT_FILE "000000000"
#define EEPROM_FILE_TYPE    ".dump"

static constexpr int max_file_count = 5;

static int name_comparer(const struct dirent **d1, const struct dirent **d2) {
    return strcmp((*d1)->d_name, (*d2)->d_name);
}

static int name_filter(const struct dirent *d) {
    if (!strcmp(d->d_name, "."))
        return 0;
    if (!strcmp(d->d_name, ".."))
        return 0;
    return 1;
}

/**
 * @brief sort files, delete
 * TODO remove dynamic allocation (scandir)
 * @param dir_name
 * @return uint32_t index of new dump
 */
static uint32_t sort_files_find_new_index_delete_old_files(const char *dir_name) {
    struct dirent **namelist;

    int cnt = scandir(dir_name, &namelist, name_filter, name_comparer);
    if (cnt <= 0)
        return 0; // new dump index
    else {
        int last_name = -1; // names are numbers
        for (int i = 0; i < cnt; ++i) {
            if (i <= (cnt - max_file_count)) {
                char full_name[128];
                if (snprintf(full_name, sizeof(full_name), "%s%s", dir_name, namelist[i]->d_name) > 0)
                    remove(full_name);
            }
            if (i == (cnt - 1)) {
                last_name = atoi(namelist[i]->d_name);
            }
            free(namelist[i]);
        }
        free(namelist);
        return last_name + 1; // new file name
    }
}

/**
 * @brief erase dumps within directory
 *
 * @param dir_name  name (path) of directory
 * @return int   count of deleted dumps, negative number on error
 */
static int erase_dumps_in_folder(const char *dir_name) {
    struct dirent **namelist;

    int cnt = scandir(dir_name, &namelist, nullptr, nullptr);
    if (cnt > 0) {
        for (int i = 0; i < cnt; ++i) {
            char full_name[128];
            if (snprintf(full_name, sizeof(full_name), "%s%s", dir_name, namelist[i]->d_name) > 0)
                remove(full_name);
            free(namelist[i]);
        }
        free(namelist);
    }
    return cnt;
}

/**
 * @brief copy eeprom to file using internal (small) buffer
 *
 * @param dump_file
 */
static void copy_eeprom_to_file(FILE *dump_file) {
    char buff[16] = {};

    // read eeprom and write it to file
    if (eeprom_read_to_buffer__first(buff, sizeof(buff))) {
        fwrite(buff, 1, sizeof(buff), dump_file);
        while (eeprom_read_to_buffer__next(buff, sizeof(buff))) {
            fwrite(buff, 1, sizeof(buff), dump_file);
        }
    }
}

/**
 * @brief modifies the name with current index
 * I could have used snprintf, but this should be much faster
 *
 * input example    "/internal/eeprom_dump/init/000000000.dump"
 * output example   "/internal/eeprom_dump/init/000000666.dump"
 *
 * @tparam SZ                   array size
 * @param init_name             default name with index to fill array with
 * @return std::array<char, SZ> current file name
 */
template <size_t SZ>
static std::array<char, SZ> modify_name(const char *init_name, uint32_t file_index) {
    std::array<char, SZ> ret;
    memcpy(&ret[0], init_name, SZ);

    int write_pos = SZ - sizeof(EEPROM_FILE_TYPE) - 1;

    // modify buffer with file name
    // atoi would corrupt buffer with '\0'
    while (file_index) {
        ret[write_pos--] = (file_index % 10) + '0';
        file_index /= 10;
    }
    return ret;
}

/*****************************************************************************/
// path definition
#define EEPROM_INIT_DUMP_PATH "/internal/eeprom_dump/init/"
#define INIT_ERR_FILE_NAME    EEPROM_INIT_DUMP_PATH EEPROM_DEFAULT_FILE EEPROM_FILE_TYPE
/*****************************************************************************/
bool eeprom::dump_init_crc_err() {
    buddy::scratch_buffer::ScratchBuffer &dump = buddy::scratch_buffer::forced_get();
    size_t sz = size_t(dump.buffer[0]) + (size_t(dump.buffer[1]) << 8);
    if (sz) {
        std::error_code ec;
        std::filesystem::create_directories(EEPROM_INIT_DUMP_PATH, ec);
        if (ec)
            return false;
        uint32_t new_file_index = sort_files_find_new_index_delete_old_files(EEPROM_INIT_DUMP_PATH);
        auto name_buff = modify_name<sizeof(INIT_ERR_FILE_NAME)>(INIT_ERR_FILE_NAME, new_file_index);

        // store buff to file
        FILE *dump_file = fopen(name_buff.begin(), "wb");
        if (dump_file) {
            fwrite(&dump.buffer[2], // 0 an 1 contain total size, but it is also stored in head, so we dont need to save it
                1, sz, dump_file);
            fclose(dump_file);
            return true;
        }
    }
    return false;
}

enum class write_err_type : char {
    wrong_id_war_known = 0,
    wrong_id_war_unknown = 1,
    variant_mismatch = 2,
    write_error = 3
};

/*****************************************************************************/
// path definition
#define EEPROM_WRITE_ERR_DUMP_PATH "/internal/eeprom_dump/write/"
#define INIT_WRITE_ERR_FILE_NAME   EEPROM_WRITE_ERR_DUMP_PATH EEPROM_DEFAULT_FILE EEPROM_FILE_TYPE
/*****************************************************************************/
FILE *open_write_dump(write_err_type type) {
    std::filesystem::create_directories(EEPROM_WRITE_ERR_DUMP_PATH);
    uint32_t new_file_index = sort_files_find_new_index_delete_old_files(EEPROM_WRITE_ERR_DUMP_PATH);
    auto name_buff = modify_name<sizeof(INIT_WRITE_ERR_FILE_NAME)>(INIT_WRITE_ERR_FILE_NAME, new_file_index);

    // store buff to file
    FILE *dump_file = fopen(name_buff.begin(), "wb");

    if (dump_file) {
        char c = char(type);
        fwrite(&c, 1, sizeof(char), dump_file);
    }
    return dump_file;
}

bool eeprom::dump_wrong_id__var_known(int enum_eevar_id, variant8_t var) {
    FILE *dump_file = open_write_dump(write_err_type::wrong_id_war_known);
    if (dump_file) {
        copy_eeprom_to_file(dump_file);

        //store variant too
        char buff[sizeof(var)];
        memcpy(buff, &var, sizeof(buff));
        fwrite(buff, 1, sizeof(buff), dump_file);

        fclose(dump_file);
        return true;
    }
    return false;
}

bool eeprom::dump_wrong_id__var_unknown(int enum_eevar_id) {
    FILE *dump_file = open_write_dump(write_err_type::wrong_id_war_unknown);
    if (dump_file) {
        copy_eeprom_to_file(dump_file);

        fclose(dump_file);
        return true;
    }
    return false;
}

// very similar to dump_wrong_id__var_known
// but it is just coincidence
// so there is no point to make shared code
bool eeprom::dump_variant_mismatch(int enum_eevar_id, variant8_t var) {
    FILE *dump_file = open_write_dump(write_err_type::variant_mismatch);
    if (dump_file) {
        copy_eeprom_to_file(dump_file);

        //store variant too
        char buff[sizeof(var)];
        memcpy(buff, &var, sizeof(buff));
        fwrite(buff, 1, sizeof(buff), dump_file);

        fclose(dump_file);
        return true;
    }
    return false;
}

bool eeprom::dump_write_err(const eeprom_vars_t &ram_data, int iteration, int var_id, const char *iteration_buff, size_t iteration_buff_sz) {
    FILE *dump_file = open_write_dump(write_err_type::write_error);
    if (dump_file) {
        copy_eeprom_to_file(dump_file);

        char int_buff[sizeof(int)];

        //store iteration
        memcpy(int_buff, &iteration, sizeof(int_buff));
        fwrite(int_buff, 1, sizeof(int_buff), dump_file);

        //store var_id
        memcpy(int_buff, &var_id, sizeof(int_buff));
        fwrite(int_buff, 1, sizeof(int_buff), dump_file);

        //store iteration_buff
        fwrite(iteration_buff, 1, iteration_buff_sz, dump_file);

        fclose(dump_file);
        return true;
    }
    return false;
}

size_t eeprom::erase_all_dumps() {
    int write_err = erase_dumps_in_folder(EEPROM_WRITE_ERR_DUMP_PATH);
    int init_err = erase_dumps_in_folder(EEPROM_INIT_DUMP_PATH);
    return std::max(0, init_err) + std::max(0, write_err);
}
