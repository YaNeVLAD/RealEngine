include(FetchContent)

FetchContent_Declare(
        EnTT
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.15.0
)
FetchContent_MakeAvailable(EnTT)

FetchContent_Declare(
        SFML
        GIT_REPOSITORY https://github.com/SFML/SFML.git
        GIT_TAG 3.0.2
        GIT_SHALLOW ON
        EXCLUDE_FROM_ALL
        SYSTEM)
FetchContent_MakeAvailable(SFML)

FetchContent_Declare(
        FSM
        GIT_REPOSITORY https://github.com/YaNeVLAD/StateMachine.git
        GIT_TAG master
)
FetchContent_MakeAvailable(FSM)

# STB_IMAGE
set(STB_IMAGE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/stb")
if (NOT EXISTS "${STB_IMAGE_PATH}/stb_image_impl.cpp")
    file(WRITE "${STB_IMAGE_PATH}/stb_image_impl.cpp"
            "#define STB_IMAGE_IMPLEMENTATION\n#include \"stb_image.h\"")
endif ()

add_library(stb_image STATIC "${STB_IMAGE_PATH}/stb_image_impl.cpp")
target_include_directories(stb_image PUBLIC "${STB_IMAGE_PATH}")

# STB_IMAGE_RESIZE
set(STB_IMAGE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/stb")
if (NOT EXISTS "${STB_IMAGE_PATH}/stb_image_resize_impl.cpp")
    file(WRITE "${STB_IMAGE_PATH}/stb_image_resize_impl.cpp"
            "#define STB_IMAGE_RESIZE_IMPLEMENTATION\n#include \"stb_image_resize.h\"")
endif ()

add_library(stb_image_resize STATIC "${STB_IMAGE_PATH}/stb_image_resize_impl.cpp")
target_include_directories(stb_image_resize PUBLIC "${STB_IMAGE_PATH}")