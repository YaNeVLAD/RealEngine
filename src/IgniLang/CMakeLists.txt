re_add_module(IgniLang)

target_link_libraries(IgniLang
        PUBLIC
        ReCore

        FSM
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
add_executable(SemanticsCodeGen tools/CodeGen/main.cpp)
target_link_libraries(SemanticsCodeGen PRIVATE ReCore nlohmann_json::nlohmann_json)

set(SEMANTICS_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/semantics.json")
set(TARGETS_CONFIG "${CMAKE_CURRENT_SOURCE_DIR}/targets.json")

set(GENERATED_CPP "${CMAKE_CURRENT_BINARY_DIR}/GeneratedSemantics.cpp")
set(GENERATED_HPP "${CMAKE_CURRENT_BINARY_DIR}/GeneratedSemantics.hpp")

add_custom_command(
        OUTPUT ${GENERATED_CPP} ${GENERATED_HPP}
        COMMAND SemanticsCodeGen ${GENERATED_HPP} ${GENERATED_CPP} ${SEMANTICS_CONFIG} ${TARGETS_CONFIG}
        DEPENDS SemanticsCodeGen ${SEMANTICS_CONFIG} ${TARGETS_CONFIG}
        COMMENT "Generating semantics logic from JSON using SemanticsCodeGen..."
)

target_sources(IgniLang PRIVATE ${GENERATED_CPP} ${GENERATED_HPP})

target_include_directories(IgniLang PUBLIC ${CMAKE_CURRENT_BINARY_DIR})