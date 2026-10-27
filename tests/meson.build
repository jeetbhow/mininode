gtest_dep = dependency('gtest', main: true, fallback: ['gtest', 'gtest_main_dep'])

test_sources = files(
    'http_test.cpp',
    'headers_test.cpp',
)

unit_tests = executable(
  'unit_tests',
  test_sources,
  dependencies: [http_dep, gtest_dep],
)

# GTEST_BRIEF prints only failures, so they fit in the log tail that
# `meson test --print-errorlogs` shows.
test('unit', unit_tests, env: ['GTEST_BRIEF=1'])
