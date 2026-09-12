set(COMMON_SOURCES
        src/Image.cpp
        src/Keyboard.cpp
        src/Mouse.cpp
        src/Model.cpp
        src/TextureLoader.cpp
        src/AnimatedModel.cpp
        src/AssetManager.cpp
        src/Animator.cpp
)

if (RE_RENDER_BACKEND STREQUAL "SFML")
    message(STATUS "RenderCore: Building with SFML backend")
    file(GLOB_RECURSE BACKEND_SOURCES
            "src/SFML/*.cpp"
            "include/RenderCore/SFML/*.hpp"
    )
    set(BACKEND_DEFINES RE_RENDER_PLATFORM_SFML)

elseif (RE_RENDER_BACKEND STREQUAL "FILAMENT")
    message(STATUS "RenderCore: Building with Filament backend")
    file(GLOB_RECURSE BACKEND_SOURCES
            "src/Filament/*.cpp"
            "include/RenderCore/Filament/*.hpp"
    )
    list(APPEND BACKEND_SOURCES
            "src/GLFW/GLFWWindow.cpp"
            "src/GLFW/Input.cpp"
    )
    set(BACKEND_DEFINES RE_RENDER_PLATFORM_FILAMENT)

    set(LIT_MATERIAL_SRC "${CMAKE_CURRENT_SOURCE_DIR}/materials/lit.mat")
    set(RENDER_CORE_GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated/resources")
    set(LIT_MATERIAL_OUT "${RENDER_CORE_GENERATED_DIR}/lit.filamat")
    set(LIT_RESOURCES_C "${RENDER_CORE_GENERATED_DIR}/lit_resources.c")
    set(LIT_RESOURCES_H "${RENDER_CORE_GENERATED_DIR}/lit_resources.h")

    add_custom_command(
            OUTPUT "${LIT_MATERIAL_OUT}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RENDER_CORE_GENERATED_DIR}"
            COMMAND "${MATC_TOOL}" -a opengl -a vulkan -a metal -o "${LIT_MATERIAL_OUT}" "${LIT_MATERIAL_SRC}"
            DEPENDS "${LIT_MATERIAL_SRC}"
            COMMENT "Compiling lit material..."
    )

    add_custom_command(
            OUTPUT "${LIT_RESOURCES_C}" "${LIT_RESOURCES_H}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${RENDER_CORE_GENERATED_DIR}"
            COMMAND ${CMAKE_COMMAND} -E chdir "${RENDER_CORE_GENERATED_DIR}"
            "${RESGEN_TOOL}" -c -p lit_resources "${LIT_MATERIAL_OUT}"
            DEPENDS "${LIT_MATERIAL_OUT}"
            COMMENT "Embedding lit material resources..."
    )

    list(APPEND BACKEND_SOURCES
            "${LIT_RESOURCES_C}"
            "${LIT_RESOURCES_H}"
    )
endif ()

re_add_module(ReRenderCore
        SOURCES
        ${COMMON_SOURCES}
        ${BACKEND_SOURCES})

if (RE_RENDER_BACKEND STREQUAL "FILAMENT")
    target_include_directories(ReRenderCore
            PRIVATE
            "${RENDER_CORE_GENERATED_DIR}"
    )
endif ()

target_link_libraries(ReRenderCore
        PUBLIC
        ReCore
        ${RENDER_LIBS}

        PRIVATE
        imgui
        filagui
        stb_image
        stb_image_resize
        bcdec
        tinyobjloader
        tinygltf
)

target_compile_definitions(ReRenderCore PRIVATE RE_RENDER_CORE_EXPORTS ${BACKEND_DEFINES})

add_custom_command(
        TARGET ReRenderCore POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/shaders"
        "$<TARGET_FILE_DIR:ReRenderCore>/../bin/shaders"
        COMMENT "Copying shaders to bin/shaders directory..."
)