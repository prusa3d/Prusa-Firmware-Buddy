#!/bin/sh
MESSAGE=$1
BASE_DIR=$2
PR_DIR=$3
shift 3

# this assumes we're running from the repository root
AVR_SIZE=$(echo .dependencies/avr-gcc-*/bin/avr-size)
test -x "$AVR_SIZE" || exit 2

avr_size()
{
    "$AVR_SIZE" --mcu=atmel32u4 -C "$@"
}

avr_flash()
{
    avr_size "$@" | sed -ne 's/^Program: *\([0-9]\+\).*/\1/p'
}

avr_ram()
{
    avr_size "$@" | sed -ne 's/^Data: *\([0-9]\+\).*/\1/p'
}

cat <<EOF > "$MESSAGE"
All values in bytes. Δ Delta to base

| ΔFlash | ΔSRAM | Used Flash | Used SRAM | Free Flash | Free SRAM |
| ------ | ----- | -----------| --------- | ---------- | --------- |
EOF

atmel32u4_max_upload_size=$(grep "prusa_mm_control.upload.maximum_size=" .dependencies/prusa3dboards-*/boards.txt | cut -d "=" -f2)
atmel32u4_max_upload_data_size=$(grep "prusa_mm_control.upload.maximum_data_size=" .dependencies/prusa3dboards-*/boards.txt | cut -d "=" -f2)

base_bin=$(echo ${BASE_DIR}/firmware)
base_flash=$(avr_flash "$base_bin")
base_ram=$(avr_ram "$base_bin")

pr_bin=$(echo ${PR_DIR}/firmware)
pr_flash=$(avr_flash "$pr_bin")
pr_ram=$(avr_ram "$pr_bin")

flash_d=$(($pr_flash - $base_flash))
ram_d=$(($pr_ram - $base_ram))

flash_free=$(($atmel32u4_max_upload_size - $pr_flash))
ram_free=$(($atmel32u4_max_upload_data_size - $pr_ram))

echo "| $flash_d | $ram_d | $pr_flash | $pr_ram | $flash_free | $ram_free |" >> "$MESSAGE"
