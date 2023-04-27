### Development with LSP-based IDEs (Visual Studio [Code], Vim, Sublime Text, etc.)

1. Run `python utils/bootstrap.py` to download required dependencies.
2. Create a build directory and enter it (`mkdir build`, `cd build`).
3. Invoke CMake directly with your configuration, for example:

    ```bash
    cmake .. --preset mini \
             -G Ninja \
             -DCMAKE_EXPORT_COMPILE_COMMANDS=YES \
             -DBOOTLOADER=YES \
             -DCMAKE_BUILD_TYPE=Debug
    ```

    See the header of `./CMakeLists.txt` for more command-line options (most of them are one-to-one mapped with `build.py`'s options).
4. And invoke `ninja`. It will generate a `compile_commands.json` file, that an LSP server can pick up and use to provide autocompletion to your editor (we recommend using `clangd`).
5. Install some spell checker (optional but recommended).

> This assumes you have sufficient version of cmake and ninja available in your PATH.
