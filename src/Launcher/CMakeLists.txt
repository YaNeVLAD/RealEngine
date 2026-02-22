add_executable(ReLauncher main.cpp
        Components.hpp
        HierarchySystem.hpp

        Lab1/Letters/JumpPhysicsSystem.hpp
        Lab1/Letters/LettersLayout.hpp

        Lab1/Circle/CircleLayout.hpp

        Lab1/Hangman/HangmanLayout.hpp
        Lab1/Hangman/HangmanLayout.cpp

        Lab2/Alchemy/AlchemyLayout.hpp

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

if (WIN32 AND RENDER_LIBS)
    foreach (lib ${RENDER_LIBS})
        if (TARGET ${lib})
            get_target_property(target_type ${lib} TYPE)

            if (target_type STREQUAL "SHARED_LIBRARY")
                add_custom_command(TARGET ReLauncher POST_BUILD
                        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                        "$<TARGET_FILE:${lib}>"
                        "$<TARGET_FILE_DIR:ReLauncher>"
                        COMMENT "Copying ${lib} DLL to output directory"
                        COMMAND_EXPAND_LISTS
                )
            endif ()
        endif ()
    endforeach ()
endif ()