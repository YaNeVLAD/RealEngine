cmake_minimum_required(VERSION 3.31)
project(RealEngine VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(BUILD_SHARED_LIBS "Build using shared libraries" ON)

if (NOT BUILD_SHARED_LIBS)
    set(SFML_STATIC_LIBRARIES ON)

    add_compile_definitions(RE_STATIC)

    message(STATUS "Building Real Engine as STATIC libraries")
else ()
    message(STATUS "Building Real Engine as SHARED libraries")
endif ()

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(Dependencies)
include(AddModule)

add_subdirectory(src/Core)
add_subdirectory(src/Scripting)
add_subdirectory(src/Runtime)
add_subdirectory(src/ECS)
add_subdirectory(src/Launcher)