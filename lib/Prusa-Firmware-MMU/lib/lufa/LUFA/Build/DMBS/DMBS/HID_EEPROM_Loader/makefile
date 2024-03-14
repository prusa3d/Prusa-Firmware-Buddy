#
#            DMBS Build System
#     Released into the public domain.
#
#  dean [at] fourwalledcubicle [dot] com
#        www.fourwalledcubicle.com
#

# Run "make help" for target help.

MCU          = atmega128
ARCH         = AVR8
F_CPU        = 1000000
OPTIMIZATION = s
TARGET       = HID_EEPROM_Loader
SRC          = $(TARGET).c
CC_FLAGS     =
LD_FLAGS     =
OBJECT_FILES = InputEEData.o
OBJDIR       = obj

# Default target
all:

# Determine the AVR sub-architecture of the build main application object file
FIND_AVR_SUBARCH = avr$(shell avr-objdump -f $(OBJDIR)/$(TARGET).o | grep architecture | cut -d':' -f3 | cut -d',' -f1)

# Create a linkable object file with the input binary EEPROM data stored in the FLASH section
$(OBJDIR)/InputEEData.o: $(OBJDIR)/$(TARGET).o
	@echo $(MSG_OBJCPY_CMD) Converting \"InputEEData.eep\" to a binary file \"InputEEData.bin\"
	avr-objcopy -I ihex -O binary InputEEData.eep InputEEData.bin

	@echo $(MSG_OBJCPY_CMD) Converting \"InputEEData.bin\" to a object file \"InputEEData.o\"
	avr-objcopy -I binary -O elf32-avr -B $(call FIND_AVR_SUBARCH) --rename-section .data=.progmem.data,contents,alloc,readonly,data InputEEData.bin $@

# Include LUFA build script makefiles
include ../core.mk
include ../gcc.mk
include ../hid.mk
