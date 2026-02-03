re_add_module(ReScripting)

target_link_libraries(ReScripting
        PUBLIC
        ReCore
)

target_compile_definitions(ReScripting PRIVATE RE_SCRIPTING_EXPORTS)