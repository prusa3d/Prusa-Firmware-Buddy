// otp.h - OTP memory mapping
#ifndef _OTP_H
#define _OTP_H

// board revision (three bytes maj.min.sub)
#define OTP_BOARD_REVISION_ADDR 0x1FFF7800
#define OTP_BOARD_REVISION_SIZE 3

// timestamp (seconds since 1970, little-endian)
#define OTP_BOARD_TIME_STAMP_ADDR 0x1FFF7804
#define OTP_BOARD_TIME_STAMP_SIZE 4

// serial number without first four characters "CZPX" (total 15 chars, zero terminated)
#define OTP_SERIAL_NUMBER_ADDR 0x1FFF7808
#define OTP_SERIAL_NUMBER_SIZE 16

// mac address - xx:xx:xx:xx:xx:xx, six byte array
#define OTP_MAC_ADDRESS_ADDR 0x1FFF781A
#define OTP_MAC_ADDRESS_SIZE 6

// lock block - 16 bytes (byte at 0x1FFF7A00 locks 0x1FFF7800-0x1FFF781f)
#define OTP_LOCK_BLOCK_ADDR 0x1FFF7A00
#define OTP_LOCK_BLOCK_SIZE 16

// unique identifier 96bits
#define OTP_STM32_UUID_ADDR 0x1FFF7A10
#define OTP_STM32_UUID_SIZE 12

#define otp_lock_sector0 (*((uint8_t *)OTP_LOCK_BLOCK_ADDR))

#endif // _OTP_H
