re_add_module(ReRVM)

target_link_libraries(ReRVM
        PUBLIC
        ReCore
        ReRenderCore # TODO: Remove dependency by moving IAsset to Core

        FSM
)

target_compile_definitions(ReRVM PRIVATE RE_RVM_EXPORTS)