include(FetchContent)

# EnTT
FetchContent_Declare(
        EnTT
        GIT_REPOSITORY https://github.com/skypjack/entt.git
        GIT_TAG v3.15.0
)
FetchContent_MakeAvailable(EnTT)

# GLM
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 0af55ccecd98d4e5a8d1fad7de25ba429d60e863
)
FetchContent_MakeAvailable(glm)

if (RE_RENDER_BACKEND STREQUAL "SFML") # SFML
    FetchContent_Declare(
            SFML
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG 3.0.2
            GIT_SHALLOW ON
            EXCLUDE_FROM_ALL
            SYSTEM)
    FetchContent_MakeAvailable(SFML)
    set(RENDER_LIBS sfml-graphics)
    add_compile_definitions(RE_USE_SFML_RENDER)

elseif (RE_RENDER_BACKEND STREQUAL "GLFW") # GLFW
    FetchContent_Declare(
            glfw
            GIT_REPOSITORY https://github.com/glfw/glfw.git
            GIT_TAG 3.3.8
    )
    FetchContent_MakeAvailable(glfw)

    # GLAD
    set(GLAD_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/glad")
    add_library(GLAD STATIC "${GLAD_PATH}/glad.c")
    target_include_directories(GLAD PUBLIC "${GLAD_PATH}")

    set(RENDER_LIBS glfw glm GLAD)
    add_compile_definitions(RE_USE_GLFW_RENDER)
endif ()

# FSM
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