set(COMMON_SOURCES
        src/Renderer2D.cpp
)

if (RE_RENDER_BACKEND STREQUAL "GLFW")
    message(STATUS "Renderer2D: Building with OpenGL/GLFW backend")
    file(GLOB_RECURSE BACKEND_SOURCES
            "src/OpenGL/*.cpp"
            "include/Renderer2D/OpenGL/*.hpp"
    )

elseif (RE_RENDER_BACKEND STREQUAL "SFML")
    message(STATUS "Renderer2D: Building with SFML backend")
    file(GLOB_RECURSE BACKEND_SOURCES
            "src/SFML/*.cpp"
            "include/Renderer2D/SFML/*.hpp"
    )

endif ()

re_add_module(ReRender2D
        SOURCES
        ${COMMON_SOURCES}
        ${BACKEND_SOURCES}
)

target_link_libraries(ReRender2D
        PUBLIC
        ReCore
        ReRenderCore
)

target_compile_definitions(ReRender2D PRIVATE RE_RENDER_2D_EXPORTS)