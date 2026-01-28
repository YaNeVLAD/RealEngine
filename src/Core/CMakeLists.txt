file(GLOB_RECURSE SOURCES "src/*.cpp" "include/*.hpp")

add_library(ReCore SHARED ${SOURCES})

target_include_directories(ReCore
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        src
)

target_compile_definitions(ReCore PRIVATE CORE_EXPORTS)
set_target_properties(ReCore PROPERTIES LINKER_LANGUAGE CXX)

if(MSVC)
    target_compile_options(ReCore PRIVATE /W4)
else()
    target_compile_options(ReCore PRIVATE -Wall -Wextra -Wpedantic)
endif()