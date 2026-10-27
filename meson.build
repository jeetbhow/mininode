project(
  'http',
  'cpp',
  version: '0.0.1',
  license: 'MIT',
  license_files: 'LICENSE',
  default_options: [
    'cpp_std=c++23',
    'warning_level=2',
  ],
)

# Check if the compiler supports std::expected, a C++23 feature.
cpp = meson.get_compiler('cpp')
if not cpp.has_header_symbol('expected', 'std::expected')
  error('std::expected is unavailable; use GCC 12+ or Clang 19+.')
endif

project_inc = include_directories('include')

subdir('src')
subdir('tests')
subdir('app')