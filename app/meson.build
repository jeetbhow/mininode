cli_src = files(
    'cli.cpp'
)

cli11_dep = dependency('CLI11')

cli = executable(
    'cli',
    cli_src,
    include_directories: project_inc,
    dependencies: [http_dep, cli11_dep]
)