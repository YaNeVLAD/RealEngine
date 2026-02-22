re_add_module(ReRender2D)

target_link_libraries(ReRender2D
        PUBLIC
        ReCore
        ReRenderCore
)

target_compile_definitions(ReRender2D PRIVATE RE_RENDER_2D_EXPORTS)