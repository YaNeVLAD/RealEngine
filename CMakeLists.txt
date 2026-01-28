cmake_minimum_required(VERSION 3.31)
project(RealEngine VERSION 0.1.0 LANGUAGES CXX)

include(FetchContent)

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

FetchContent_Declare(
        EnTT
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.15.0
)
FetchContent_MakeAvailable(EnTT)

FetchContent_Declare(SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.2
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)

add_subdirectory(src/Core)
add_subdirectory(src/Scripting)
add_subdirectory(src/Runtime)
add_subdirectory(src/Launcher)