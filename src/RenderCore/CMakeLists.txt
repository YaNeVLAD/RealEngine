re_add_module(ReRenderCore)

target_link_libraries(ReRenderCore
        PUBLIC
        ReCore

        SFML::System
        SFML::Window
        SFML::Graphics
)

target_compile_definitions(ReRenderCore PRIVATE RE_RENDER_CORE_EXPORTS)