add_executable(igni-cli
        src/igni-cli.cpp
)

target_include_directories(igni-cli
        PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
        PRIVATE
        src
)

target_link_libraries(igni-cli
        PUBLIC
        ReCore
        ReRVM
        IgniLang

        FSM
)

target_compile_definitions(igni-cli PRIVATE IGNI_CLI_EXPORT)

add_custom_command(TARGET igni-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:igni-cli>/assets"
        COMMENT "Copying assets directory to build output..."
)