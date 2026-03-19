re_add_module(IgniLang)

target_link_libraries(IgniLang
        PUBLIC
        ReCore

        FSM
)

target_compile_definitions(IgniLang PRIVATE IGNI_EXPORT)