#DUMP

##dump.bin file structure

offset        size         content  
0x00000000    0x00020000   RAM (0x20000000..0x2001ffff)  
0x00020000    0x00010000   CCMRAM (0x10000000..0x1000ffff)  
0x00030000    0x00008000   System + OTP (0x1fff0000..0x1fff8000)  
0x00038000    0x00100000   Flash (0x08000000..0x080fffff)  
planned, not implemented
0x00138000    0x00000800   EEPROM0 (0x0000..0x0800) saved eeprom  
0x00138800    0x00000800   EEPROM1 (0x0000..0x0800) current eeprom  
0x00139000  

##registers - CCMRAM (0x1000ff00-0x1000ffff)
offset  size   content  
0x00    0x60   general registers  
0x60    0x8c   SCB  
0xf0    0x10   dumpinfo  

##general registers
0x00   r0  
0x04   r1  
0x08   r2  
0x0c   r3  
0x10   r4  
0x14   r5  
0x18   r6  
0x1c   r7  
0x20   r8  
0x24   r9  
0x28   r10  
0x2c   r11  
0x30   r12  
0x34   sp  
0x38   lr  
0x3c   pc  
0x40   xpsr  
0x44   PRIMASK  
0x48   BASEPRI  
0x4c   FAULTMASK  
0x50   CONTROL  
0x54   MSP  
0x58   PSP  
0x5c   lrexc  

##SCB
0x00   CPUID    (R/ )  CPUID Base Register  
0x04   ICSR     (R/W)  Interrupt Control and State Register  
0x08   VTOR     (R/W)  Vector Table Offset Register  
0x0C   AIRCR    (R/W)  Application Interrupt and Reset Control Register  
0x10   SCR      (R/W)  System Control Register  
0x14   CCR      (R/W)  Configuration Control Register  
0x18   SHP      (R/W)  System Handlers Priority Registers (4-7, 8-11, 12-15)  
0x24   SHCSR    (R/W)  System Handler Control and State Register  
0x28   CFSR     (R/W)  Configurable Fault Status Register  
0x2C   HFSR     (R/W)  HardFault Status Register  
0x30   DFSR     (R/W)  Debug Fault Status Register  
0x34   MMFAR    (R/W)  MemManage Fault Address Register  
0x38   BFAR     (R/W)  BusFault Address Register  
0x3C   AFSR     (R/W)  Auxiliary Fault Status Register  
0x40   PFR      (R/ )  Processor Feature Register  
0x48   DFR      (R/ )  Debug Feature Register  
0x4C   ADR      (R/ )  Auxiliary Feature Register  
0x50   MMFR     (R/ )  Memory Model Feature Register  
0x60   ISAR     (R/ )  Instruction Set Attributes Register  
0x84   RESERVED0  
0x88   CPACR    (R/W)  Coprocessor Access Control  

##dumpinfo
offset  size   content  
0x00    0x01   dump type and flags
0x01    0x0f   reserved

###dump types and flags
0x01    HardFault
0x02    IWDG warning callback
0x80    Dump not saved (bit7)
