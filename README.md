# http

A C++20 executable built with Meson.

Install a C++ compiler, Meson, and Ninja, then run these commands from this directory:

```sh
meson setup build
meson compile -C build
meson test -C build
./build/src/http
```

On Windows, run `build\src\http.exe`.

Put source files in `src/` and headers in `include/`. Add new source files to
`src/meson.build`. The `runs` smoke test verifies that the executable exits successfully;
add behavior tests as the project grows.
