re_add_module(ReRuntime)

target_link_libraries(ReRuntime
        PUBLIC
        ReCore
        ReECS
        ReScripting
        EnTT::EnTT
        SFML::Graphics
        SFML::Window
        SFML::System
)

target_compile_definitions(ReRuntime PRIVATE RUNTIME_EXPORTS)