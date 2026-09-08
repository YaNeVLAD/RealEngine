# adds a new Real Engine module target
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
        file(GLOB_RECURSE COMMON_SOURCES
                "src/*.cpp" "include/*.hpp" "include/*.h" "include/*.inl"
        )
        list(FILTER COMMON_SOURCES EXCLUDE REGEX "src/Platform/.*")

        set(PLATFORM_SOURCES "")
        if (WIN32)
            file(GLOB_RECURSE PLATFORM_SOURCES "src/Platform/Win32/*.cpp")
        else ()
            file(GLOB_RECURSE PLATFORM_SOURCES "src/Platform/Posix/*.cpp")

            if (CMAKE_SYSTEM_NAME STREQUAL "Linux")
                file(GLOB_RECURSE LINUX_SOURCES "src/Platform/Linux/*.cpp")
                list(APPEND PLATFORM_SOURCES ${LINUX_SOURCES})
            elseif (APPLE)
                file(GLOB_RECURSE APPLE_SOURCES "src/Platform/Apple/*.cpp")
                list(APPEND PLATFORM_SOURCES ${APPLE_SOURCES})
            endif ()
        endif ()

        set(ARG_SOURCES ${COMMON_SOURCES} ${PLATFORM_SOURCES})
        message(STATUS "Module ${target_name}: Auto-discovered ${ARG_SOURCES}")
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

    if (UNIX AND NOT APPLE)
        target_link_libraries(${target_name} PRIVATE ${CMAKE_DL_LIBS})
    endif ()

    if (MSVC)
        target_compile_options(${target_name} PRIVATE /W4)
    else ()
        target_compile_options(${target_name} PRIVATE -Wall -Wextra -Wpedantic)
    endif ()
endfunction()