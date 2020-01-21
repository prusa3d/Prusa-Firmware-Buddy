#include "utils.h"
#include "config.h"
#include "otp.h"
#include "sys.h"
#include "shared_config.h"
#include "display.h"
#include "string.h"
#include "tm_stm32f4_crc.h"
#include "qrcodegen.h"
#include "lang.h"


char* eofstr(char* str)
{
     return(str+strlen(str));
}

void appendCRC(char* str)
{
     uint32_t crc;

     TM_CRC_Init();                               // !!! spravne patri uplne jinam (zatim neni jasne kam)
     crc = TM_CRC_Calculate8((uint8_t*)(str+sizeof(ER_URL)-1), strlen(str)-sizeof(ER_URL)+1, 1);
     sprintf(eofstr(str), "/%08lX", crc);
}

void get_path_info(char* str, int error_code)
{
     strcpy(str, ER_URL);
     sprintf(eofstr(str), "%d/", error_code);
     sprintf(eofstr(str), "%d/", PRINTER_TYPE);
     sprintf(eofstr(str), "%08lX%08lX%08lX/", *(uint32_t*)(OTP_STM32_UUID_ADDR), *(uint32_t*)(OTP_STM32_UUID_ADDR+sizeof(uint32_t)), *(uint32_t*)(OTP_STM32_UUID_ADDR+2*sizeof(uint32_t)));
     sprintf(eofstr(str), "%d/", FW_VERSION);
     sprintf(eofstr(str), "%s", ((ram_data_exchange.model_specific_flags && APPENDIX_FLAG_MASK) ? "U" : "L"));
}

void create_path_info(char* str, int error_code)
{
     get_path_info(str, error_code);
     appendCRC(str);
}

bool getQR(char* str, uint8_t* pData, enum qrcodegen_Ecc qrcodegen_ecl)
{
     uint8_t temp_buff[qrcodegen_BUFFER_LEN_FOR_VERSION(qrcodegen_VERSION)];
     bool QRok;

     QRok = qrcodegen_encodeText(str, temp_buff, pData, qrcodegen_ecl, qrcodegen_VERSION, qrcodegen_VERSION, qrcodegen_Mask_AUTO, true);
     return(QRok);
}

void drawQR(uint8_t* pData)
{
     int size;

     size = qrcodegen_getSize(pData);
     for(int y = -BORDER; y < (size+BORDER); y++)
          for (int x = -BORDER; x < (size+BORDER); x++)
               display->fill_rect(rect_ui16(X0+x*MS, Y0+y*MS, MS, MS), ((qrcodegen_getModule(pData, x, y) ? COLOR_BLACK : COLOR_WHITE)));
}

bool createQR(char* str, enum qrcodegen_Ecc qrcodegen_ecl)
{
     bool QRok;
     uint8_t qrcode_buff[qrcodegen_BUFFER_LEN_FOR_VERSION(qrcodegen_VERSION)];

     QRok = getQR(str, qrcode_buff, qrcodegen_ecl);
     if( QRok )
          drawQR(qrcode_buff);
     return(QRok);
}
