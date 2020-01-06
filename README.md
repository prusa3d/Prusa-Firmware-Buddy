# Buddy

This repository includes source code and firmware releases for the Original Prusa 3D printers based on the 32-bit ARM microcontrollers.

The currently supported model is:
- Original Prusa MINI

## Getting Started

### Requirements

- Python 3.6 or newer

### Cloning this repository

Run `git clone --recurse https://github.com/prusa3d/Prusa-Firmware-Buddy.git`.

In the case you already cloned the repository without the `--recurse` flag, run `git submodule update --init`.

### Building (on all platforms, without an IDE)

Run `python utils/build.py`. The binaries are then going to be stored under `./build/products`.

- Without any arguments, it will build a release version of the firmware for all supported printers and bootloader settings.
- To generate `.bbf` versions of the firmware, use: `./utils/build.py --generate-bbf`.
- Use `--build-type` to select build configurations to be built (`debug`, `release`).
- Use `--printer` to select for which printers the firmware should be built.
- By default, it will build the firmware in "prerelease mode" set to `beta`. You can change the prerelease using `--prerelease alpha`, or use `--final` to build a final version of the firmware.
- Use `--host-tools` to include host tools in the build (`bin2cc`, `png2font`, ...)
- Find more options using the `--help` flag!

#### Examples:

Build the firmware for MINI in `debug` mode:

```bash
python utils/build.py --printer mini --build-type debug
```

Build _final_ version for all printers and create signed `.bbf` versions:

```bash
python utils/build.py --final --generate-bbf --signing-key <path-to-private-key>
```

Build the firmware for MINI using a custom version of gcc-arm-none-eabi (available in `$PATH`) and use `Make` instead of `Ninja` (not recommended):

```bash
python utils/build.py --printer mini --toolchain cmake/AnyGccArmNoneEabi.cmake --generator 'Unix Makefiles'
```

### Development with LSP-based IDEs (Visual Studio [Code], Vim, Sublime Text, etc.)

1. Run `python utils/bootstrap.py` to download required dependencies.
2. Create a build directory and enter it (`mkdir build`, `cd build`).
3. Invoke CMake directly with your configuration, for example:

    ```bash
    cmake .. -G Ninja \
             -DCMAKE_TOOLCHAIN_FILE=../cmake/GccArmNoneEabi.cmake \
             -DCMAKE_EXPORT_COMPILE_COMMANDS=YES \
             -DBOOTLOADER=YES \
             -DPRINTER=MINI \
             -DCMAKE_BUILD_TYPE=Debug
    ```

    See the header of `./CMakeLists.txt` for more command-line options (most of them are one-to-one mapped with `build.py`'s options).
4. And invoke `ninja`. It will generate a `compile_commands.json` file, that an LSP server can pick up and use to provide autocompletion to your editor.

> This assumes you have sufficient version of cmake and ninja available in your PATH.

### Development using STM32CubeIDE (all platforms)

Installation:

1. Install latest STM32CubeIDE ([link](https://www.st.com/en/development-tools/stm32cubeide.html))
2. Install `cmake4eclipse` extension (`Help` > `Eclipse Marketplace...` > search for `cmake4eclipse` and install it)

Generate and open the project:

1. Generate a project file by running: `python utils/build.py --generate-cproject`
2. Open the project in the IDE (`File` > `Import Projects from File System...` > Select the root directory of this repository > `Finish`)

> 💡Changes to the generated project are not tracked by git.
> The build is still driven by CMake; therefore, if you want to add a file or change some compiler settings, change it in CMakeLists.txt directly.

## Flashing Custom Firmware

To install custom firmware, you have to break the appendix on the board. Learn how to in the following article https://help.prusa3d.com/article/zoiw36imrs-flashing-custom-firmware.

## License

The firmware source code is licensed under the GNU General Public License v3.0 and the graphics and design are licensed under Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0). Fonts are licensed under different license (see [LICENSE](LICENSE.md)).
