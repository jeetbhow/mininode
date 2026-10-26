gtest_dep = dependency('gtest', main: true, fallback: ['gtest', 'gtest_main_dep'])

test_sources = files(
    'http_test.cpp',
)

unit_tests = executable(
  'unit_tests',
  test_sources,
  dependencies: [http_dep, gtest_dep],
)

test('unit', unit_tests)
