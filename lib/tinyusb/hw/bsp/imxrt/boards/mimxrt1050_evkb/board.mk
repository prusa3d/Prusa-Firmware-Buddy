CFLAGS += -DCPU_MIMXRT1052DVL6B
MCU_VARIANT = MIMXRT1052

# For flash-pyocd target
PYOCD_TARGET = mimxrt1050

# flash using pyocd
flash: flash-pyocd
