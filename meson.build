project(
    'accion',
    'c',
    default_options: {
        'std_c': 'gnu23',
    },
)

cmake = import('cmake')

raylib_proj = cmake.subproject('raylib')

# raylib_proj.include_directories('tgt')

executable(
    meson.project_name(),
    sources: [
        'src/main.c',
        'src/galton_board.c',
    ],
    dependencies: raylib_proj.dependency('raylib'),
)