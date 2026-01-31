re_add_module(ReECS)

target_link_libraries(ReECS
        PUBLIC
        ReCore
)

target_compile_definitions(ReECS PRIVATE ECS_EXPORTS)