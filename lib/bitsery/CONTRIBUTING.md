# Contributing to Bitsery

So you want to contribute something to Bitsery? That's great!
Contributions are always very much appreciated, whether it's a bug fix, a new feature or documentation.
However, to make sure the process of accepting patches goes smoothly, you should try to follow these few simple guidelines when
you contribute:

1. Fork the repository.
2. Create new branch based on the *develop* branch (`git checkout -b your_branch develop`). If your contribution is a bug fix, you should name your branch `bugfix/xxx`; for a feature, it should be `feature/xxx`. Otherwise, just use your good judgment. Consistent naming of branches is appreciated since it makes the output of `git branch` easier to understand with a single glance.
3. Do your modifications on that branch. Except for special cases, your contribution should include proper unit tests and documentation.
4. Make sure your modifications did not break anything by building, running tests:
  ```shell
  mkdir build
  cd build
  cmake -DBITSERY_BUILD_TESTS=ON ..
  make
  (cd tests; ctest)
  ```
  or run CTest scripts and view code coverage (scripts tested on ubuntu, requires lcov for coverage):
  ```shell
  cd scripts
  ctest -S build.bitsery.cmake
  ./show_coverage.sh build
  ```
5. Commit your changes, and push to your fork (`git push origin your_branch`). Commit message should be one line short description. When applicable, please squash adjacent *wip* commits into a single *logical* commit.
6. Open a pull request against Bitsery *develop* branch.


If you're working with visual studio, there is how to build and run all tests from command line
```shell
mkdir build
cd build
cmake -DBITSERY_BUILD_TESTS=ON -DGTEST_ROOT="<PATH to GTEST>" -DCMAKE_CXX_FLAGS_RELEASE=/MT ..
cmake --build . --config Release
(cd tests && ctest -C Release && cd ..)
```
/MT option might be optional, depending on how gtest was built.

## Style guide

Just use your own judgment and stick to the style of the surrounding code.
