# SWD flashing — RAM state contract

When flashing Buddy firmware via SWD (e.g. with `st-flash` + ST-Link), the
default flow is **incomplete on its own** and can leave the printer in a
confused or stuck state on the next boot. This page documents the missing
clear sequence and explains why.

## TL;DR

After `st-flash write`, run **before** the MCU reset:

```tcl
# OpenOCD script (interface/stlink.cfg + target/stm32f4x.cfg)

# 1. Reset boot-firmware data exchange in shared RAM
#    (DataExchange struct in src/common/data_exchange.cpp,
#     mapped to section .boot_fw_data_exchange at 0x20000000)
mwb 0x20000000 0x00     ; fw_update_flag = FwAutoUpdate::off
mwb 0x20000001 0x01     ; appendix_status (set to "present")
mwb 0x20000002 0x00     ; fw_signature
mwb 0x20000003 0x01     ; bootloader_valid

# 2. Enable backup-SRAM clock and PWR access
mmw 0x40023830 0x00040000 0   ; RCC_AHB1ENR |= BKPSRAMEN (bit 18)
mmw 0x40007000 0x00000100 0   ; PWR_CR |= DBP (bit 8)

# 3. Clear the first 16 bytes of backup SRAM
#    (power-panic storage in src/common/power_panic_storage_bkpsram.cpp)
mww 0x40024000 0x00000000
mww 0x40024004 0x00000000
mww 0x40024008 0x00000000
mww 0x4002400C 0x00000000

# 4. NVIC system reset via AIRCR (writes the VECTKEY + SYSRESETREQ)
mww 0xE000ED0C 0x05FA0004
shutdown
```

Skip step 1 and the bootloader may interpret stale RAM as a request to
re-flash from USB, freezing at "50%" with no BBF to flash. Skip step 3
and the firmware may interpret stale backup SRAM as a recoverable
power-panic from a prior session.

## Why st-flash alone isn't enough

The xBuddy STM32F427 has three RAM regions that survive an SWD flash:

| Region            | Address       | Used by                                  |
| ----------------- | ------------- | ---------------------------------------- |
| Shared SRAM       | `0x20000000`  | DataExchange between bootloader & FW     |
| Main SRAM         | `0x20000020+` | Normal heap/stack                        |
| Backup SRAM       | `0x40024000`  | Power-panic state (`MAGIC_VALID_VALUE`)  |

`st-flash write` only touches the application area in internal flash
(`0x08020000` onwards). It does NOT clear any of the SRAM regions
above, and it does NOT issue the same kind of cold-boot reset the
printer performs via its power switch.

The result is that on the next boot, both the bootloader and the main
firmware see whatever values were in RAM when SWD started talking to
the MCU — which is usually leftover state from the firmware that was
running just before the flash. Two specific failure modes:

### 1. False firmware-update trigger (shared SRAM)

The DataExchange struct at `0x20000000` starts with:

```cpp
struct __attribute__((packed)) DataExchange {
    FwAutoUpdate fw_update_flag;   // 1 byte, enum
    uint8_t appendix_status;
    uint8_t fw_signature;
    uint8_t bootloader_valid;
    ...
};
```

`FwAutoUpdate` is an `enum class : uint8_t` with values like `on=0xAA`,
`off=0x00`, `older=0x55`, `specified=0xBB`. The bootloader reads this
byte during boot to decide whether to re-flash from `firmware.bbf` on
USB. If the leftover byte happens to match one of the "do an update"
values (~3% chance per random byte across the enum range), the
bootloader hangs trying to flash a file that isn't there — the
infamous "stuck at 50%" symptom.

Clearing the first four bytes to `00 01 00 01` puts the bootloader
back in the normal "no update requested, appendix present, signed"
state.

### 2. False power-panic recovery (backup SRAM)

`backup_sram_data_t` at `0x40024000` ends with a 32-bit `magic_valid`
field set to `0xFA0746DC` whenever there's recoverable state. The
firmware also checks CRCs over the data, so a truly random garbage
match is extraordinarily unlikely (~1/2³² × CRC odds). In practice
this is much less of a problem than the shared-RAM case — but the
power-panic path takes long enough to evaluate that even a *correct*
"no panic recorded" determination delays boot. Clearing the magic
upfront skips the check entirely.

If you only want to fix one thing, fix shared SRAM.

## Reference openocd script

A working end-to-end SWD flash including the steps above is in the
project's `flash_coreone.sh` (downstream — not in the upstream tree).
The relevant openocd invocation is:

```sh
openocd -f interface/stlink.cfg -f target/stm32f4x.cfg \
    -c "init; halt" \
    -c "mwb 0x20000000 0x00; mwb 0x20000001 0x01; mwb 0x20000002 0x00; mwb 0x20000003 0x01" \
    -c "mmw 0x40023830 0x00040000 0; mmw 0x40007000 0x00000100 0" \
    -c "mww 0x40024000 0x00000000; mww 0x40024004 0x00000000; mww 0x40024008 0x00000000; mww 0x4002400C 0x00000000" \
    -c "mww 0xE000ED0C 0x05FA0004" \
    -c "shutdown"
```

## Notes

- A hard power-cycle (turn the printer off at the wall, wait ~5 s, on
  again) accomplishes the same thing as the openocd sequence, because
  all SRAM regions lose state without standby power on the V_BAT pin.
  The openocd path exists so the SWD flow can be fully scripted
  without physical access to the power switch.
- These details apply specifically to the xBuddy STM32F427-based
  printers (Core One/+, MK4, MK3.9, MK3.5, …). Other Buddy variants
  (Mini, XL) may have different RAM layouts.
