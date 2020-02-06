//dumpread - main.c

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "dump.h"
#include "mapfile.h"


int main(int argc, char** argv)
{
//	argc = sizeof(args)/sizeof(char*);
//	argv = args;
	int ret = 0;
//	int argn = 0;
//	char* arg = 0;
/*	char src_filename[MAX_PATH] = "";
	char dst_filename[MAX_PATH] = "";
	char out_filename[MAX_PATH] = "";
	int char_w = 0;
	int char_h = 0;
	int char_bpp = 0;
	int charset_cols = 0;
	int charset_rows = 0;
	//parse args
	while (++argn < argc)
	{
		arg = argv[argn];
		if (sscanf(arg, "-src=%s", src_filename) == 1) continue;
		if (sscanf(arg, "-dst=%s", dst_filename) == 1) continue;
		if (sscanf(arg, "-out=%s", out_filename) == 1) continue;
		if (sscanf(arg, "-bpp=%d", &char_bpp) == 1) continue;
		if (sscanf(arg, "-w=%d", &char_w) == 1) continue;
		if (sscanf(arg, "-h=%d", &char_h) == 1) continue;
		if (sscanf(arg, "-c=%d", &charset_cols) == 1) continue;
		if (sscanf(arg, "-r=%d", &charset_rows) == 1) continue;
	}
	//check args
	if ((ret == 0) && (strlen(src_filename) == 0))
		{ fputs("SRC_FILENAME not defined!\n", stderr); ret = 1; }
	if ((ret == 0) && (strlen(dst_filename) == 0))
		{ fputs("DST_FILENAME not defined!\n", stderr); ret = 1; }
	if ((ret == 0) && (strlen(out_filename) == 0))
		{ fputs("OUT_FILENAME not defined!\n", stderr); ret = 1; }
	if ((ret == 0) && ((char_w < 1) || (char_w > 256)))
		{ fputs("WIDTH out of range!\n", stderr); ret = 1; }
	if ((ret == 0) && ((char_h < 1) || (char_w > 256)))
		{ fputs("HEIGHT out of range!\n", stderr); ret = 1; }
	if ((ret == 0) && ((charset_cols < 1) || (charset_cols > 256)))
		{ fputs("COLS out of range!\n", stderr); ret = 1; }
	if ((ret == 0) && ((charset_rows < 1) || (charset_rows > 256)))
		{ fputs("ROWS out of range!\n", stderr); ret = 1; }
	if ((ret == 0) && ((char_bpp != 1) && (char_bpp != 2) && (char_bpp != 4) && (char_bpp != 8)))
		{ fputs("BITSPERPIXEL out of range!\n", stderr); ret = 1; }
	if (ret != 0)
	{
		printf("dumpread - utility for working with dump.bin file\n");
		printf(" arguments:\n");
		printf("  -src=SRC_FILENAME  source dump.bin file\n");
		printf("  -map=MAP_FILENAME  map file\n");
		printf("  -cmd=COMMAND       bits per pixel (1,2,4,8, default 1)\n");
		getchar();
	}
	if (ret == 0)
	{
	}
*/

#if 0
	dump_t dump = {0,0,0,0};
	if (dump_load(&dump, "dump.bin"))
	{
		uint32_t* gen_regs = (uint32_t*)(dump.ccram + 0xff00);
		uint32_t* scb_regs = (uint32_t*)(dump.ccram + 0xff60);
		dump_print_hardfault_simple(gen_regs, scb_regs);
		dump_save_bin_to_file(dump.ram, DUMP_RAM_SIZE, "dump_ram.bin");
		dump_save_bin_to_file(dump.ccram, DUMP_CCRAM_SIZE, "dump_ccram.bin");
		dump_save_bin_to_file(dump.otp, DUMP_OTP_SIZE, "dump_otp.bin");
		dump_save_bin_to_file(dump.flash, DUMP_FLASH_SIZE, "dump_flash.bin");
	}
#endif

	mapfile_t* map = mapfile_load("Prusa-Firmware-Buddy.map");
	mapfile_free(map);

	printf("ready\n");
	fflush(stdout);
	getchar();
	return ret;
}

