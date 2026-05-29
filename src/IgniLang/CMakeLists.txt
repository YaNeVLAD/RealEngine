re_add_module(IgniLang)

target_link_libraries(IgniLang
        PUBLIC
        ReCore

        FSM
        nlohmann_json::nlohmann_json
)

target_compile_definitions(IgniLang PRIVATE IGNI_EXPORT)

enable_testing()
include(FetchContent)
FetchContent_Declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG v1.14.0
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

add_executable(IgniTests
        tests/IgniLangTests.cpp
        tests/FrontendTests.cpp
        tests/E2ETests.cpp
        tests/DotNetE2ETests.cpp
)

target_link_libraries(IgniTests
        PRIVATE
        IgniLang
        ReRVM
        ReCore
        gtest_main
)

include(GoogleTest)
gtest_discover_tests(IgniTests)

add_custom_command(TARGET IgniTests PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/tests/scripts"
        "$<TARGET_FILE_DIR:IgniTests>/tests/scripts"
        COMMENT "Copying test scripts to output directory..."
)

# CODEGEN_TOOL_TARGET
add_executable(IgniCodeGen tools/CodeGen/main.cpp)
target_link_libraries(IgniCodeGen PRIVATE ReCore nlohmann_json::nlohmann_json)

# SEMANTICS GENERATION
set(SEMANTICS_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/semantics.json")
set(TARGETS_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/targets.json")

set(GENERATED_SEMANTICS_CPP "${CMAKE_CURRENT_BINARY_DIR}/GeneratedSemantics.cpp")
set(GENERATED_SEMANTICS_HPP "${CMAKE_CURRENT_BINARY_DIR}/GeneratedSemantics.hpp")

add_custom_command(
        OUTPUT ${GENERATED_SEMANTICS_CPP} ${GENERATED_SEMANTICS_HPP}
        COMMAND IgniCodeGen ${GENERATED_SEMANTICS_HPP} ${GENERATED_SEMANTICS_CPP} ${SEMANTICS_CONFIG} ${TARGETS_CONFIG}
        DEPENDS IgniCodeGen ${SEMANTICS_CONFIG} ${TARGETS_CONFIG}
        COMMENT "Generating semantics logic from JSON using IgniCodeGen..."
)

# AST GENERATION
set(AST_SCHEMA_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/ast_schema.json")

set(GENERATED_AST_CPP "${CMAKE_CURRENT_BINARY_DIR}/AstNodesGenerated.cpp")
set(GENERATED_AST_HPP "${CMAKE_CURRENT_BINARY_DIR}/AstNodesGenerated.hpp")

add_custom_command(
        OUTPUT ${GENERATED_AST_CPP} ${GENERATED_AST_HPP}
        COMMAND IgniCodeGen ${GENERATED_AST_HPP} ${GENERATED_AST_CPP} ${AST_SCHEMA_CONFIG}
        DEPENDS IgniCodeGen ${AST_SCHEMA_CONFIG}
        COMMENT "Generating AST structures and JSON serializers using IgniCodeGen..."
)

target_sources(IgniLang
        PRIVATE
        ${GENERATED_SEMANTICS_CPP} ${GENERATED_SEMANTICS_HPP}
        ${GENERATED_AST_CPP} ${GENERATED_AST_HPP}
)

target_include_directories(IgniLang PUBLIC ${CMAKE_CURRENT_BINARY_DIR})