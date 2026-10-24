sources = files(
    'main.cpp'
)

app = executable(
  'http',
  sources,
  include_directories: project_inc,
  install: true,
)

test('runs', app)
