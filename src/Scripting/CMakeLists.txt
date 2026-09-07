re_add_module(ReScripting)

target_link_libraries(ReScripting
        PUBLIC
        ReCore
        ReRVM

        PRIVATE
        dotnet_host
)

target_compile_definitions(ReScripting PRIVATE RE_SCRIPTING_EXPORTS)

# CSharp Engine API Build
get_property(IS_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(IS_MULTI_CONFIG)
    set(CSHARP_OUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/$<CONFIG>")
else()
    set(CSHARP_OUT_DIR "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
endif()

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