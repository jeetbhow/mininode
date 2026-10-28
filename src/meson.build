lib_sources = files(
    'headers.cpp',
    'http.cpp',
    'utils.cpp',
    'router.cpp'
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