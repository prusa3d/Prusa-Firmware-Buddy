// mapfile.cpp

#include "mapfile.h"
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>
#include <map>


// class cmapfile_mem_entry (encapsulates mapfile_mem_entry_t)
class cmapfile_mem_entry : public mapfile_mem_entry_t
{
public:
	cmapfile_mem_entry(uint32_t a, uint32_t s, void* p);
private:
};


class cmapfile_entry : public mapfile_entry_t
{
public:
	cmapfile_entry();
	~cmapfile_entry();
private:
	std::string m_name;
	uint32_t m_addr;
	uint32_t m_size;
};

cmapfile_entry::cmapfile_entry()
{

}

cmapfile_entry::~cmapfile_entry()
{

}


class cmapfile : public mapfile_t
{
public:
	cmapfile();
	~cmapfile();
	bool load(const char* fn);
protected:
	int add_fill(uint32_t addr, uint32_t size);
	int add_text(uint32_t addr, uint32_t size, const char* name);
	int add_data(uint32_t addr, uint32_t size, const char* name);
	int add_bss(uint32_t addr, uint32_t size, const char* name);
private:
	std::string m_filename;
//	std::map<std::string, uint32_t> m_map;
	std::vector<cmapfile_mem_entry> m_mem;
};


cmapfile::cmapfile()
{

}

cmapfile::~cmapfile()
{

}

int cmapfile::add_fill(uint32_t addr, uint32_t size)
{
	m_mem.push_back(cmapfile_mem_entry(addr, size, 0));
	return m_mem.size() - 1;
}

int cmapfile::add_text(uint32_t addr, uint32_t size, const char* name)
{
	m_mem.push_back(cmapfile_mem_entry(addr, size, 0));
	printf("TEXT %08x %04x %s\n", addr, size, name);
	return m_mem.size() - 1;
}

int cmapfile::add_data(uint32_t addr, uint32_t size, const char* name)
{
	m_mem.push_back(cmapfile_mem_entry(addr, size, 0));
	printf("DATA %08x %04x %s\n", addr, size, name);
	return m_mem.size() - 1;
}

int cmapfile::add_bss(uint32_t addr, uint32_t size, const char* name)
{
	m_mem.push_back(cmapfile_mem_entry(addr, size, 0));
	printf("BSS %08x %04x %s\n", addr, size, name);
	return m_mem.size() - 1;
}

bool cmapfile::load(const char* fn)
{
	char line[1024]; // entire line
//	char sect[16];   // section name ('data', 'bss')
	char buff[1024];
	char name[256];
//	char file[256];
	uint32_t addr;
	uint32_t size;
	FILE* fmap = fopen(fn, "r");
	if (fmap)
	{
		m_filename = fn;
/*
		m_map["test0"] = 0;
		m_map["test1"] = 1;
		m_map["test2"] = 2;

		std::string tst;
		uint32_t u;
		u = m_map["test0"];
		u = m_map["test1"];
		u = m_map["test2"];
		m_mem.push_back(cmapfile_mem_entry(0, 0, 0));
		m_mem.push_back(cmapfile_mem_entry(1, 1, 0));
*/
		// skip all lines to "Memory Configuration"
		while (fgets(line, 1024, fmap))
			if (strcmp(line, "Memory Configuration\n") == 0)
				break;
		// skip all lines to "Linker script and memory map"
		while (fgets(line, 1024, fmap))
			if (strcmp(line, "Linker script and memory map\n") == 0)
				break;
		int n = 0;
		int c = 0;
		int s = 0;
		while (fgets(line, 1024, fmap))
		{
			if ((s = sscanf(line, " *fill*         %x%x", &addr, &size)) == 2)
				add_fill(addr, size);
			else if ((s = sscanf(line, " .text.%s%x%x%[^\n]", name, &addr, &size, buff)) >= 1)
			{
				if (s == 1)
				{
					if (fgets(line, 1024, fmap) &&
						(sscanf(line, "                %x%x%[^\n]", &addr, &size, buff)) == 3)
						add_text(addr, size, name);
				}
				else if (s == 4)
					add_text(addr, size, name);
			}
			else if ((s = sscanf(line, " .data.%s%x%x%[^\n]", name, &addr, &size, buff)) >= 1)
			{
				if (s == 1)
				{
					if (fgets(line, 1024, fmap) &&
						(sscanf(line, "                %x%x%[^\n]", &addr, &size, buff)) == 3)
						add_data(addr, size, name);
				}
				else if (s == 4)
					add_data(addr, size, name);
			}
			else if ((s = sscanf(line, " .bss.%s%x%x%[^\n]", name, &addr, &size, buff)) >= 1)
			{
				if (s == 1)
				{
					if (fgets(line, 1024, fmap) &&
						(sscanf(line, "                %x%x%[^\n]", &addr, &size, buff)) == 3)
						add_bss(addr, size, name);
				}
				else if (s == 4)
					add_bss(addr, size, name);
			}

		}
		printf("n=%u\n", n);
		printf("c=%u\n", c);
		printf("mem.size=%u\n", m_mem.size());
		fclose(fmap);
	}
	return false;
}


extern "C"
{

void mapfile_free(mapfile_t* pm)
{
	if (pm) delete ((cmapfile*)pm);
}

mapfile_t* mapfile_load(const char* fn)
{
	cmapfile* mapfile = new cmapfile();
	if (mapfile->load(fn))
		return mapfile;
	delete mapfile;
	return 0;
}

}


// class cmapfile_mem_entry - implementation

cmapfile_mem_entry::cmapfile_mem_entry(uint32_t a, uint32_t s, void* p)
{
	addr = a;
	size = s;
	ptr = p;
}


