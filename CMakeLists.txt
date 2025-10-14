cmake_minimum_required(VERSION 3.31)
include(FetchContent)
project(Engine)

set(CMAKE_CXX_STANDARD 23)

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

add_executable(Engine main.cpp)

target_link_libraries(Engine PRIVATE
        EnTT::EnTT
        SFML::Graphics
)

if (MINGW)
    target_link_options(Engine PRIVATE -static -Wl,--allow-multiple-definition)
endif ()