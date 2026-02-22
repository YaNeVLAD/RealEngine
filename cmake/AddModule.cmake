# add a new target which is Real Engine module
# for this function to work properly you need to organize module folders like this:
# <module_name>/
#   include/
#     <module_name>/
#       Header files (.h .hpp)
#   src/
#     Source files (.cpp)
function(re_add_module target_name)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs SOURCES)

    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if (NOT ARG_SOURCES)
        file(GLOB_RECURSE ARG_SOURCES "src/*.cpp" "include/*.hpp" "include/*.h")
        message(STATUS "Module ${target_name}: No sources provided, using auto-discovery.")
    endif ()

    add_library(${target_name} ${ARG_SOURCES})

    target_include_directories(${target_name}
            PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
            $<INSTALL_INTERFACE:include>
            PRIVATE
            src
    )

    set_target_properties(${target_name} PROPERTIES LINKER_LANGUAGE CXX)

    if (MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else ()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif ()
endfunction()