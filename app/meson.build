app_src = files(
    'main.cpp'
)

app = executable(
    'app',
    app_src,
    include_directories: project_inc,
    dependencies: http_dep
)

test('runs', app)