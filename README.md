# mininode

An HTTP framework for C++23 in the style of Express.js, built on raw POSIX
sockets and epoll with no dependencies beyond the standard library. It is a
work in progress; see the roadmap below.

## Building

You need a C++ compiler with `std::expected` (GCC 12+ or Clang 19+), Meson, and
Ninja. The Makefile wraps the usual steps and builds with clang-20:

```sh
make          # configure and build
make test     # build and run the tests
make run      # build and run the example app
```

Or run Meson directly:

```sh
meson setup build
meson compile -C build
meson test -C build
./build/app/app
```

## Layout

- `include/http/` - public headers of the `http` library
- `src/` - the library's sources, built as the static library `libhttp`
- `app/` - an example app that links the library
- `tests/` - GoogleTest unit tests

## Roadmap

- [x] Request parsing
- [ ] Router
- [ ] Response
- [ ] Route registration
- [ ] Middleware
- [ ] Server
- [ ] Polish
