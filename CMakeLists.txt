cmake_minimum_required(VERSION 3.14)  # Specify the minimum CMake version
project(accion)
set(VERSION "0.1")

# Generates compile_commands.json
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
    message("CMAKE_BUILD_TYPE is not defined, will use Debug mode by default.")
endif()

# Making a string `build_type` so it is convinient
string(TOLOWER ${CMAKE_BUILD_TYPE} build_type)

if(NOT MSVC)
  message("Preparing build directories")
  set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT my_executable)
  if(build_type STREQUAL debug)
      set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Debug)
  else()
      set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/Release)
  endif()
endif()


# Declare source files
file(GLOB SOURCES 
    ${PROJECT_SOURCE_DIR}/src/*.c
    ${PROJECT_SOURCE_DIR}/src/*.cpp
)


# Declare executable
add_executable(accion_exe)

# Keep executable name consistent across platforms
if(WIN32)
    set_target_properties(accion_exe PROPERTIES OUTPUT_NAME "${PROJECT_NAME}")
else()
    set_target_properties(accion_exe PROPERTIES OUTPUT_NAME "${PROJECT_NAME}.exe")
endif()

target_sources(accion_exe PRIVATE ${SOURCES})

# Prepare Raylib library installation directory
set(RAYLIB_INSTALL_DIR "${CMAKE_BINARY_DIR}/raylib-prebuilt")

if(MSVC)
    set(RAYLIB_URL "https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_win64_msvc16.zip")
elseif(MINGW)
    set(RAYLIB_URL "https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_win64_mingw-w64.zip")
elseif(LINUX)
    set(RAYLIB_URL "https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_linux_amd64.tar.gz")
else()#APPLE
    set(RAYLIB_URL "https://github.com/raysan5/raylib/releases/download/5.0/raylib-5.0_macos.tar.gz")
endif()
include(FetchContent)
FetchContent_Populate(
  raylib_prebuilt_lib
  URL      ${RAYLIB_URL}
  SOURCE_DIR ${RAYLIB_INSTALL_DIR}
)

# Specify the include and library directories
set(RAYLIB_INCLUDE_DIR "${RAYLIB_INSTALL_DIR}/include")
set(RAYLIB_SHARED_DIR "${RAYLIB_INSTALL_DIR}/lib")

# Add the include directories
include_directories(accion_exe ${RAYLIB_INCLUDE_DIR})
link_directories(accion_exe ${RAYLIB_SHARED_DIR})

# Add warning flags
if(MSVC)
  target_compile_options(accion_exe PRIVATE /W3 )
else()
  target_compile_options(accion_exe PRIVATE -Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion)
endif()


# link library

if(LINUX)
    set(RAYLIB_SHARED_LIB "libraylib.so.5.0.0")
elseif(APPLE)
    set(RAYLIB_SHARED_LIB "libraylib.5.0.0.dylib")
else()
    set(RAYLIB_SHARED_LIB "raylib.dll")
endif()

target_link_libraries(accion_exe PRIVATE "${RAYLIB_SHARED_DIR}/${RAYLIB_SHARED_LIB}")

# Copies assets folder to the build directory
add_custom_command(TARGET accion_exe PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${PROJECT_SOURCE_DIR}/assets"
        $<TARGET_FILE_DIR:accion_exe>/assets)


# Copy shared lib to the build directory
add_custom_command(
    TARGET accion_exe PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy ${RAYLIB_SHARED_DIR}/${RAYLIB_SHARED_LIB} $<TARGET_FILE_DIR:accion_exe>    # <--this is out-file path
    COMMENT "Copying shared library: ${RAYLIB_SHARED_LIB}")