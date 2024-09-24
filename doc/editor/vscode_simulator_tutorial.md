# Setting up a Prusa MK4/Mini/XL development and simulation system in Linux Ubuntu 22.03.4 and 24.04 LTS

## Install Visual Studio Code
Important: Do NOT install VS Code via Snap or otherwise you might have issues with libraries !

1. go to https://code.visualstudio.com/docs/setup/linux and install the deb package
   ```bash
    sudo apt-get install wget gpg
    wget -qO- https://packages.microsoft.com/keys/microsoft.asc | gpg --dearmor > packages.microsoft.gpg
    sudo install -D -o root -g root -m 644 packages.microsoft.gpg /etc/apt/keyrings/packages.microsoft.gpg
    sudo sh -c 'echo "deb [arch=amd64,arm64,armhf signed-by=/etc/apt/keyrings/packages.microsoft.gpg] https://packages.microsoft.com/repos/code stable main" > /etc/apt/sources.list.d/vscode.list'
    rm -f packages.microsoft.gpg
   ```

2. ```bash
    sudo apt install apt-transport-https -y
    sudo apt update -y
    sudo apt install code -y

   ```

## Install compilation tools
1. ```bash
    sudo apt install build-essential pre-commit git libncurses5-dev libusb-1.0-0-dev pkg-config libtool freeglut3-dev libglew-dev python-is-python3 python3-venv -y
   ```
2. ```bash
    git clone https://github.com/Kitware/CMake
    cd CMake
    mkdir build && cd build
    ../bootstrap && make -j && sudo make install
    cd ..
   ```
3. ```bash
    sudo ln -s /usr/lib/x86_64-linux-gnu/libtinfo.so.6 /usr/lib/x86_64-linux-gnu/libtinfo.so.5
    sudo ln -s /usr/lib/x86_64-linux-gnu/libncurses.so.6 /usr/lib/x86_64-linux-gnu/libncurses.so.5
    sudo ln -s /usr/lib/x86_64-linux-gnu/libglut.so.3.12 /usr/lib/x86_64-linux-gnu/libglut.so.3
    ```


## Setup openocd
1. ```bash
    git clone https://github.com/ntfreak/openocd.git
   ```
2. ```bash
    sudo apt install libjaylink-dev -y
   ```
3. ```bash
    cd openocd
    ./bootstrap
   ```
4. ```bash
    ./configure --enable-stlink --enable-jlink
   ```
5. ```bash
    make -j && sudo make install
    cd ..
   ```

## Setup gdb-multiarch
1. ```bash
    sudo apt install gdb-multiarch -y
   ```
   In case gdb fails to start due to missing python 3.8 libraries, just link the gdb to gdb-multiarch:
   ```bash
    mv ./.dependencies/gcc-arm-none-eabi-13.2.1/bin/arm-none-eabi-gdb ./.dependencies/gcc-arm-none-eabi-13.2.1/bin/arm-none-eabi-gdb.bak
    sudo ln -s /usr/bin/gdb-multiarch ./.dependencies/gcc-arm-none-eabi-13.2.1/bin/arm-none-eabi-gdb
   ```

## Get the prusa xbuddy source code
1. ```bash
    git clone https://github.com/prusa3d/Prusa-Firmware-Buddy
   ```
2. ```bash
    cd Prusa-Firmware-Buddy
   ```bash
3. ```bash
    python -m venv .venv
   ```
4. ```bash
    pre-commit
   ```
5. ```bash
    python ./utils/build.py --preset=mk4 --build-type=debug
   ``` (replace mk4 here with mini or xl if needed, this will download the other requirements)


## Start and setup Visual Studio Code
1. Start visual studio code ("code" in shell)
2. Press Ctrl-Shift-X to open up the extensions menu
3. Enter in the search bar and install:
   "Cortex Debug"
   "Cortex Debug: Device Support Pack - STM32F4"
   "clangd"
   "Code Spell Checker"
   "CMake Tools"
   "C/C++"
   "C/C++ Extension Pack"
   "Python"
   "Output Colorizer"

4. Press Ctrl-Shift-P and enter "Preferences: Open User Settings (JSON)" and use the following snippet :
   ``` {
        "terminal.integrated.env.linux": {
        "GTK_PATH": ""
        },
        }
   ```

## Setting up black-magic-probe / st-link
1. ```bash
    echo 'SUBSYSTEM=="usb",GROUP="users",MODE="0666"' | sudo tee -a /etc/udev/rules.d/90-usbpermission.rules
   ```
2. ```bash
    sudo udevadm control --reload-rules
   ```
3. In VS Code, press Ctrl-Shift-P and enter "Preferences: Open User Settings (JSON)" and add the following after the commata:
    ```
    "cortex-debug.openocdPath":"/usr/local/bin/openocd"
    ```
### After the project folder has been opened, in .vscode/launch.json, add this:
```
        {
            "name": "Black Magic Probe",
            "type": "cppdbg",
            "request": "launch",
            "preLaunchTask": "CMake: build",
            "cwd": "${workspaceRoot}",
            "MIMode": "gdb",
            "targetArchitecture": "arm",
            "logging": {
                "engineLogging": true
            },
            "program": "${workspaceRoot}/build-vscode-buddy/firmware",
            "miDebuggerPath": "arm-none-eabi-gdb",
            //"miDebuggerServerAddress": "/dev/ttyACM0",
            "customLaunchSetupCommands": [
                {
                  "text": "cd ${workspaceRoot}"
                },
                {
                  "text": "target extended-remote /dev/ttyACM0" /* replace with your COM or /dev/ttyX */
                },
                {
                  "text": "monitor swdp_scan"
                },
                {
                  "text": "attach 1"
                },
                {
                  "text": "file ${workspaceRoot}/build-vscode-buddy/firmware"
                },
                {
                  "text": "load"
                },
                {
                    "text": "cd ${workspaceRoot}" /* set bath back so VScode can find source files */
                },
                {
                  "text": "set mem inaccessible-by-default off"
                },
                {
                  "text": "break main"
                }
              ],
            "serverLaunchTimeout": 10000,
            "windows": {
                "miDebuggerPath": "arm-none-eabi-gdb.exe"
            }
        }
```

## Opening the project in VS Code

1. Start VS Code
2. Click the folder icon on the left top sidemenu : Explorer
3. Open Folder -> Double click on "Prusa-Firmware-Buddy" Folder and press open (top right)
4. Select "Yes, I trust the authors"
4. In the .vscode folder, double click on launch.json and add:
   ```
   "showDevDebugOutput": "raw",
   ```

6. Click on the pyramid symbol (CMake), then over over the mouse on "Configure" project status, click on "[No Configure Preset Selected]", then on the pen symbol and select your Printer firmware, for example "mk4_debug_noboot".

![cmake_preset](/home/bjk/Prusa-Firmware-Buddy/doc/editor/vscode_tutorial/cmake_preset.png)

7. Hover over the "Build" project status and then click on the Build icon. Wait for the build to finish.

![cmake_build](/home/bjk/Prusa-Firmware-Buddy/doc/editor/vscode_tutorial/build.png)


8. Click on the Debug Symbol (sidebar, play icon) and on top, click on the drag drop window of run and debug, select "Launch simulator". Press F5.

You can now use the simulator by clicking into it and using the cursor and enter keys on the keyboard to navigate. Also PrusaLink should be accessible via http://127.0.0.1:3333 in the host browser. Enjoy !

![](/home/bjk/Prusa-Firmware-Buddy/doc/editor/vscode_tutorial/simulator.jpeg)

If the MK4 self-test for the heater fails, try to apply the patch [here](/home/bjk/Prusa-Firmware-Buddy/doc/editor/vscode_tutorial/fan_test_patch_simulator_mk4.diff)

For more details on the simulator, see [here](https://github.com/vintagepc/MINI404/wiki).
