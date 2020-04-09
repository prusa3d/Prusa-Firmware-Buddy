### Development using Visual Studio Code

![vscode screenshot](vscode_screenshot.png)

1. In Visual Studio Code, install the following extensions:
    - `CMake Tools` (handles configuring and building the project),
    - `Cortex-Debug` and `Cortex-Debug: Device Support Pack - STM32F4` (support for debugging of the firmware),
    - and `ccls` extension, which provides autocompletion and code navigation.
2. Install [ccls](https://github.com/MaskRay/ccls) on your system
    - Linux: `sudo snap install ccls --classic` ([full instructions](https://snapcraft.io/ccls))
        - Also, check out this: https://github.com/MaskRay/ccls/wiki/FAQ#maximum-number-of-file-descriptors
    - Mac: `brew install ccls`
    - <details>
        <summary>Windows</summary>
        1. Download our precompiled binaries from [here](https://prusa-buddy-firmware-dependencies.s3.eu-central-1.amazonaws.com/windows_tools.zip).
        2. Unzip them to `C:\Tools` so your file structure looks like this:

            ```
            C:\Tools
             ├── LLVM
             └── lsp-ccls
            ```

        3. Add the path to the `ccls` binary to your Visual Studio Code's settings:
            1. Ctrl+Shift+P and `Preferences: Open Settings (JSON)`
            2. Add the following to the JSON.

                ```JSON
                "ccls.launch.command": "c:\\Tools\\lsp-ccls\\bin\\ccls.exe",
                ```
        </details>

3. Install OpenOCD
    - Linux: `sudo apt install openocd`
    - Mac: `brew install openocd --HEAD`
    - <details>
        <summary>Windows</summary>
        1. Download the latest version from [here](https://gnutoolchains.com/arm-eabi/openocd).
        2. Extract the content to some permanent location.
        3. In vscode, Ctrl+Shift+P and `Preferences: Open Settings (JSON)`
        4. Add the following line with appropriate path the openocd executable.

            ```json
            "cortex-debug.openocdPath": "c:\\Path\\To\\openocd.exe"
            ```
        </details>

3. In Visual Studio Code, open the directory with this repository.

### FAQ

#### OpenOCD: Error: Can't find interface/stlink.cfg

Most likely, your OpenOCD is too old (or let's rephrase it - not new enough). The general solution is to uninstall it and build it from source yourself! Yay! 💪

1. `git clone https://github.com/ntfreak/openocd.git`
2. And follow the instructions in the readme (your are mostly interested in `OpenOCD Dependencies` and `Compiling OpenOCD`).

#### Linux/Ubuntu: libusb_open() failed with LIBUSB_ERROR_ACCESS
If you get an error like this, you are missing permissions to access the USB interface:
On Ubuntu this may be solved:

```bash
echo 'SUBSYSTEM=="usb",GROUP="users",MODE="0666"' > /etc/udev/rules.d/90-usbpermission.rules
udevadm control --reload-rules
```

Unplug and plug the STlink back and it should work.

#### Linux/Ubuntu: Error: couldn't bind tcl to socket on port 6666: Address already in use

The openocd process is already running - kill it ;) .

#### Linux/Ubuntu: arm-none-eabi-gdb not working

If you get an error like `error while loading shared libraries: libncurses.so.5: cannot open shared object file: No such file or directory`
on a newer incarnation of Linux/Ubuntu, it means your libraries (libncurses and probably also libtinfo) are newer than the arm-gdb was compiled for.
The output from `ldd` shows, that the arm-gdb didn't get all its libraries:

```bash
$ ldd gcc-arm-none-eabi-7.3.1/bin/arm-none-eabi-gdb
        linux-vdso.so.1 (0x00007fff072a8000)
        libdl.so.2 => /lib/x86_64-linux-gnu/libdl.so.2 (0x00007f900b711000)
        libncurses.so.5 => not found
        libtinfo.so.5 => not found
        libm.so.6 => /lib/x86_64-linux-gnu/libm.so.6 (0x00007f900b5c2000)
        libc.so.6 => /lib/x86_64-linux-gnu/libc.so.6 (0x00007f900b3d1000)
        /lib64/ld-linux-x86-64.so.2 (0x00007f900b751000)
```

On Ubuntu 19.10 this is exacty the case, the system contains `libncurses.so.6` and not `libncurses.so.5`.
This may be _hacked_ similarly like this:

```bash
ln -s /usr/lib/x86_64-linux-gnu/libncurses.so.6 /usr/lib/x86_64-linux-gnu/libncurses.so.5
ln -s /usr/lib/x86_64-linux-gnu/libtinfo.so.6 /usr/lib/x86_64-linux-gnu/libtinfo.so.5
```
Surprisingly, the arm-gdb is fine and works well then.
