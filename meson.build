project(
    'accion',
    'c',
)

libm = meson.get_compiler('c').find_library('libm', required: false)

cmake = import('cmake')

raylib_proj = cmake.subproject('raylib')

executable(
    meson.project_name(),
    sources: [
        'src/main.c',
        'src/galton_board.c',
    ],
    dependencies: [
        raylib_proj.dependency('raylib'),
        libm,
    ],
    install: true,
)