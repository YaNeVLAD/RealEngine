re_add_module(ReECS)

target_link_libraries(ReECS
        PUBLIC
        ReCore

        EnTT::EnTT
)

target_compile_definitions(ReECS PRIVATE RE_ECS_EXPORTS)