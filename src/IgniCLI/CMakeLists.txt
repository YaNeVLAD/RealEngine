add_executable(igni-cli
        src/igni-cli.cpp
)

if (MSVC)
    target_compile_options(igni-cli PRIVATE /bigobj)
endif ()

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
)

target_compile_definitions(igni-cli PRIVATE IGNI_CLI_EXPORT)

add_executable(igni-backend-dotnet src/igni-backend-dotnet.cpp)
add_executable(igni-backend-rvm src/igni-backend-rvm.cpp)

add_custom_command(TARGET igni-cli POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:igni-cli>/assets"

        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_SOURCE_DIR}/backends.json"
        "$<TARGET_FILE_DIR:igni-cli>/backends.json"

        COMMENT "Copying assets and backends.json to build output..."
)