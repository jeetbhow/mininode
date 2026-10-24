gtest_dep = dependency('gtest', main: true, fallback: ['gtest', 'gtest_main_dep'])

test_sources = files(
    'smoke_test.cpp'
)

unit_tests = executable(
  'unit_tests',
  test_sources,
  include_directories: project_inc,
  dependencies: gtest_dep,
)

test('unit', unit_tests)
