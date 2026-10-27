app_src = files(
    'main.cpp'
)

cli11_dep = dependency('CLI11')

app = executable(
    'app',
    app_src,
    include_directories: project_inc,
    dependencies: [http_dep, cli11_dep]
)

test('runs', app)
