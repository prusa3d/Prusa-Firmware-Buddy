# Compiler specific patches

This folder will provide patches for various C++ compilers that are not C++11 compatible yet. This allows providing any fix for any compiler, without polluting core library with compiler-specific fixes.

A patch can be applied either with `git apply` or `patch` command, like this:
```bash
git apply patches/<patch_name>
patch -p1 < patches/<patch_name>
```

* [centos7_gcc4.8.2.diff](centos7_gcc4.8.2.diff) in this version, unordered_map is not fully C++11 compatible yet. It is lacking some constructors that accept allocator, and isn't using `std::allocator_traits`.
