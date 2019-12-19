# A3ides

Firmware for the Original Prusa 3D Printers based on 32-bit ARM microcontrollers.

## Getting Started

### Requirements

- Python 3.6 or newer

### Building (on all platforms, without an IDE)

Run `python utils/build.py`. The binaries are then going to be stored under `./build/products`.

- Without any arguments, it will build a release version of the firmware for all supported printers and bootloader settings.
- To generate `.bbf` versions of the firmware, use: `./utils/build.py --generate-bbf --signing-key <path-to-private-key>`.
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

### Development using STM32CubeIDE (all platforms)

Installation:

1. Install latest STM32CubeIDE ([link](https://www.st.com/en/development-tools/stm32cubeide.html))
2. Install `cmake4eclipse` extension (`Help` > `Eclipse Marketplace...` > search for `cmake4eclipse` and install it)

Generate and open the project:

1. Generate a project file by running: `python utils/build.py --generate-cproject`
2. Open the project in the IDE (`File` > `Import Projects from File System...` > Select the root directory of this repository > `Finish`)

> 💡Changes to the generated project are not tracked by git.
> The build is still driven by CMake; therefore, if you want to add a file or change some compiler settings, change it in CMakeLists.txt directly.

### Development with LSP-based IDEs (Visual Studio [Code], Vim, Sublime Text, etc.)

1. Install dependencies as described in section *Building*.
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

## Webserver integration
manual configurations and changes
1. Add Ethernet and LWIP from CubeMX
2. In CubeMX configurations for Ethernet
	a. parameter setting -> Set PHY address = 0
	b. Mode -> RMII
3. LWIP
	a. enable HTTPD

4. add fsdata.c (from original cubeMX library) in ..lwip/apps/httpd folder and exclude from build
   fsdata.c contains the ST example html pages
   if you want to add custom page change the #define HTTPD_USE_CUSTOM_FSDATA 1 in LWip/src/include/lwip/apps/httpd_opts.h
   and provide "fsdata_custom.c"

5. to open web files from usb flash one has to implement fs_custom.c

6. httpd options are configured in LWip/src/include/lwip/apps/httpd_opts.h

To test html file load from external usb flash and page contents at the root location and connect to the board

## Fonts

* **big** - DejaVu Sans Mono, Bold 20px
	([Free license](https://dejavu-fonts.github.io/License.html))
* **normal** - DejaVu Sans Mono 18px
	([Free license](https://dejavu-fonts.github.io/License.html))
* **small** - DejaVu Sans Mono, Bold 20px
	([Free license](https://dejavu-fonts.github.io/License.html))
* **terminal** - IBM ISO9 15px
  ([CC-BY-SA-4.0 license](https://int10h.org/oldschool-pc-fonts/fontlist/))
