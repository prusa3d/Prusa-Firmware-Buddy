# How to run unit tests?

```bash
# create a build folder and run cmake within it
cd ../.. && mkdir build_tests && cd build_tests
cmake .. -G Ninja -DBOARD=BUDDY

# build all the unit tests
ninja tests

# run the unit tests
ctest .

> In case you don't have sufficient CMake or Ninja installed, you can use the ones downloaded by build.py/bootstrap.py:
>   ```bash
>   export PATH="$(python ../utils/bootstrap.py --print-dependency-directory cmake)/bin:$PATH"
>   export PATH="$(python ../utils/bootstrap.py --print-dependency-directory ninja):$PATH"
>   ```

> It is recommended to use GCC for compiling unit tests.

# How to create a new unit test?

1. Create a corresponding directory for it.
    - For example, for a unit in `src/guiapi/src/gui_timer.c` create directory `tests/unit/guiapi/gui_timer`.
2. Store your unittest cases within this directory together with their dependencies.
    Don't use the same file name for testing file and source file. Use '.cpp' extension.
3. Add a CMakeLists.txt with description on how to build your tests.
    - See other unit tests for examples.
    - Don't forget to register any directory you add using `add_subdirectory` in CMakeLists.txt in the same directory.

# Tests on Windows

1. Download & install MinGW and make sure .../MinGW/bin/ is in your path.
2. Check if Python is installed.
3. Download & install some bash (GIT bash could be already installed).
4. Run bash and get to your repository directory (cd ...).
5. Run these to prepare for test:

```bash
mkdir -p build_tests \
&& cd build_tests \
&& rm -rf * \
&& export PATH="$(python ../utils/bootstrap.py --print-dependency-directory cmake)/bin:$PATH" \
&& export PATH="$(python ../utils/bootstrap.py --print-dependency-directory ninja):$PATH" \
&& export CTEST_OUTPUT_ON_FAILURE=1 \
&& cmake .. -G Ninja
```

6. Run the tests:

```bash
ninja tests && ctest
```
