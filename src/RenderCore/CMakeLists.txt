set(COMMON_SOURCES
        src/Image.cpp
        src/Keyboard.cpp
        src/Mouse.cpp
)

if (RE_RENDER_BACKEND STREQUAL "GLFW")
    message(STATUS "RenderCore: Building with OpenGL/GLFW backend")
    file(GLOB_RECURSE BACKEND_SOURCES
            "src/GLFW/*.cpp"
            "include/RenderCore/GLFW/*.hpp"
    )
    set(BACKEND_LIBS
            glfw
            glm
            GLAD
    )
    set(BACKEND_DEFINES RE_RENDER_PLATFORM_OPENGL)

elseif (RE_RENDER_BACKEND STREQUAL "SFML")
    message(STATUS "RenderCore: Building with SFML backend")
    file(GLOB_RECURSE BACKEND_SOURCES
            "src/SFML/*.cpp"
            "include/RenderCore/SFML/*.hpp"
    )
    set(BACKEND_LIBS
            SFML::Graphics
            SFML::Window
            SFML::System
    )
    set(BACKEND_DEFINES RE_RENDER_PLATFORM_SFML)

endif ()

re_add_module(ReRenderCore
        SOURCES
        ${COMMON_SOURCES}
        ${BACKEND_SOURCES})

target_link_libraries(ReRenderCore
        PUBLIC
        ReCore
        ${BACKEND_LIBS}

        PRIVATE
        stb_image
        stb_image_resize
        glm
)

target_compile_definitions(ReRenderCore PRIVATE RE_RENDER_CORE_EXPORTS ${BACKEND_DEFINES})