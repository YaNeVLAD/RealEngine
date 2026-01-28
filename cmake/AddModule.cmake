# add a new target which is Real Engine module
function(re_add_module target_name)
    file(GLOB_RECURSE SOURCES "src/*.cpp" "include/*.hpp" "include/*.h")

    add_library(${target_name} ${SOURCES})

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