# Puncover

https://github.com/HBehrens/puncover

# Motivation
Puncover is a neat tool for investigation of code and data structures sizes in an ELF file.
It comes especially handy when looking for a part of code to optimize for size.

# How to make it work

Set up a new python virtual environment (recommended) and activate it
```bash
python -m venv .
source bin/activate
```

Install puncover through pip
```bash
pip3 install puncover
```

Run puncover. Note that multiple parameters are necessary for optimal results, namely:
- gcc tools base name (note the syntax below)
- elf file location
- build directory
- source code location

Ideally use absolute paths.

```bash
puncover \
--gcc_tools_base STM32toolchain/gcc-arm-none-eabi-13.2.1/bin/arm-none-eabi- \
--elf       Prusa-Firmware-Buddy/build/mini_release_boot/firmware \
--build_dir Prusa-Firmware-Buddy/build/mini_release_boot \
--src_root  Prusa-Firmware-Buddy
```

Puncover runs a web server, crunches the elf file and sources and presents a website
running at http://127.0.0.1:5000

Puncover is also able to autoreload the ELF file, so it is possible to keep it running at the background,
build firmware and investigate the changes without restarting it.
It just takes a while to reload on larger code bases.
