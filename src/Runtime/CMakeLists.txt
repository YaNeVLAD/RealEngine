re_add_module(ReRuntime)

target_link_libraries(ReRuntime
        PUBLIC
        ReCore
        ReECS
        ReScripting
        ReRenderCore
        ReRender2D
)

target_compile_definitions(ReRuntime PRIVATE RE_RUNTIME_EXPORTS)