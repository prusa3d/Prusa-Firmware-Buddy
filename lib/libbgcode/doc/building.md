_**Contents**_

  * [Quick guide using presets](#quick-guide-using-presets)
  * [Building on Windows](#building-on-windows)
  
# Quick guide using presets

To build the library quickly on any platform with the least possible effort use following commands inside the checked out source directory:

```bash
cmake --preset default -DLibBGCode_BUILD_DEPS=ON
cmake --build --preset default
```

The library can also be installed with modifying the previous commands:

```bash
cmake --preset default -DLibBGCode_BUILD_DEPS=ON -DCMAKE_INSTALL_PREFIX=<install-dir>
cmake --build --preset default --target install
```

where  `<install-dir>` is an arbitrary install folder.

# Building the Python bindings

The library ships with a Python language binding which can be built in the standard way using the following command:

```bash
python -m pip install ./
```

run inside the checked out source directory.

# Building on Windows

## Step by Step Visual Studio Instructions

### Install the tools

Install Visual Studio Community 2019, or higher from [visualstudio.microsoft.com/vs/](https://visualstudio.microsoft.com/vs/).
Older versions are not supported as libbgcode requires support for C++17.
Select all workload options for C++ and make sure to launch Visual Studio after install (to ensure that the full setup completes).
Following instructions assume you are using VS 2019.

Install CMake for Windows from [cmake.org](https://cmake.org/)
Download and run the exe accepting all defaults

Install git for Windows from [gitforwindows.org](https://gitforwindows.org/)
Download and run the exe accepting all defaults

### Download sources

Clone the respository.
To place it in C:\src\libbgcode, run:
```
c:> mkdir src
c:> cd src
c:\src> git clone https://github.com/prusa3d/libbgcode.git
```

### Build dependencies

Open Visual Studio x64 Native Tools Command Prompt.
To create build folder for dependencies, run:
```
c:\src> cd libbgcode
c:\src\libbgcode> cd deps
c:\src\libbgcode\deps> mkdir build
c:\src\libbgcode\deps> cd build
```

To build `Release` configuration, run:
```
c:\src\libbgcode\deps\build> cmake .. -G "Visual Studio 16 2019" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Release
c:\src\libbgcode\deps\build> cmake --build .
```

To build `Debug` configuration, run:
```
c:\src\libbgcode\deps\build> cmake .. -G "Visual Studio 16 2019" -DBUILD_SHARED_LIBS=OFF -DCMAKE_BUILD_TYPE=Debug
c:\src\libbgcode\deps\build> cmake --build .
```

All the dependencies files will be generated into the folder:
```
c:\src\libbgcode\deps\build\destdir\usr\local
```

### Generate Visual Studio project file for libbgcode, referencing the precompiled dependencies.

Open Visual Studio x64 Native Tools Command Prompt.
To create build folder for libbgcode, run:
```
c:\src> cd libbgcode
c:\src\libbgcode> mkdir build
c:\src\libbgcode> cd build
```

To generate the VS solution for libbgcode, run:
```
c:\src\libbgcode\build> cmake -G "Visual Studio 16 2019" .. -A x64 -DCMAKE_PREFIX_PATH=C:\src\libbgcode\deps\build\destdir\usr\local -DCMAKE_INSTALL_PREFIX=C:\src\libbgcode\retail
```
Note that `CMAKE_PREFIX_PATH` must be absolute path. A relative path like "....\deps\build\destdir\usr\local" does not work.

### Compile/Install libbgcode.

Double-click c:\src\libbgcode\build\LibBGCode.sln to open in Visual Studio 2019 or open Visual Studio for C++ development (VS asks this the first time you start it).

To compile, run Build->Build Solution or press F7 for every variant (Debug/Release) you want to build.

To install, select INSTALL project in the Solution Explorer and run Build->Build INSTALL or right-click on INSTALL project in Solution Explorer and select Build.

All the library files will be installed into the folder:
```
c:\src\libbgcode\retail
```
