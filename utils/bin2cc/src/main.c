//bin2cc - main.c

#include <stdio.h>

int bin2cc(char *src_filename, char *dst_filename, char *var_name);

int main(int argc, char **argv) {
    if (argc < 4)
        return 1;
    int ret = bin2cc(argv[1], argv[2], argv[3]);
    return ret;
}

int bin2cc(char *src_filename, char *dst_filename, char *var_name) {
    //	src_filename = "D:/Projects/Prusa3D/STM32/Marlin_Buddy/Gui/Res/png/icon_pepa.png";
    //	dst_filename = "D:/Projects/Prusa3D/STM32/Marlin_Buddy/Gui/Res/cc/icon_pepa.png";
    //	src_filename = "D:\\Projects\\Prusa3D\\STM32\\Marlin_Buddy\\Gui\\Res\\png\\icon_pepa.png";
    //	dst_filename = "D:\\Projects\\Prusa3D\\STM32\\Marlin_Buddy\\Gui\\Res\\cc\\png_icon_pepa.c";
    FILE *src = fopen(src_filename, "rb");
    FILE *dst = fopen(dst_filename, "w+");
    if ((!src) || (!dst))
        return 1;
    fseek(src, 0, SEEK_END);
    int file_size = ftell(src);
    int size = file_size;
    int bpl = 128; //bytes per line
    fseek(src, 0, SEEK_SET);
    //fprintf(dst, "//\n");
    //fprintf(dst, "#include <inttypes.h>\n\n");
    fprintf(dst, "const uint8_t %s[] =\n{\n", var_name);
    while (size) {
        for (int i = 0; (i < bpl) && size; i++) {
            fprintf(dst, "%c0x%02x,", i ? ' ' : '\t', fgetc(src));
            size--;
        }
        fputc('\n', dst);
    }
    fprintf(dst, "};\n");
    //fprintf(dst, "const uint16_t %s_size = %u;\n", var_name, file_size);
    fclose(dst);
    fclose(src);
    return 0;
}
