# Karpuz Starter Project

Here's how this is supposed to work

```sh
mkdir build
cd build
cmake ../ -DCMAKE_VERBOSE_MAKEFILE=1
cmake --build . -- -j`nproc`
```

For MacOS users who installed flex/bison suite with brew, the following cmake
command should work:

```sh
mkdir build
cd build
cmake ../ \
    -D FL_LIBRARY=/opt/homebrew/opt/flex/lib/libfl.a \
    -D FL_INCLUDE_DIR=/opt/homebrew/opt/flex/include \
    -D BISON_EXECUTABLE=/opt/homebrew/opt/bison/bin/bison
cmake --build . -- -j`nproc`
```

Once the code compiles, you can use the ``ctest`` command to run the
test suite. Peruse ``ctest --help`` output for more info

Cheatsheet:

```sh
ctest -j`nproc`
ctest --rerun-failed
ctest --output-on-failure
ctest -V
ctest -R plus.*  # (regex)
```

HW3 tests integrate with Spidermonkey version 128 (aka mozjs-128) to run
generated WASM code inside an actual Web runtime.

You are NOT required to get it running to complete the homework. If you want to
try anyway, read README-mozjs.md for additional configuration options.
