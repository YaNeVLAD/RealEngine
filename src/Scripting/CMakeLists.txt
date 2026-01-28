file(GLOB_RECURSE SOURCES "src/*.cpp" "include/*.hpp")

add_library(ReScripting ${SOURCES})

target_include_directories(ReScripting
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        PRIVATE
        src
)

target_link_libraries(ReScripting PUBLIC ReCore)

target_compile_definitions(ReScripting PRIVATE SCRIPTING_EXPORTS)
set_target_properties(ReScripting PROPERTIES LINKER_LANGUAGE CXX)

if(MSVC)
    target_compile_options(ReScripting PRIVATE /W4)
else()
    target_compile_options(ReScripting PRIVATE -Wall -Wextra)
endif()