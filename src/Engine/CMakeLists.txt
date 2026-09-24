re_add_module(ReEngine TYPE SHARED)

target_link_libraries(ReEngine
        PRIVATE
        ReCore
        ReECS
        ReRenderCore
        ReRuntime
        ReScripting
)

target_compile_definitions(ReEngine PRIVATE RE_ENGINE_EXPORTS)