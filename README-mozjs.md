# WASM integration for the Karpuz testsuite

To get WASM tests working, start by installing mozjs-128 to your development
environment.

Once done, you should reconfigure your project with the following additional
arguments:

```sh
cmake ... -D KARPUZ_TEST_WASMGEN=ON -D KARPUZ_TEST_WASMGEN_MOZJS=ON
```

For Mac users, mozjs-128 is available on macports. After installation, you should
modify your configure command to look like this:

```sh
cmake ../ \
  -D FL_LIBRARY=/opt/homebrew/opt/flex/lib/libfl.a \
  -D FL_INCLUDE_DIR=/opt/homebrew/opt/flex/include \
  -D BISON_EXECUTABLE=/opt/homebrew/opt/bison/bin/bison \
  -D KARPUZ_TEST_WASMGEN=ON \
  -D KARPUZ_TEST_WASMGEN_MOZJS=ON \
  -D MOZJS_INCLUDE_DIR=/opt/local/include \
  -D MOZJS_LIBRARY=/opt/local/lib/libmozjs-128.dylib \
  -DCMAKE_EXE_LINKER_FLAGS="-L/opt/local/lib"
```
