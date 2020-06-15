# How to run unit tests?

```bash
# create a build folder and run cmake within it
cd ../.. && mkdir build_tests && cd build_tests
cmake .. -G Ninja

# build all the unit tests
ninja tests

# run the unit tests
ctest .
```

> In case you don't have sufficient CMake or Ninja installed, you can use the ones downloaded by build.py/bootstrap.py:
>   ```bash
>   export PATH="$(utils/bootstrap.py --print-dependency-directory cmake)/bin:$PATH"
>   export PATH="$(utils/bootstrap.py --print-dependency-directory ninja):$PATH"
>   ```

> It is recommended to use GCC for compiling unit tests.

# How to create a new unit test?

1. Create a corresponding directory for it.
    - For example, for a unit in `src/guiapi/src/gui_timer.c` create directory `tests/unit/guiapi/gui_timer`.
2. Store your unittest cases within this directory together with their dependencies.
3. Add a CMakeLists.txt with description on how to build your tests.
    - See other unit tests for examples.
    - Don't forget to register any directory you add using `add_subdirectory` in CMakeLists.txt in the same directory.

# Windows installation

1. Download MinGw
2. Download Catch2 repository
3. Set your install paths into define_paths.sh
4. run Git Bash
5. run `source define_paths`
6. follow the beginning of this README
...
