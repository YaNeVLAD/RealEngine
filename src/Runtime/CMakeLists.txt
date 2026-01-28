file(GLOB_RECURSE SOURCES "src/*.cpp" "include/*.hpp")

add_library(ReRuntime SHARED ${SOURCES})

target_include_directories(ReRuntime
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PRIVATE
        src
)

target_link_libraries(ReRuntime
        PUBLIC
        ReCore
        ReScripting
        EnTT::EnTT
        SFML::Graphics
        SFML::Window
        SFML::System
)

target_compile_definitions(ReRuntime PRIVATE RUNTIME_EXPORTS)
set_target_properties(ReRuntime PROPERTIES LINKER_LANGUAGE CXX)

if(MSVC)
    target_compile_options(ReRuntime PRIVATE /W4)
else()
    target_compile_options(ReRuntime PRIVATE -Wall -Wextra)
endif()