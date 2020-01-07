### Development using Visual Studio Code

1. In Visual Studio Code, install the following extensions:
    - `CMake Tools` (handles configuring and building the project)
    - `ccls` (this is an LSP server providing autocompletion for the project)
    - `Cortex-Debug` and `Cortex-Debug: Device Support Pack - STM32F4` (support for debugging of the firmware)
2. Install OpenOCD and add it to your PATH
    - On Windows: Download latest version of openOCD (gnutoolchains.com/arm-eabi/openocd/), extract content,
    then through windows search open control panel "Edit the system environment variables"
    and click on button "Environment Variables...". In the "System variables" table edit "Path" variable.
    Add new path to `bin` folder at the OpenOCD extracted location.
3. In Visual Studio Code, open the directory with this repository.
