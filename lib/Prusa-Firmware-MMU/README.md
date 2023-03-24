# Prusa-Firmware-MMU-Private

## How to prepare build env and tools
As the first step extract the AVR-GCC to some dir, e.g. `/home/user/AVRToolchainMMU/avr8-gnu-toolchain-5.4.0`

Add `/home/user/AVRToolchainMMU/avr8-gnu-toolchain-5.4.0/bin` to your `PATH`.

```
mkdir .dependencies
cd .dependencies
mkdir gcc-avr-5.4.0
cd ..
utils/bootstrap.py
```

`bootstrap.py` will now download all the "missing" dependencies into the `.dependencies` folder:
- clang-format-9.0.0-noext
- cmake-3.15.5
- ninja-1.9.0

Note: bootstrap.py will not try to download the AVR-GCC as there is already a directory called
`gcc-avr-5.4.0`. This will be fixed when we find out where to download the correct packages reliably.

## How to build the preliminary project so far:
Now the process is the same as in the Buddy Firmware:
```
utils/build.py
```

builds the firmware.hex in build/mmu_release

In case you'd like to build the project directly via cmake you can use an approach like this:
```
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE=../cmake/AnyAvrGcc.cmake
ninja
```

Should produce a firmware.hex file as well.
