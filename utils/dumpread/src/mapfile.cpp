// mapfile.cpp

#include "mapfile.h"
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>

// class cmapfile_mem_entry (encapsulates mapfile_mem_entry_t)
class cmapfile_mem_entry : public mapfile_mem_entry_t {
public:
    cmapfile_mem_entry(const cmapfile_mem_entry &e);
    cmapfile_mem_entry(mapfile_mem_type_t t, uint32_t a, uint32_t s, const char *n);
    ~cmapfile_mem_entry();

private:
    void init(mapfile_mem_type_t t, uint32_t a, uint32_t s, const char *n);
};

// class cmapfile (encapsulates mapfile_t)
class cmapfile : public mapfile_t {
public:
    cmapfile();
    ~cmapfile();
    bool load(const char *fn);
    cmapfile_mem_entry *find_mem_entry_by_name(const char *name);
    cmapfile_mem_entry *find_mem_entry_by_addr(uint32_t addr);

protected:
    int add_mem_entry(mapfile_mem_type_t type, uint32_t addr, uint32_t size, const char *name);
    void set_last_mem_entry_size(uint32_t size);

private:
    std::string m_filename;
    std::vector<cmapfile_mem_entry> m_mem;
    std::map<std::string, int> m_map;
};

// class cmapfile_mem_entry - implementation

cmapfile_mem_entry::cmapfile_mem_entry(const cmapfile_mem_entry &e) {
    init(e.type, e.addr, e.size, e.name);
}

cmapfile_mem_entry::cmapfile_mem_entry(mapfile_mem_type_t t, uint32_t a, uint32_t s, const char *n) {
    init(t, a, s, n);
}

cmapfile_mem_entry::~cmapfile_mem_entry() {
    if (name != 0)
        free(name);
}

void cmapfile_mem_entry::init(mapfile_mem_type_t t, uint32_t a, uint32_t s, const char *n) {
    type = t;
    addr = a;
    size = s;
    name = 0;
    if (n && (strlen(n) > 0)) {
        name = (char *)malloc(strlen(n) + 1);
        strcpy(name, n);
    }
}

// class cmapfile - implementation

cmapfile::cmapfile() {
}

cmapfile::~cmapfile() {
}

bool cmapfile::load(const char *fn) {
    char line[1024]; // entire line
                     //    char sect[16];   // section name ('data', 'bss')
    char buff[1024];
    char name[256];
    //    char file[256];
    uint32_t addr;
    uint32_t size;
    FILE *fmap = fopen(fn, "r");
    if (fmap) {
        m_filename = fn;
        // skip all lines to "Memory Configuration"
        while (fgets(line, 1024, fmap))
            if (strcmp(line, "Memory Configuration\n") == 0)
                break;
        // skip all lines to "Linker script and memory map"
        while (fgets(line, 1024, fmap))
            if (strcmp(line, "Linker script and memory map\n") == 0)
                break;
        int s = 0;
        int common_section = 0;
        uint32_t common_addr = 0;
        uint32_t common_size = 0;
        while (fgets(line, 1024, fmap)) {
            if (!common_section) {
                if ((s = sscanf(line, " *fill*         %x%x", &addr, &size)) == 2) {
                    add_mem_entry(mem_type_fill, addr, size, 0);
                } else if ((s = sscanf(line, " .text.%s%x%x%[^\n]", name, &addr, &size, buff)) >= 1) {
                    if (s == 1) {
                        if (fgets(line, 1024, fmap) && (sscanf(line, "                %x%x%[^\n]", &addr, &size, buff)) == 3)
                            add_mem_entry(mem_type_text, addr, size, name);
                    } else if (s == 4)
                        add_mem_entry(mem_type_text, addr, size, name);
                } else if ((s = sscanf(line, " .data.%s%x%x%[^\n]", name, &addr, &size, buff)) >= 1) {
                    if (s == 1) {
                        if (fgets(line, 1024, fmap) && (sscanf(line, "                %x%x%[^\n]", &addr, &size, buff)) == 3)
                            add_mem_entry(mem_type_data, addr, size, name);
                    } else if (s == 4)
                        add_mem_entry(mem_type_data, addr, size, name);
                } else if ((s = sscanf(line, " .rodata.%s%x%x%[^\n]", name, &addr, &size, buff)) >= 1) {
                    if (s == 1) {
                        if (fgets(line, 1024, fmap) && (sscanf(line, "                %x%x%[^\n]", &addr, &size, buff)) == 3)
                            add_mem_entry(mem_type_rodata, addr, size, name);
                    } else if (s == 4)
                        add_mem_entry(mem_type_rodata, addr, size, name);
                } else if ((s = sscanf(line, " .bss.%s%x%x%[^\n]", name, &addr, &size, buff)) >= 1) {
                    if (s == 1) {
                        if (fgets(line, 1024, fmap) && (sscanf(line, "                %x%x%[^\n]", &addr, &size, buff)) == 3)
                            add_mem_entry(mem_type_bss, addr, size, name);
                    } else if (s == 4)
                        add_mem_entry(mem_type_bss, addr, size, name);
                } else if (strncmp(line, " *(COMMON)", 10) == 0)
                    common_section = 1;
            } else {
                if ((s = sscanf(line, " *fill*         %x%x", &addr, &size)) == 2) {
                    if (common_size)
                        set_last_mem_entry_size(common_size);
                    common_addr = 0;
                    common_size = 0;
                    add_mem_entry(mem_type_fill, addr, size, 0);
                }
                if ((s = sscanf(line, " COMMON         %x%x%[^\n]", &addr, &size, name)) == 3) {
                    if (common_size)
                        set_last_mem_entry_size(common_size);
                    // add_mem_entry(mem_type_common, addr, size, name);
                    common_addr = addr;
                    common_size = size;
                } else if ((s = sscanf(line, "                %x                %[^\n]", &addr, name)) == 2) {
                    uint32_t prev_size = addr - common_addr;
                    if (prev_size)
                        set_last_mem_entry_size(prev_size);
                    common_addr = addr;
                    common_size -= prev_size;
                    add_mem_entry(mem_type_other, addr, 0, name);
                } else if (strlen(line) <= 1) {
                    if (common_size)
                        set_last_mem_entry_size(common_size);
                    common_section = 0;
                    common_addr = 0;
                    common_size = 0;
                }
            }
        }
        fclose(fmap);
        return true;
    }
    return false;
}

cmapfile_mem_entry *cmapfile::find_mem_entry_by_name(const char *name) {
    cmapfile_mem_entry *pentry = NULL;
    std::map<std::string, int>::iterator it;
    it = m_map.find(name);
    if (it != m_map.end()) {
        int index = it->second;
        pentry = &m_mem[index];
    }
    return pentry;
}

cmapfile_mem_entry *cmapfile::find_mem_entry_by_addr(uint32_t addr) {
    cmapfile_mem_entry *pentry = NULL;
    for (int index = 0; index < (signed)m_mem.size(); index++) {
        pentry = &m_mem[index];
        if ((pentry->addr <= addr) && ((pentry->addr + pentry->size) > addr))
            return pentry;
    }
    return NULL;
}

int cmapfile::add_mem_entry(mapfile_mem_type_t type, uint32_t addr, uint32_t size, const char *name) {
    int index = m_mem.size();
    m_mem.push_back(cmapfile_mem_entry(type, addr, size, name));
    if ((name && (strlen(name) > 0)))
        m_map[name] = index;
    //    printf("TEXT %08x %04x %s\n", addr, size, name);
    return index;
}

void cmapfile::set_last_mem_entry_size(uint32_t size) {
    m_mem.back().size = size;
    //    printf("0x%08x 0x%04x %s\n", m_mem.back().addr, m_mem.back().size, m_mem.back().name);
}

// "C" interface implementation

extern "C" {

void mapfile_free(mapfile_t *pm) {
    if (pm)
        delete ((cmapfile *)pm);
}

mapfile_t *mapfile_load(const char *fn) {
    cmapfile *mapfile = new cmapfile();
    if (mapfile->load(fn))
        return mapfile;
    delete mapfile;
    return 0;
}

mapfile_mem_entry_t *mapfile_find_mem_entry_by_name(mapfile_t *pm, const char *name) {
    cmapfile *mapfile = (cmapfile *)pm;
    cmapfile_mem_entry *entry = mapfile->find_mem_entry_by_name(name);
    return entry;
}

mapfile_mem_entry_t *mapfile_find_mem_entry_by_addr(mapfile_t *pm, uint32_t addr) {
    cmapfile *mapfile = (cmapfile *)pm;
    cmapfile_mem_entry *entry = mapfile->find_mem_entry_by_addr(addr);
    return entry;
}
}
