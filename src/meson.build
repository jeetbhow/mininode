lib_sources = files(
    'version.cpp'
)

http_lib = static_library(
  'http',
  lib_sources,
  include_directories: project_inc,
)

http_dep = declare_dependency(
  link_with: http_lib,
  include_directories: project_inc,
)

app = executable(
  'http',
  files('main.cpp'),
  dependencies: http_dep,
  install: true,
)

test('runs', app)
