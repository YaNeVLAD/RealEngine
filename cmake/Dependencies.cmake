include(FetchContent)


# GLM
FetchContent_Declare(
        glm
        GIT_REPOSITORY https://github.com/g-truc/glm.git
        GIT_TAG 1.0.1
)
set(GLM_BUILD_LIBRARY OFF CACHE INTERNAL "")
set(BUILD_SHARED_LIBS_SAVED ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)

FetchContent_MakeAvailable(glm)

set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_SAVED})

# EnTT
if (RE_USE_ENTT)
    FetchContent_Declare(
            EnTT
            GIT_REPOSITORY https://github.com/skypjack/entt.git
            GIT_TAG v3.15.0
    )
    FetchContent_MakeAvailable(EnTT)
endif ()

# TinyObjLoader
FetchContent_Declare(
        tinyobjloader
        GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
        GIT_TAG release
)
FetchContent_MakeAvailable(tinyobjloader)
target_compile_definitions(tinyobjloader INTERFACE TINYOBJLOADER_DISABLE_FAST_FLOAT)
target_compile_definitions(tinyobjloader PRIVATE TINYOBJLOADER_DISABLE_FAST_FLOAT)

# JoltPhysics
FetchContent_Declare(
        Jolt
        GIT_REPOSITORY https://github.com/jrouwe/JoltPhysics.git
        GIT_TAG v5.5.0
        SOURCE_SUBDIR "Build"
)

set(USE_STATIC_MSVC_RUNTIME_LIBRARY OFF CACHE INTERNAL "Use dynamic MSVC runtime for Jolt")
set(TARGET_UNIT_TESTS OFF CACHE INTERNAL "Disable Jolt unit tests")
set(TARGET_HELLO_WORLD OFF CACHE INTERNAL "Disable Jolt hello world")
set(TARGET_PERFORMANCE_TEST OFF CACHE INTERNAL "Disable Jolt performance test")
set(TARGET_SAMPLES OFF CACHE INTERNAL "Disable Jolt samples")
set(TARGET_VIEWER OFF CACHE INTERNAL "Disable Jolt viewer")

FetchContent_MakeAvailable(Jolt)

if (RE_RENDER_BACKEND STREQUAL "SFML") # SFML
    FetchContent_Declare(
            SFML
            GIT_REPOSITORY https://github.com/SFML/SFML.git
            GIT_TAG 3.0.2
            GIT_SHALLOW ON
            EXCLUDE_FROM_ALL
            SYSTEM)
    FetchContent_MakeAvailable(SFML)

    set(RENDER_LIBS
            SFML::Graphics
            SFML::Window
            SFML::System
            glm::glm
    )
    add_compile_definitions(RE_USE_SFML_RENDER)

elseif (RE_RENDER_BACKEND STREQUAL "GLFW") # GLFW
    # GLFW
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

    set(RENDER_LIBS
            glfw
            glm::glm
            GLAD
    )
    add_compile_definitions(RE_USE_GLFW_RENDER)
endif ()

# FSM
FetchContent_Declare(
        FSM
        GIT_REPOSITORY https://github.com/YaNeVLAD/StateMachine.git
        GIT_TAG master
)
FetchContent_MakeAvailable(FSM)

# IMGUI
FetchContent_Declare(
        imgui_repo
        GIT_REPOSITORY https://github.com/ocornut/imgui.git
        GIT_TAG v1.91.1
)
FetchContent_MakeAvailable(imgui_repo)
set(IMGUI_SOURCES
        "${imgui_repo_SOURCE_DIR}/imgui.cpp"
        "${imgui_repo_SOURCE_DIR}/imgui_draw.cpp"
        "${imgui_repo_SOURCE_DIR}/imgui_tables.cpp"
        "${imgui_repo_SOURCE_DIR}/imgui_widgets.cpp"
        "${imgui_repo_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
        "${imgui_repo_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp"
)
add_library(imgui STATIC ${IMGUI_SOURCES})
target_include_directories(imgui PUBLIC
        "${imgui_repo_SOURCE_DIR}"
        "${imgui_repo_SOURCE_DIR}/backends"
)
if (RE_RENDER_BACKEND STREQUAL "GLFW")
    target_link_libraries(imgui PRIVATE glfw)
    target_compile_definitions(imgui PUBLIC IMGUI_IMPL_OPENGL_LOADER_GLAD)
endif ()

# TINYGLTF
set(TINYGLTF_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(TINYGLTF_BUILD_LOADER_EXAMPLE OFF CACHE BOOL "" FORCE)
set(TINYGLTF_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
        tinygltf
        GIT_REPOSITORY https://github.com/syoyo/tinygltf.git
        GIT_TAG v3.0.0
)

set(BUILD_SHARED_LIBS_SAVED ${BUILD_SHARED_LIBS})
set(BUILD_SHARED_LIBS OFF)

FetchContent_MakeAvailable(tinygltf)

set(BUILD_SHARED_LIBS ${BUILD_SHARED_LIBS_SAVED})

# NLOHMANN_JSON
FetchContent_Declare(
        nlohmann_json
        GIT_REPOSITORY https://github.com/nlohmann/json.git
        GIT_TAG v3.11.3
)
FetchContent_MakeAvailable(nlohmann_json)

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

# MINIAUDIO
set(MINIAUDIO_PATH "${CMAKE_CURRENT_SOURCE_DIR}/external/miniaudio")
if (NOT EXISTS "${MINIAUDIO_PATH}/miniaudio_impl.cpp")
    file(WRITE "${MINIAUDIO_PATH}/miniaudio_impl.cpp"
            "#define MINIAUDIO_IMPLEMENTATION\n#include \"miniaudio.h\"")
endif ()
add_library(miniaudio STATIC "${MINIAUDIO_PATH}/miniaudio_impl.cpp")
target_include_directories(miniaudio PUBLIC "${MINIAUDIO_PATH}")

# .NET CORE HOST (nethost)
find_program(DOTNET_CLI dotnet)
if (NOT DOTNET_CLI)
    message(FATAL_ERROR "dotnet CLI not found. Install .NET 10 SDK (or higher).")
endif()

if (WIN32)
    set(NETHOST_RID "win-x64")
    set(NETHOST_LIB_NAME "nethost")
    set(DEFAULT_DOTNET_ROOT "C:/Program Files/dotnet")
endif()

if (DEFINED ENV{DOTNET_ROOT})
    set(DOTNET_ROOT "$ENV{DOTNET_ROOT}")
else()
    set(DOTNET_ROOT ${DEFAULT_DOTNET_ROOT})
endif()

set(DOTNET_TARGET_VERSION "10.0")

file(GLOB NETHOST_SEARCH_PATHS "${DOTNET_ROOT}/packs/Microsoft.NETCore.App.Host.${NETHOST_RID}/${DOTNET_TARGET_VERSION}*/runtimes/${NETHOST_RID}/native")

if (NOT NETHOST_SEARCH_PATHS)
    message(FATAL_ERROR "Can't find nethost packets for .NET ${DOTNET_TARGET_VERSION}.\n Make sure that installed .NET SDK match your architecture (x64/arm64).")
endif()

list(SORT NETHOST_SEARCH_PATHS COMPARE NATURAL ORDER DESCENDING)
list(GET NETHOST_SEARCH_PATHS 0 NETHOST_PATH)
message(STATUS "Found .NET Host (nethost): ${NETHOST_PATH}")

find_path(NETHOST_INCLUDE_DIR
        NAMES nethost.h hostfxr.h coreclr_delegates.h
        PATHS ${NETHOST_PATH}
        NO_DEFAULT_PATH
)

find_library(NETHOST_LIBRARY
        NAMES ${NETHOST_LIB_NAME} libnethost.a nethost.lib
        PATHS ${NETHOST_PATH}
        NO_DEFAULT_PATH
)

if (WIN32)
    find_file(NETHOST_DLL
            NAMES nethost.dll
            PATHS ${NETHOST_PATH}
            NO_DEFAULT_PATH
    )
    
    if (NETHOST_DLL)
        # file(COPY ...) отрабатывает при генерации кэша CMake.
        # Он скопирует nethost.dll прямо в вашу папку bin.
        file(COPY "${NETHOST_DLL}" DESTINATION "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
        message(STATUS "Copied nethost.dll to ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
    else()
        message(WARNING "nethost.dll not found in ${NETHOST_PATH}")
    endif()
endif()

if (NETHOST_INCLUDE_DIR AND NETHOST_LIBRARY)
    add_library(dotnet_host STATIC IMPORTED)
    set_target_properties(dotnet_host PROPERTIES
            IMPORTED_LOCATION "${NETHOST_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${NETHOST_INCLUDE_DIR}"
    )
else()
    message(FATAL_ERROR "nethost headers not found in path: ${NETHOST_PATH}")
endif()