add_executable(ReLauncher "main.cpp"
        LauncherApplication.hpp)

set_target_properties(ReLauncher
        PROPERTIES
        WIN32_EXECUTABLE "$<OR:$<CONFIG:Release>,$<CONFIG:Dist>>"
)

target_compile_definitions(ReLauncher
        PRIVATE
        "$<$<OR:$<CONFIG:Release>,$<CONFIG:Dist>>:RE_DIST>"
)

target_link_libraries(ReLauncher PRIVATE ReRuntime)

if (WIN32)
    add_custom_command(TARGET ReLauncher POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SFML::Graphics>
            $<TARGET_FILE:SFML::Window>
            $<TARGET_FILE:SFML::System>
            $<TARGET_FILE_DIR:ReLauncher>
    )
endif ()
