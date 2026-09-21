re_add_module(ReScripting)

target_link_libraries(ReScripting
        PUBLIC
        ReCore
        ReRVM

        PRIVATE
        dotnet_host
)

target_compile_definitions(ReScripting PRIVATE RE_SCRIPTING_EXPORTS)

# Bridge reflection codegen: ComponentSchema.json -> C++ tables + C# mirrors.
find_package(Python3 COMPONENTS Interpreter REQUIRED)

set(BRIDGE_SCHEMA "${CMAKE_CURRENT_SOURCE_DIR}/bridge/ComponentSchema.json")
set(BRIDGE_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/bridge/generate_bridge.py")
set(BRIDGE_GEN_DIR "${CMAKE_CURRENT_BINARY_DIR}/gen")
set(BRIDGE_GEN_HPP_IDS "${BRIDGE_GEN_DIR}/bridge/ReflectionIds.gen.hpp")
set(BRIDGE_GEN_HPP_TABLES "${BRIDGE_GEN_DIR}/bridge/ReflectionTables.gen.inc")
set(BRIDGE_GEN_CS "${CMAKE_CURRENT_SOURCE_DIR}/EngineAPI/ReflectionSchema.gen.cs")

add_custom_command(
        OUTPUT "${BRIDGE_GEN_HPP_IDS}" "${BRIDGE_GEN_HPP_TABLES}" "${BRIDGE_GEN_CS}"
        COMMAND "${Python3_EXECUTABLE}" "${BRIDGE_GENERATOR}"
        --schema "${BRIDGE_SCHEMA}"
        --cpp-ids "${BRIDGE_GEN_HPP_IDS}"
        --cpp-tables "${BRIDGE_GEN_HPP_TABLES}"
        --csharp "${BRIDGE_GEN_CS}"
        DEPENDS "${BRIDGE_SCHEMA}" "${BRIDGE_GENERATOR}"
        COMMENT "Generating scripting bridge reflection tables..."
        VERBATIM
)

add_custom_target(BridgeCodegen
        DEPENDS "${BRIDGE_GEN_HPP_IDS}" "${BRIDGE_GEN_HPP_TABLES}" "${BRIDGE_GEN_CS}"
)

add_dependencies(ReScripting BridgeCodegen)

target_include_directories(ReScripting
        PUBLIC
        $<BUILD_INTERFACE:${BRIDGE_GEN_DIR}>
)

# CSharp Engine API Build
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if (IS_MULTI_CONFIG)
    set(CSHARP_OUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
else ()
    set(CSHARP_OUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif ()

set(ENGINE_API_CSPROJ "${CMAKE_CURRENT_SOURCE_DIR}/EngineAPI/EngineAPI.csproj")

add_custom_target(EngineAPI_Build ALL
        COMMAND dotnet build "${ENGINE_API_CSPROJ}"
        -c $<IF:$<CONFIG:Debug>,Debug,Release>
        -o "${CSHARP_OUT_DIR}"
        --nologo -v q
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        COMMENT "Compiling C# Engine API..."
        VERBATIM
)

add_dependencies(ReScripting EngineAPI_Build)
add_dependencies(EngineAPI_Build BridgeCodegen)