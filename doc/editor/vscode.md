### Development using Visual Studio Code

1. In Visual Studio Code, install the following extensions:
    - `CMake Tools` (handles configuring and building the project)
    - `Cortex-Debug` and `Cortex-Debug: Device Support Pack - STM32F4` (support for debugging of the firmware)
    - For C/C++ autocompletion and code navigation
        - (1. option, recommended) On Linux or Mac
            1. Install `ccls` (`apt install ccls` on linux, `brew install ccls` on Mac)
            2. Install `ccls` extension to Visual Studio Code
        - (2. option) On Windows, Linux or Mac
            1. Install `C/C++` extension to Visual Studio Code

        > Note: Do not use both `C/C++` and `ccl
2. Install OpenOCD and add it to your PATH
    - Microsoft Windows:
    Download the latest version of openOCD from https://gnutoolchains.com/arm-eabi/openocd (7z archives).
    Extract the content into some directory.
    Navigate to Windows Control panel->System->Advanced system settings->Environment Variables.
    Add the path to OpenOCD/bin to System variables/PATH.
3. In Visual Studio Code, open the directory with this repository.
