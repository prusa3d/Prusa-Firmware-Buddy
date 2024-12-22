# Prusa-Firmware-MMU
This repository includes source code and firmware releases for the Original Prusa Multi Material Unit based on 8-bit ATMEL microcontroller.

The currently supported models are:
- Original Prusa MMU3
- Original Prusa MMU2S

## Introduction
This is the new firmware for the Multi Material Unit (MMU).

### Motivation
The key motivation for developing a new firmware structure were as follows:

- adding a possibility of reporting the MMU's state even during running commands - the architecture of the original MM-control-01 project didn't allow to achieve this requirement
- while being able to report the internal state of the MMU, the printer should be able to describe the error states clearly on its LCD without leaving the user to rely on some blinking LEDs
- modular design prepared for possible future upgrades

### Firmware architecture
The whole firmware is composed of simple state machines which run all at once - it is a kind of simple cooperative multi-tasking while not eating up any significant resources by deploying generic task switching solutions like RTOS or similar. The general rule is to avoid waiting inside these state machines, no state machine is allowed to block execution of others. That implies making separate waiting states which only check for some condition to be true before proceeding further.

The firmware is separated into 4 layers:

- HAL is responsible for talking to the physical hardware, in our case an AVR processor and its peripherals, TMC2130 stepper drivers, shift registers etc.
- modules are the components abstracted of the real hardware and/or connection. A typical example are the buttons, LEDs, Idler, Selector etc.
- logic layer is the application logic - this layer provides the sequences and logical relations between modules thus forming the behavior of the MMU.
- main is the top layer, it is responsible for initialization of the whole firmware and performing the main loop, where the stepping of all the automata is located.

## Getting Started

### Requirements

- Python 3.6 or newer (with pip)

### Cloning this repository

Run `git clone https://github.com/prusa3d/Prusa-Firmware-MMU.git`.

### How to prepare build env and tools
Run `./utils/bootstrap.py`

`bootstrap.py` will now download all the "missing" dependencies into the `.dependencies` folder:
- clang-format-18.1.8
- cmake-3.30.3
- ninja-1.12.1
- avr-gcc-7.3.0

### How to build the preliminary project so far:
Now the process is the same as in the Buddy Firmware:
```
./utils/build.py
```

builds the `MMU3_<major>.<minor>.<revision>+<commit nr>`` in build/release folder

In case you'd like to build the project directly via cmake you can use an approach like this:
```
mkdir build
cd build
cmake .. -G Ninja -DCMAKE_TOOLCHAIN_FILE=../cmake/AvrGcc.cmake
ninja
```

It will produce a `MMU3_<version>.hex` file.

### Development

The build process of this project is driven by CMake and `build.py` is just a high-level wrapper around it. As most modern IDEs support some kind of CMake integration, it should be possible to use almost any editor for development. Below are some documents describing how to setup some popular text editors.

- Visual Studio Code
- Vim
- Other LSP-based IDEs (Atom, Sublime Text, ...)

#### Formatting

All the source code in this repository is automatically formatted:

- C/C++ files using [clang-format](https://clang.llvm.org/docs/ClangFormat.html),
- Python files using [yapf](https://github.com/google/yapf),
- and CMake files using [cmake-format](https://github.com/cheshirekow/cmake_format).

If you want to contribute, make sure to install [pre-commit](https://pre-commit.com) and then run `pre-commit install` within the repository. This makes sure that all your future commits will be formatted appropriately. Our build server automatically rejects improperly formatted pull requests.

## License

The firmware source code is licensed under the GNU General Public License v3.0 and the graphics and design are licensed under Attribution-NonCommercial-ShareAlike 4.0 International (CC BY-NC-SA 4.0). Fonts are licensed under different license (see [LICENSE](LICENSE.md)).
