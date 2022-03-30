#include "resources/revision.hpp"
#include <stdio.h>

bool buddy::resources::InstalledRevision::fetch(Revision &revision) {
    FILE *fp = fopen(file_path, "rb");

    if (fp == nullptr) {
        return false;
    }

    unsigned read = fread(&revision.hash[0], 1, revision.hash.size(), fp);

    fclose(fp);
    return read == revision.hash.size();
}

void buddy::resources::InstalledRevision::clear() {
    remove(file_path);
}

bool buddy::resources::InstalledRevision::set(const Revision &revision) {
    FILE *fp = fopen(file_path, "wb");

    if (fp == nullptr) {
        return false;
    }

    unsigned written = fwrite(&revision.hash[0], 1, revision.hash.size(), fp);

    fclose(fp);
    return written == revision.hash.size();
}
